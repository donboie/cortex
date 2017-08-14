#include <assert.h>
#include <algorithm>

#include "tbb/concurrent_vector.h"
#include "tbb/parallel_for.h"

#include "boost/mpl/and.hpp"
#include "OpenEXR/ImathVec.h"

#include "IECore/DespatchTypedData.h"
#include "IECore/MeshAlgo.h"
#include "IECore/FaceVaryingPromotionOp.h"
#include "IECore/DespatchTypedData.h"
#include "IECore/DespatchTypedData.h"
#include "IECore/TypeTraits.h"
#include "IECore/private/PrimitiveAlgoUtils.h"

#include "IECore/private/CountToOffset.h"

using namespace IECore;
using namespace Imath;

namespace
{
template< typename U>
class DeleteFlaggedUniformFunctor
{
	public:
		typedef DataPtr ReturnType;

		DeleteFlaggedUniformFunctor( typename IECore::TypedData<std::vector<U> >::ConstPtr flagData ) : m_flagData( flagData )
		{
		}

		template<typename T>
		ReturnType operator()( const T *data )
		{
			const typename T::ValueType &inputs = data->readable();
			const std::vector<U> &flags = m_flagData->readable();

			T *filteredResultData = new T();
			ReturnType result(filteredResultData);

			typename T::ValueType &filteredResult = filteredResultData->writable();

			filteredResult.reserve( inputs.size() );

			for( size_t i = 0; i < inputs.size(); ++i )
			{
				if( !flags[i] )
				{
					filteredResult.push_back( inputs[i] );
				}
			}

			return result;
		}

	private:
		typename IECore::TypedData<std::vector<U> >::ConstPtr m_flagData;
};

template<typename U>
class DeleteFlaggedFaceVaryingFunctor
{
	public:
		typedef DataPtr ReturnType;

		DeleteFlaggedFaceVaryingFunctor( typename IECore::TypedData<std::vector<U> >::ConstPtr flagData, ConstIntVectorDataPtr verticesPerFaceData ) : m_flagData( flagData ), m_verticesPerFaceData( verticesPerFaceData )
		{
		}

		template<typename T>
		ReturnType operator()( const T *data )
		{
			const typename T::ValueType &inputs = data->readable();
			const std::vector<int> &verticesPerFace = m_verticesPerFaceData->readable();
			const std::vector<U> &flags = m_flagData->readable();

			T *filteredResultData = new T();
			typename T::ValueType &filteredResult = filteredResultData->writable();

			filteredResult.reserve( inputs.size() );

			size_t offset = 0;
			for( size_t f = 0; f < verticesPerFace.size(); ++f )
			{
				int numVerts = verticesPerFace[f];
				if( !flags[f] )
				{
					for( int v = 0; v < numVerts; ++v )
					{
						filteredResult.push_back( inputs[offset + v] );
					}
				}
				offset += numVerts;
			}

			return filteredResultData;
		}

	private:
		typename IECore::TypedData<std::vector<U> >::ConstPtr m_flagData;
		ConstIntVectorDataPtr m_verticesPerFaceData;

};

template<typename U>
class DeleteFlaggedVertexFunctor
{
	public:
		typedef DataPtr ReturnType;

		DeleteFlaggedVertexFunctor( size_t maxVertexId, ConstIntVectorDataPtr vertexIdsData, ConstIntVectorDataPtr verticesPerFaceData, typename IECore::TypedData<std::vector<U> >::ConstPtr flagData )
			: m_flagData( flagData ), m_verticesPerFaceData( verticesPerFaceData ), m_vertexIdsData( vertexIdsData )
		{
			const std::vector<int> &vertexIds = m_vertexIdsData->readable();
			const std::vector<int> &verticesPerFace = m_verticesPerFaceData->readable();
			const std::vector<U> &flags = m_flagData->readable();

			m_usedVerticesData = new BoolVectorData();
			std::vector<bool> &usedVertices = m_usedVerticesData->writable();

			usedVertices.resize( maxVertexId, false );

			size_t offset = 0;
			for( size_t f = 0; f < verticesPerFace.size(); ++f )
			{
				int numVerts = verticesPerFace[f];
				if( !flags[f] )
				{
					for( int v = 0; v < numVerts; ++v )
					{
						usedVertices[vertexIds[offset + v]] = true;
					}
				}
				offset += numVerts;
			}


			m_remappingData = new IntVectorData();
			std::vector<int> &remapping = m_remappingData->writable();

			// again this array is too large but large enough
			remapping.resize(maxVertexId, -1 );

			size_t newIndex = 0;
			for (size_t i = 0; i < usedVertices.size(); ++i)
			{
				if( usedVertices[i] )
				{
					remapping[i] = newIndex;
					newIndex++;
				}
			}
		}

