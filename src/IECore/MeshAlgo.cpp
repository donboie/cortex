#include <assert.h>
#include <algorithm>

#include "OpenEXR/ImathVec.h"

#include "IECore/DespatchTypedData.h"
#include "IECore/MeshAlgo.h"

using namespace IECore;
using namespace Imath;

namespace
{

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

} // anonymous namespace

namespace IECore
{

namespace MeshAlgo
{

std::pair<PrimitiveVariable, PrimitiveVariable> calculateTangents(
	const MeshPrimitive *mesh, const std::string &uvSet, /* = "st" */
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
		throw InvalidArgumentException(
			(
				boost::format( "MeshAlgo::calculateTangents : MeshPrimitive has no FaceVarying FloatVectorData primitive variable named \"%s\"." ) %
					( texturePrimVarNames.uName() )
			).str()
		);
	}

	ConstFloatVectorDataPtr vData = mesh->variableData<FloatVectorData>( texturePrimVarNames.vName(), PrimitiveVariable::FaceVarying );
	if( !vData )
	{
		throw InvalidArgumentException(
			(
				boost::format( "MeshAlgo::calculateTangents : MeshPrimitive has no FaceVarying FloatVectorData primitive variable named \"%s\"." ) %
					( texturePrimVarNames.vName() )
			).str()
		);
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

	for( size_t faceIndex = 0; faceIndex < vertsPerFace.size(); faceIndex++ )
	{
		assert( vertsPerFace[faceIndex] == 3 );

		// indices into the facevarying data for this face
		size_t fvi0 = faceIndex * 3;
		size_t fvi1 = fvi0 + 1;
		size_t fvi2 = fvi1 + 1;
		assert( fvi2 < vertIds.size() );
		assert( fvi2 < u.size() );
		assert( fvi2 < v.size() );

		// positions for each vertex of this face
		const V3f &p0 = points[vertIds[fvi0]];
		const V3f &p1 = points[vertIds[fvi1]];
		const V3f &p2 = points[vertIds[fvi2]];

		// uv coordinates for each vertex of this face
		const V2f uv0( u[fvi0], v[fvi0] );
		const V2f uv1( u[fvi1], v[fvi1] );
		const V2f uv2( u[fvi2], v[fvi2] );

		// compute tangents and normal for this face
		const V3f e0 = p1 - p0;
		const V3f e1 = p2 - p0;

		const V2f e0uv = uv1 - uv0;
		const V2f e1uv = uv2 - uv0;

		V3f tangent = ( e0 * -e1uv.y + e1 * e0uv.y ).normalized();
		V3f bitangent = ( e0 * -e1uv.x + e1 * e0uv.x ).normalized();

		V3f normal = ( p2 - p1 ).cross( p0 - p1 );
		normal.normalize();

		// and accumlate them into the computation so far
		uTangents[stIndices[fvi0]] += tangent;
		uTangents[stIndices[fvi1]] += tangent;
		uTangents[stIndices[fvi2]] += tangent;

		vTangents[stIndices[fvi0]] += bitangent;
		vTangents[stIndices[fvi1]] += bitangent;
		vTangents[stIndices[fvi2]] += bitangent;

		normals[stIndices[fvi0]] += normal;
		normals[stIndices[fvi1]] += normal;
		normals[stIndices[fvi2]] += normal;

	}

	// normalize and orthogonalize everything
	for( size_t i = 0; i < uTangents.size(); i++ )
	{
		normals[i].normalize();

		uTangents[i].normalize();
		vTangents[i].normalize();

		// Make uTangent/vTangent orthogonal to normal
		uTangents[i] -= normals[i] * uTangents[i].dot( normals[i] );
		vTangents[i] -= normals[i] * vTangents[i].dot( normals[i] );

		uTangents[i].normalize();
		vTangents[i].normalize();

		if( orthoTangents )
		{
			vTangents[i] -= uTangents[i] * vTangents[i].dot( uTangents[i] );
			vTangents[i].normalize();
		}

		// Ensure we have set of basis vectors (n, uT, vT) with the correct handedness.
		if( uTangents[i].cross( vTangents[i] ).dot( normals[i] ) < 0.0f )
		{
			uTangents[i] *= -1.0f;
		}
	}

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

namespace
{

class DeleteFlaggedUniformFunctor
{
	public:
		typedef DataPtr ReturnType;

		DeleteFlaggedUniformFunctor( ConstIntVectorDataPtr flagData ) : m_flagData( flagData )
		{
		}

		template<typename T>
		ReturnType operator()( const T *data )
		{
			const typename T::ValueType &inputs = data->readable();
			const std::vector<int> &flags = m_flagData->readable();

			T *filteredResultData = new T();
			typename T::ValueType &filteredResult = filteredResultData->writable();

			filteredResult.reserve( inputs.size() );

			for( size_t i = 0; i < inputs.size(); ++i )
			{
				if( !flags[i] )
				{
					filteredResult.push_back( inputs[i] );
				}
			}

			return filteredResultData;
		}

	private:
		ConstIntVectorDataPtr m_flagData;
};

class DeleteFlaggedFaceVaryingFunctor
{
	public:
		typedef DataPtr ReturnType;

		DeleteFlaggedFaceVaryingFunctor( ConstIntVectorDataPtr flagData, ConstIntVectorDataPtr verticesPerFaceData ) : m_flagData( flagData ), m_verticesPerFaceData( verticesPerFaceData )
		{
		}

		template<typename T>
		ReturnType operator()( const T *data )
		{
			const typename T::ValueType &inputs = data->readable();
			const std::vector<int> &verticesPerFace = m_verticesPerFaceData->readable();
			const std::vector<int> &flags = m_flagData->readable();

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
		ConstIntVectorDataPtr m_flagData;
		ConstIntVectorDataPtr m_verticesPerFaceData;

};

class DeleteFlaggedVertexFunctor
{
	public:
		typedef DataPtr ReturnType;

		DeleteFlaggedVertexFunctor( size_t maxVertexId, ConstIntVectorDataPtr vertexIdsData, ConstIntVectorDataPtr verticesPerFaceData, ConstIntVectorDataPtr flagData )
			: m_flagData( flagData ), m_verticesPerFaceData( verticesPerFaceData ), m_vertexIdsData( vertexIdsData )
		{
			const std::vector<int> &vertexIds = m_vertexIdsData->readable();
			const std::vector<int> &verticesPerFace = m_verticesPerFaceData->readable();
			const std::vector<int> &flags = m_flagData->readable();

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
		ConstIntVectorDataPtr m_flagData;
		ConstIntVectorDataPtr m_verticesPerFaceData;
		ConstIntVectorDataPtr m_vertexIdsData;

		BoolVectorDataPtr m_usedVerticesData;

		// map from old vertex index to new
		IntVectorDataPtr m_remappingData;
};

}



MeshPrimitivePtr deleteFaces( const MeshPrimitive *meshPrimitive, const std::string &primvar )
{
	const IntVectorData *deleteFlagData = meshPrimitive->variableData<IntVectorData>( primvar, PrimitiveVariable::Uniform );

	if (!deleteFlagData)
	{
		throw InvalidArgumentException(
			(
				boost::format( "MeshAlgo::deleteFaces : MeshPrimitive has no Uniform IntVectorData primitive variable named \"%s\"." ) %
					( primvar )
			).str()
		);
	}

	// construct 3 functors for deleting (uniform, vertex & face varying) primvars
	DeleteFlaggedUniformFunctor uniformFunctor(deleteFlagData);
	DeleteFlaggedFaceVaryingFunctor faceVaryingFunctor(deleteFlagData, meshPrimitive->verticesPerFace());
	DeleteFlaggedVertexFunctor vertexFunctor( meshPrimitive->variableSize(PrimitiveVariable::Vertex), meshPrimitive->vertexIds(), meshPrimitive->verticesPerFace(), deleteFlagData );

	// filter verticesPerFace using DeleteFlaggedUniformFunctor
	IECore::Data *inputVerticesPerFace = const_cast< IECore::Data * >( IECore::runTimeCast<const IECore::Data>( meshPrimitive->verticesPerFace() ) );
	IECore::DataPtr outputVerticesPerFace = despatchTypedData<DeleteFlaggedUniformFunctor, TypeTraits::IsVectorTypedData>( inputVerticesPerFace, uniformFunctor );
	IntVectorDataPtr verticesPerFace = IECore::runTimeCast<IECore::IntVectorData>(outputVerticesPerFace);

	// filter VertexIds using DeleteFlaggedFaceVaryingFunctor
	IECore::Data *inputVertexIds = const_cast< IECore::Data * >( IECore::runTimeCast<const IECore::Data>( meshPrimitive->vertexIds() ) );
	IECore::DataPtr outputVertexIds = despatchTypedData<DeleteFlaggedFaceVaryingFunctor, TypeTraits::IsVectorTypedData>( inputVertexIds, faceVaryingFunctor );

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
				IECore::DataPtr outputData = despatchTypedData<DeleteFlaggedUniformFunctor, TypeTraits::IsVectorTypedData>( inputData, uniformFunctor );
				outMeshPrimitive->variables[it->first] = PrimitiveVariable( it->second.interpolation, outputData );
			}
			break;
		case PrimitiveVariable::Vertex:
		case PrimitiveVariable::Varying:
			{
				IECore::Data *inputData = const_cast< IECore::Data * >( it->second.data.get() );
				IECore::DataPtr ouptputData = despatchTypedData<DeleteFlaggedVertexFunctor, TypeTraits::IsVectorTypedData>( inputData, vertexFunctor );
				outMeshPrimitive->variables[it->first] = PrimitiveVariable( it->second.interpolation, ouptputData );
			}
			break;
		case PrimitiveVariable::FaceVarying:
			{
				IECore::Data *inputData = const_cast< IECore::Data * >( it->second.data.get() );
				IECore::DataPtr outputData = despatchTypedData<DeleteFlaggedFaceVaryingFunctor, TypeTraits::IsVectorTypedData>( inputData, faceVaryingFunctor );
				outMeshPrimitive->variables[it->first] = PrimitiveVariable ( it->second.interpolation, outputData);
			}
			break;
		case PrimitiveVariable::Constant:
		case PrimitiveVariable::Invalid:
			outMeshPrimitive->variables[it->first] = it->second;
			break;

		}
	}
	return outMeshPrimitive;
}

} //namespace MeshAlgo
} //namespace IECore