		template<typename T>
		ReturnType operator()( const T *data )
		{
			const std::vector<bool> &usedVertices = m_usedVerticesData->readable();

			const typename T::ValueType &vertices = data->readable();

			T *filteredVerticesData = new T();
			typename T::ValueType &filteredVertices = filteredVerticesData->writable();

			filteredVertices.reserve( vertices.size() );

			for( size_t v = 0; v < vertices.size(); ++v )
			{
				if( usedVertices[v] )
				{
					filteredVertices.push_back( vertices[v] );
				}
			};

			return filteredVerticesData;
		}

		ConstIntVectorDataPtr getRemapping() const { return m_remappingData; }

	private:
		typename IECore::TypedData<std::vector<U> >::ConstPtr m_flagData;
		ConstIntVectorDataPtr m_verticesPerFaceData;
		ConstIntVectorDataPtr m_vertexIdsData;

		BoolVectorDataPtr m_usedVerticesData;

		// map from old vertex index to new
		IntVectorDataPtr m_remappingData;
};

template<typename T>
MeshPrimitivePtr deleteFaces( const MeshPrimitive* meshPrimitive, const typename IECore::TypedData<std::vector<T> > *deleteFlagData)
{
	// construct 3 functors for deleting (uniform, vertex & face varying) primvars
	DeleteFlaggedUniformFunctor<T> uniformFunctor(deleteFlagData);
	DeleteFlaggedFaceVaryingFunctor<T> faceVaryingFunctor(deleteFlagData, meshPrimitive->verticesPerFace());
	DeleteFlaggedVertexFunctor<T> vertexFunctor( meshPrimitive->variableSize(PrimitiveVariable::Vertex), meshPrimitive->vertexIds(), meshPrimitive->verticesPerFace(), deleteFlagData );

	// filter verticesPerFace using DeleteFlaggedUniformFunctor
	IECore::Data *inputVerticesPerFace = const_cast< IECore::Data * >( IECore::runTimeCast<const IECore::Data>( meshPrimitive->verticesPerFace() ) );
	IECore::DataPtr outputVerticesPerFace = despatchTypedData<DeleteFlaggedUniformFunctor<T>, TypeTraits::IsVectorTypedData>( inputVerticesPerFace, uniformFunctor );
	IntVectorDataPtr verticesPerFace = IECore::runTimeCast<IECore::IntVectorData>(outputVerticesPerFace);

	// filter VertexIds using DeleteFlaggedFaceVaryingFunctor
	IECore::Data *inputVertexIds = const_cast< IECore::Data * >( IECore::runTimeCast<const IECore::Data>( meshPrimitive->vertexIds() ) );
	IECore::DataPtr outputVertexIds = despatchTypedData<DeleteFlaggedFaceVaryingFunctor<T>, TypeTraits::IsVectorTypedData>( inputVertexIds, faceVaryingFunctor );

	// remap the indices also
	ConstIntVectorDataPtr remappingData = vertexFunctor.getRemapping();
	const std::vector<int>& remapping = remappingData->readable();

	IntVectorDataPtr vertexIdsData = IECore::runTimeCast<IECore::IntVectorData>(outputVertexIds);
	IntVectorData::ValueType& vertexIds = vertexIdsData->writable();

	for (size_t i = 0; i < vertexIds.size(); ++i)
	{
		vertexIds[i] = remapping[vertexIds[i]];
	}

	// construct mesh without positions as they'll be set when filtering the primvars
	MeshPrimitivePtr outMeshPrimitive = new MeshPrimitive(verticesPerFace, vertexIdsData, meshPrimitive->interpolation());

	for (PrimitiveVariableMap::const_iterator it = meshPrimitive->variables.begin(), e = meshPrimitive->variables.end(); it != e; ++it)
	{
		switch(it->second.interpolation)
		{
			case PrimitiveVariable::Uniform:
			{
				IECore::Data *inputData = const_cast< IECore::Data * >( it->second.data.get() );
				IECore::DataPtr outputData = despatchTypedData<DeleteFlaggedUniformFunctor<T>, TypeTraits::IsVectorTypedData>( inputData, uniformFunctor );
				outMeshPrimitive->variables[it->first] = PrimitiveVariable( it->second.interpolation, outputData );
				break;
			}
			case PrimitiveVariable::Vertex:
			case PrimitiveVariable::Varying:
			{
				IECore::Data *inputData = const_cast< IECore::Data * >( it->second.data.get() );
				IECore::DataPtr ouptputData = despatchTypedData<DeleteFlaggedVertexFunctor<T>, TypeTraits::IsVectorTypedData>( inputData, vertexFunctor );
				outMeshPrimitive->variables[it->first] = PrimitiveVariable( it->second.interpolation, ouptputData );
				break;
			}

			case PrimitiveVariable::FaceVarying:
			{
				IECore::Data *inputData = const_cast< IECore::Data * >( it->second.data.get() );
				IECore::DataPtr outputData = despatchTypedData<DeleteFlaggedFaceVaryingFunctor<T>, TypeTraits::IsVectorTypedData>( inputData, faceVaryingFunctor );
				outMeshPrimitive->variables[it->first] = PrimitiveVariable ( it->second.interpolation, outputData);
				break;
			}
			case PrimitiveVariable::Constant:
			case PrimitiveVariable::Invalid:
			{
				outMeshPrimitive->variables[it->first] = it->second;
				break;
			}
		}
	}
	return outMeshPrimitive;
}

struct MeshVertexToUniform
{
	typedef DataPtr ReturnType;

	MeshVertexToUniform( const MeshPrimitive *mesh )	:	m_mesh( mesh )
	{
	}

	template<typename From> ReturnType operator()( const From* data )
	{
		typename From::Ptr result = static_cast< From* >( Object::create( data->typeId() ).get() );
		typename From::ValueType &trg = result->writable();
		const typename From::ValueType &src = data->readable();

		// TODO sipmlify this code by using polygon face & vertex iterators
		trg.reserve( m_mesh->numFaces() );

		std::vector<int>::const_iterator vId = m_mesh->vertexIds()->readable().begin();
		const std::vector<int> &verticesPerFace = m_mesh->verticesPerFace()->readable();
		for( std::vector<int>::const_iterator it = verticesPerFace.begin(); it < verticesPerFace.end(); ++it )
		{
			// initialize with the first value to avoid
			// ambiguitity during default construction
			typename From::ValueType::value_type total = src[ *vId ];
			++vId;

			for( int j = 1; j < *it; ++j, ++vId )
			{
				total += src[ *vId ];
			}

			trg.push_back( total / *it );
		}

		return result;
	}

	const MeshPrimitive *m_mesh;
};

struct MeshUniformToVertex
{
	typedef DataPtr ReturnType;

	MeshUniformToVertex( const MeshPrimitive *mesh )	:	m_mesh( mesh )
	{
	}

	template<typename From> ReturnType operator()( const From* data )
	{
		typename From::Ptr result = static_cast< From* >( Object::create( data->typeId() ).get() );
		typename From::ValueType &trg = result->writable();
		const typename From::ValueType &src = data->readable();

		size_t numVerts = m_mesh->variableSize( PrimitiveVariable::Vertex );
		std::vector<int> count( numVerts, 0 );
		trg.resize( numVerts );

		typename From::ValueType::const_iterator srcIt = src.begin();
		std::vector<int>::const_iterator vId = m_mesh->vertexIds()->readable().begin();
		const std::vector<int> &verticesPerFace = m_mesh->verticesPerFace()->readable();
		for( std::vector<int>::const_iterator it = verticesPerFace.begin(); it < verticesPerFace.end(); ++it, ++srcIt )
		{
			for( int j = 0; j < *it; ++j, ++vId )
			{
				trg[ *vId ] += *srcIt;
				++count[ *vId ];
			}
		}

		std::vector<int>::const_iterator cIt = count.begin();
		typename From::ValueType::iterator trgIt = trg.begin(), trgEnd = trg.end();
		for( trgIt = trg.begin(); trgIt != trgEnd ; ++trgIt, ++cIt )
		{
			*trgIt /= *cIt;
		}

		return result;
	}

	const MeshPrimitive *m_mesh;
};

struct MeshFaceVaryingToVertex
{
	typedef DataPtr ReturnType;

	MeshFaceVaryingToVertex( const MeshPrimitive *mesh )	:	m_mesh( mesh )
	{
	}

	template<typename From> ReturnType operator()( const From* data )
	{
		typename From::Ptr result = static_cast< From* >( Object::create( data->typeId() ).get() );
		typename From::ValueType &trg = result->writable();
		const typename From::ValueType &src = data->readable();

		size_t numVerts = m_mesh->variableSize( PrimitiveVariable::Vertex );
		std::vector<int> count( numVerts, 0 );
		trg.resize( numVerts );

		const std::vector<int>& vertexIds = m_mesh->vertexIds()->readable();
		std::vector<int>::const_iterator vertexIdIt = vertexIds.begin();
		typename From::ValueType::const_iterator srcIt = src.begin(), srcEnd = src.end();

		for( ; srcIt != srcEnd ; ++srcIt, ++vertexIdIt )
		{
			trg[ *vertexIdIt ] += *srcIt;
			++count[ *vertexIdIt ];
		}

		std::vector<int>::const_iterator cIt = count.begin();
		typename From::ValueType::iterator trgIt = trg.begin(), trgEnd = trg.end();
		for( trgIt = trg.begin(); trgIt != trgEnd ; ++trgIt, ++cIt )
		{
			*trgIt /= *cIt;
		}

		return result;
	}

	const MeshPrimitive *m_mesh;
};

struct MeshFaceVaryingToUniform
{
	typedef DataPtr ReturnType;

	MeshFaceVaryingToUniform( const MeshPrimitive *mesh )	:	m_mesh( mesh )
	{
	}

	template<typename From> ReturnType operator()( const From* data )
	{
		typename From::Ptr result = static_cast< From* >( Object::create( data->typeId() ).get() );
		typename From::ValueType &trg = result->writable();
		const typename From::ValueType &src = data->readable();

		trg.reserve( m_mesh->numFaces() );

		typename From::ValueType::const_iterator srcIt = src.begin();

		const std::vector<int> &verticesPerFace = m_mesh->verticesPerFace()->readable();
		for( std::vector<int>::const_iterator it = verticesPerFace.begin(); it < verticesPerFace.end(); ++it )
		{
			// initialize with the first value to avoid
			// ambiguity during default construction
			typename From::ValueType::value_type total = *srcIt;
			++srcIt;

			for( int j = 1; j < *it; ++j, ++srcIt )
			{
				total += *srcIt;
			}

			trg.push_back( total / *it );
		}

		return result;
	}

	const MeshPrimitive *m_mesh;
};

struct MeshAnythingToFaceVarying
{
	typedef DataPtr ReturnType;

	MeshAnythingToFaceVarying( const MeshPrimitive *mesh, PrimitiveVariable::Interpolation srcInterpolation )
		: m_mesh( mesh ), m_srcInterpolation( srcInterpolation )
	{
	}

	template<typename From> ReturnType operator()( const From* data )
	{

		// TODO replace the call to the IECore::FaceVaryingPromotionOpPtr and include the logic in this file.

		// we need to duplicate because the Op expects a primvar to manipulate..
		IECore::MeshPrimitivePtr tmpMesh = m_mesh->copy();
		// cast OK due to read-only access.
		tmpMesh->variables["tmpPrimVar"] = IECore::PrimitiveVariable( m_srcInterpolation, const_cast< From * >(data) );
		IECore::FaceVaryingPromotionOpPtr promoteOp = new IECore::FaceVaryingPromotionOp();
		promoteOp->inputParameter()->setValue( tmpMesh );
		IECore::StringVectorDataPtr names = new StringVectorData();
		names->writable().push_back( "tmpPrimVar" );
		promoteOp->primVarNamesParameter()->setValue( names );
		ReturnType result = runTimeCast< MeshPrimitive >( promoteOp->operate() )->variables["tmpPrimVar"].data;


		return result;
	}

	const MeshPrimitive *m_mesh;
	PrimitiveVariable::Interpolation m_srcInterpolation;
};

class TexturePrimVarNames
{
	public:
		TexturePrimVarNames( const std::string &uvSetName );

		std::string uvSetName;

		std::string uName() const;
		std::string vName() const;

		std::string indicesName() const;
};

TexturePrimVarNames::TexturePrimVarNames( const std::string &uvSetName ) : uvSetName( uvSetName )
{
};

std::string TexturePrimVarNames::uName() const
{
	if( uvSetName == "st" )
	{
		return "s";
	}

	return uvSetName + "_s";
}

std::string TexturePrimVarNames::vName() const
{
	if( uvSetName == "st" )
	{
		return "t";
	}

	return uvSetName + "_t";
}

std::string TexturePrimVarNames::indicesName() const
{
	return uvSetName + "Indices";
}

struct Basis
{
	Basis()
	{
	}

	Basis( V3f normal, V3f uTangent, V3f vTangent ) : normal( normal ), uTangent( uTangent ), vTangent( vTangent )
	{
	}

	V3f normal;
	V3f uTangent;
	V3f vTangent;
};

struct CountFacesPerUniqueTangentBlock
{
	CountFacesPerUniqueTangentBlock(
		const std::vector<int> &vertIds,
		const std::vector<int> &stIndices,
		std::vector<tbb::atomic<int> > &totals
	) :
		m_totals(totals),
		m_vertIds(vertIds),
		m_stIndices(stIndices)
	{}

	void operator()( const tbb::blocked_range<std::size_t> &r ) const
	{
		for( size_t faceIndex = r.begin(); faceIndex < r.end(); faceIndex++ )
		{
			size_t fvi0 = faceIndex * 3;
			size_t fvi1 = fvi0 + 1;
			size_t fvi2 = fvi1 + 1;

			int i0 = m_stIndices[fvi0];
			int i1 = m_stIndices[fvi1];
			int i2 = m_stIndices[fvi2];

			m_totals[i0].fetch_and_increment();
			m_totals[i1].fetch_and_increment();
			m_totals[i2].fetch_and_increment();
		}
	}

	std::vector<tbb::atomic<int> > &m_totals;
	const std::vector<int> &m_vertIds;
	const std::vector<int> &m_stIndices;
};


struct CalculateFaceTangentBlock
{
	CalculateFaceTangentBlock(
		std::vector<Basis> &collector,
		std::vector<tbb::atomic<int> > &counts,
		std::vector<int> &offsets,
		const std::vector<int> &vertsPerFace,
		const std::vector<int> &vertIds,
		const std::vector<int> &stIndices,
		const std::vector<V3f> &points,
		const std::vector<float> &u,
		const std::vector<float> &v )
		: m_collector(collector),
		  m_counts(counts),
		  m_offsets(offsets),
		  m_vertsPerFace(vertsPerFace),
		  m_vertIds(vertIds),
		  m_stIndices(stIndices),
		  m_points( points ),
		  m_u( u ),
		  m_v( v )
	{
	}

	void operator()( const tbb::blocked_range<std::size_t> &r ) const
	{
		for( size_t faceIndex = r.begin(); faceIndex < r.end(); faceIndex++ )
		{
			assert( m_vertsPerFace[faceIndex] == 3 );

			// indices into the facevarying data for this face
			size_t fvi0 = faceIndex * 3;
			size_t fvi1 = fvi0 + 1;
			size_t fvi2 = fvi1 + 1;
			assert( fvi2 < m_vertIds.size() );
			assert( fvi2 < m_u.size() );
			assert( fvi2 < m_v.size() );

			// positions for each vertex of this face
			const V3f &p0 = m_points[m_vertIds[fvi0]];
			const V3f &p1 = m_points[m_vertIds[fvi1]];
			const V3f &p2 = m_points[m_vertIds[fvi2]];

			// uv coordinates for each vertex of this face
			const V2f uv0( m_u[fvi0], m_v[fvi0] );
			const V2f uv1( m_u[fvi1], m_v[fvi1] );
			const V2f uv2( m_u[fvi2], m_v[fvi2] );

			// compute tangents and normal for this face
			const V3f e0 = p1 - p0;
			const V3f e1 = p2 - p0;

			const V2f e0uv = uv1 - uv0;
			const V2f e1uv = uv2 - uv0;

			V3f tangent = ( e0 * -e1uv.y + e1 * e0uv.y ).normalized();
			V3f bitangent = ( e0 * -e1uv.x + e1 * e0uv.x ).normalized();

			V3f normal = ( p2 - p1 ).cross( p0 - p1 );
			normal.normalize();

			Basis basis(normal, tangent, bitangent);

			int i0 = m_stIndices[fvi0];
			int i1 = m_stIndices[fvi1];
			int i2 = m_stIndices[fvi2];

			int o0 = m_counts[i0].fetch_and_increment();
			int o1 = m_counts[i1].fetch_and_increment();
			int o2 = m_counts[i2].fetch_and_increment();

			// and accumlate them into the computation so far
			m_collector[m_offsets[i0] + o0] = basis;
			m_collector[m_offsets[i1] + o1] = basis;
			m_collector[m_offsets[i2] + o2] = basis;
		}

	}
	std::vector<Basis > &m_collector;
	std::vector<tbb::atomic<int> > &m_counts;
	const std::vector<int> &m_offsets;
	const std::vector<int> &m_vertsPerFace;
	const std::vector<int> &m_vertIds;
	const std::vector<int> &m_stIndices;
	const std::vector<V3f> &m_points;
	const std::vector<float> &m_u;
	const std::vector<float> &m_v;
};

struct CalculateVertexTangentBlock
{
	CalculateVertexTangentBlock(
		const std::vector<Basis> &collector,
		const std::vector<tbb::atomic<int> > &counts,
		const std::vector<int> &offsets,
		std::vector<V3f> &uTangents,
		std::vector<V3f> &vTangents,
		std::vector<V3f> &normals,
		bool orthoTangents
		)
	: m_collector(collector),
	  m_counts(counts),
	  m_offsets(offsets),
	  m_uTangents(uTangents),
	  m_vTangents(vTangents),
	  m_normals(normals),
	  m_orthoTangents(orthoTangents)

	{}

	void operator()( const tbb::blocked_range<std::size_t> &r ) const
	{
		// normalize and orthogonalize everything
		for( size_t i = r.begin(); i < r.end(); i++ )
		{
			for (size_t j = 0; j <(size_t) m_counts[i]; ++j)
			{
				m_normals[i] += m_collector[m_offsets[i] + j].normal;
				m_uTangents[i] += m_collector[m_offsets[i] + j].uTangent;
				m_vTangents[i] += m_collector[m_offsets[i] + j].vTangent;
			}

			m_normals[i].normalize();
			m_uTangents[i].normalize();
			m_vTangents[i].normalize();

			// Make uTangent/vTangent orthogonal to normal
			m_uTangents[i] -= m_normals[i] * m_uTangents[i].dot( m_normals[i] );
			m_vTangents[i] -= m_normals[i] * m_vTangents[i].dot( m_normals[i] );

			m_uTangents[i].normalize();
			m_vTangents[i].normalize();

			if( m_orthoTangents )
			{
				m_vTangents[i] -= m_uTangents[i] * m_vTangents[i].dot( m_uTangents[i] );
				m_vTangents[i].normalize();
			}

			// Ensure we have set of basis vectors (n, uT, vT) with the correct handedness.
			if( m_uTangents[i].cross( m_vTangents[i] ).dot( m_normals[i] ) < 0.0f )
			{
				m_uTangents[i] *= -1.0f;
			}
		}
	}

	const std::vector<Basis > &m_collector;
	const std::vector<tbb::atomic<int> > &m_counts;
	const std::vector<int> &m_offsets;

	std::vector<V3f> &m_uTangents;
	std::vector<V3f> &m_vTangents;
	std::vector<V3f> &m_normals;
	bool m_orthoTangents;
};


} // anonymous namespace

namespace IECore
{

namespace MeshAlgo
{

std::pair<PrimitiveVariable, PrimitiveVariable> calculateTangents(
	const MeshPrimitive *mesh,
	const std::string &uvSet, /* = "st" */
	bool orthoTangents, /* = true */
	const std::string &position /* = "P" */
)
{
	if( mesh->minVerticesPerFace() != 3 || mesh->maxVerticesPerFace() != 3 )
	{
		throw InvalidArgumentException( "MeshAlgo::calculateTangents : MeshPrimitive must only contain triangles" );
	}

	const TexturePrimVarNames texturePrimVarNames( uvSet );

	const V3fVectorData *positionData = mesh->variableData<V3fVectorData>( position );
	if( !positionData )
	{
		std::string e = boost::str( boost::format( "MeshAlgo::calculateTangents : MeshPrimitive has no Vertex \"%s\" primitive variable." ) % position );
		throw InvalidArgumentException( e );
	}

	const V3fVectorData::ValueType &points = positionData->readable();

	const IntVectorData *vertsPerFaceData = mesh->verticesPerFace();
	const IntVectorData::ValueType &vertsPerFace = vertsPerFaceData->readable();

	const IntVectorData *vertIdsData = mesh->vertexIds();
	const IntVectorData::ValueType &vertIds = vertIdsData->readable();

	const IntVectorData *stIndicesData = mesh->variableData<IntVectorData>( texturePrimVarNames.indicesName() );
	if( !stIndicesData )
	{
		// I'm a little unsure about using the vertIds for the stIndices.
		stIndicesData = vertIdsData;
	}
	const IntVectorData::ValueType &stIndices = stIndicesData->readable();

	ConstFloatVectorDataPtr uData = mesh->variableData<FloatVectorData>( texturePrimVarNames.uName(), PrimitiveVariable::FaceVarying );
	if( !uData )
	{
		throw InvalidArgumentException( ( boost::format( "MeshAlgo::calculateTangents : MeshPrimitive has no FaceVarying FloatVectorData primitive variable named \"%s\"."  ) % ( texturePrimVarNames.uName() ) ).str() );
	}

	ConstFloatVectorDataPtr vData = mesh->variableData<FloatVectorData>( texturePrimVarNames.vName(), PrimitiveVariable::FaceVarying );
	if( !vData )
	{
		throw InvalidArgumentException( ( boost::format( "MeshAlgo::calculateTangents : MeshPrimitive has no FaceVarying FloatVectorData primitive variable named \"%s\"."  ) % ( texturePrimVarNames.vName() ) ).str() );
	}

	const FloatVectorData::ValueType &u = uData->readable();
	const FloatVectorData::ValueType &v = vData->readable();


	// the uvIndices array is indexed as with any other facevarying data. the values in the
	// array specify the connectivity of the uvs - where two facevertices have the same index
	// they are known to be sharing a uv. for each one of these unique indices we compute
	// the tangents and normal, by accumulating all the tangents and normals for the faces
	// that reference them. we then take this data and shuffle it back into facevarying
	// primvars for the mesh.
	int numUniqueTangents = 1 + *std::max_element( stIndices.begin(), stIndices.end() );

	std::vector<V3f> uTangents( numUniqueTangents, V3f( 0 ) );
	std::vector<V3f> vTangents( numUniqueTangents, V3f( 0 ) );
	std::vector<V3f> normals( numUniqueTangents, V3f( 0 ) );

	std::vector<tbb::atomic<int> > totals( numUniqueTangents );
	CountFacesPerUniqueTangentBlock countFacesPerUniqueTangentBlock(vertIds, stIndices, totals);

	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, vertsPerFace.size()), countFacesPerUniqueTangentBlock);

	std::vector<int> uniqueTangentsOffsets(totals.size() + 1); // additional element so we always calculate number of vertices using offset[index + 1] - offset[index]
	IECore::Private::CalculateOffsetsFromCounts<tbb::atomic<int> > calculateOffsets(totals, uniqueTangentsOffsets);
	tbb::parallel_scan( tbb::blocked_range<size_t>(0, totals.size()), calculateOffsets );

	// calculate the final offset outside the tbb loop to reduce the need for branching in the loop body
	uniqueTangentsOffsets[uniqueTangentsOffsets.size()-1] = uniqueTangentsOffsets[uniqueTangentsOffsets.size() - 2] + totals[totals.size() - 1];

	std::vector<Basis> collector( calculateOffsets.m_acc );
	std::vector<tbb::atomic<int> > counts( numUniqueTangents );

	// calculate vertex tangents and a
	CalculateFaceTangentBlock calculateFaceTangentBlock(collector, counts, uniqueTangentsOffsets, vertsPerFace, vertIds, stIndices, points, u, v);
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, vertsPerFace.size()), calculateFaceTangentBlock);

	CalculateVertexTangentBlock calculateVertexTangentBlock(collector, counts, uniqueTangentsOffsets, uTangents, vTangents, normals, orthoTangents);
	tbb::parallel_for(tbb::blocked_range<std::size_t>(0, uTangents.size()), calculateVertexTangentBlock);

	// convert the tangents back to facevarying data and add that to the mesh
	V3fVectorDataPtr fvUD = new V3fVectorData();
	V3fVectorDataPtr fvVD = new V3fVectorData();

	std::vector<V3f> &fvU = fvUD->writable();
	std::vector<V3f> &fvV = fvVD->writable();
	fvU.resize( stIndices.size() );
	fvV.resize( stIndices.size() );

	for( unsigned i = 0; i < stIndices.size(); i++ )
	{
		fvU[i] = uTangents[stIndices[i]];
		fvV[i] = vTangents[stIndices[i]];
	}

	PrimitiveVariable tangentPrimVar( PrimitiveVariable::FaceVarying, fvUD );
	PrimitiveVariable bitangentPrimVar( PrimitiveVariable::FaceVarying, fvVD );

	return std::make_pair( tangentPrimVar, bitangentPrimVar );
}

void resamplePrimitiveVariable( const MeshPrimitive *mesh, PrimitiveVariable& primitiveVariable, PrimitiveVariable::Interpolation interpolation )
{
	Data *srcData = primitiveVariable.data.get();
	DataPtr dstData;

	PrimitiveVariable::Interpolation srcInterpolation = primitiveVariable.interpolation;

	if ( srcInterpolation == interpolation )
	{
		return;
	}

	// average array to single value
	if ( interpolation == PrimitiveVariable::Constant )
	{
		Detail::AverageValueFromVector fn;
		dstData = despatchTypedData<Detail::AverageValueFromVector, Detail::IsArithmeticVectorTypedData>( srcData, fn );
		primitiveVariable = PrimitiveVariable( interpolation, dstData );
		return;
	}

	if ( primitiveVariable.interpolation == PrimitiveVariable::Constant )
	{
		DataPtr arrayData = Detail::createArrayData(primitiveVariable, mesh, interpolation);
		if (arrayData)
		{
			primitiveVariable = PrimitiveVariable(interpolation, arrayData);
		}
		return;
	}

	if( interpolation == PrimitiveVariable::Uniform )
	{
		if( srcInterpolation == PrimitiveVariable::Varying || srcInterpolation == PrimitiveVariable::Vertex )
		{
			MeshVertexToUniform fn( mesh );
			dstData = despatchTypedData<MeshVertexToUniform, Detail::IsArithmeticVectorTypedData>( srcData, fn );
		}
		else if( srcInterpolation == PrimitiveVariable::FaceVarying )
		{
			MeshFaceVaryingToUniform fn( mesh );
			dstData = despatchTypedData<MeshFaceVaryingToUniform, Detail::IsArithmeticVectorTypedData>( srcData, fn );
		}
	}
	else if( interpolation == PrimitiveVariable::Varying || interpolation == PrimitiveVariable::Vertex )
	{
		if( srcInterpolation == PrimitiveVariable::Uniform )
		{
			MeshUniformToVertex fn( mesh );
			dstData = despatchTypedData<MeshUniformToVertex, Detail::IsArithmeticVectorTypedData>( srcData, fn );
		}
		else if( srcInterpolation == PrimitiveVariable::FaceVarying )
		{
			MeshFaceVaryingToVertex fn( mesh );
			dstData = despatchTypedData<MeshFaceVaryingToVertex, Detail::IsArithmeticVectorTypedData>( srcData, fn );
		}
		else if( srcInterpolation == PrimitiveVariable::Varying || srcInterpolation == PrimitiveVariable::Vertex )
		{
			dstData = srcData;
		}
	}
	else if( interpolation == PrimitiveVariable::FaceVarying )
	{
		MeshAnythingToFaceVarying fn( mesh, srcInterpolation );
		dstData = despatchTypedData<MeshAnythingToFaceVarying, Detail::IsArithmeticVectorTypedData>( srcData, fn );
	}

	primitiveVariable = PrimitiveVariable( interpolation, dstData );
}


MeshPrimitivePtr deleteFaces( const MeshPrimitive *meshPrimitive, const PrimitiveVariable& facesToDelete )
{

	if( facesToDelete.interpolation != PrimitiveVariable::Uniform )
	{
		throw InvalidArgumentException( "MeshAlgo::deleteFaces requires an Uniform [Int|Bool|Float]VectorData primitiveVariable " );
	}

	const IntVectorData *intDeleteFlagData = runTimeCast<const IntVectorData>( facesToDelete.data.get() );

	if( intDeleteFlagData )
	{
		return ::deleteFaces( meshPrimitive, intDeleteFlagData );
	}

	const BoolVectorData *boolDeleteFlagData = runTimeCast<const BoolVectorData>( facesToDelete.data.get() );

	if( boolDeleteFlagData )
	{
		return ::deleteFaces( meshPrimitive, boolDeleteFlagData );
	}

	const FloatVectorData *floatDeleteFlagData = runTimeCast<const FloatVectorData>( facesToDelete.data.get() );

	if( floatDeleteFlagData )
	{
		return ::deleteFaces( meshPrimitive, floatDeleteFlagData );
	}

	throw InvalidArgumentException( "MeshAlgo::deleteFaces requires an Uniform [Int|Bool|Float]VectorData primitiveVariable " );

}

} //namespace MeshAlgo
} //namespace IECore
