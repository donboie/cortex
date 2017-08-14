//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2008-2013, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of Image Engine Design nor the names of any
//       other contributors to this software may be used to endorse or
//       promote products derived from this software without specific prior
//       written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////

#include "boost/format.hpp"

#include "tbb/tbb.h"

#include "IECore/MeshNormalsOp.h"
#include "IECore/DespatchTypedData.h"
#include "IECore/CompoundParameter.h"
#include "IECore/private/CountToOffset.h"

using namespace IECore;
using namespace std;

IE_CORE_DEFINERUNTIMETYPED( MeshNormalsOp );

namespace
{


//! calculate per face normal
template<typename VectorType>
struct CalculateFaceNormals
{
	CalculateFaceNormals(
		const vector<int> &vertsPerFace,
		const vector<int> &vertexOffsets,
		const vector<int> &vertIds,
		const vector<VectorType> &points,
		vector<VectorType> &outputNormals)
		: m_vertsPerFace(vertsPerFace),
		m_vertexOffsets(vertexOffsets),
		m_vertIds(vertIds),
		m_points(points),
		m_outputNormals(outputNormals)
	{
	}

	void operator()( const tbb::blocked_range<std::size_t>& r ) const
	{
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			// calculate the face normal. note that this method is very naive, and doesn't
			// cope with colinear vertices or concave faces - we could use polygonNormal() from
			// PolygonAlgo.h to deal with that, but currently we'd prefer to avoid the overhead.
			const VectorType &p0 = m_points[m_vertIds[m_vertexOffsets[i]]];
			const VectorType &p1 = m_points[m_vertIds[m_vertexOffsets[i] + 1]];
			const VectorType &p2 = m_points[m_vertIds[m_vertexOffsets[i] + 2]];

			VectorType normal = (p2-p1).cross(p0-p1);
			normal.normalize();

			m_outputNormals[i] = normal;
		}
	}

	const vector<int> &m_vertsPerFace;
	const vector<int> &m_vertexOffsets;
	const vector<int> &m_vertIds;
	const std::vector<VectorType> &m_points;

	std::vector<VectorType> &m_outputNormals;
};

struct CountFacesPerVertexBlock
{
	CountFacesPerVertexBlock(
		const std::vector<int>& vertexOffsets,
		const vector<int> &vertIds,
		std::vector<tbb::atomic<int> >& counts
	)
		: m_vertexOffsets(vertexOffsets),
		m_vertIds(vertIds),
		m_counts(counts)
	{
	}

	void operator()( const tbb::blocked_range<std::size_t>& r ) const
	{
		for( size_t f = r.begin(); f != r.end(); ++f )
		{
			for (size_t v = 0; v < (size_t) (m_vertexOffsets[f + 1] - m_vertexOffsets[f]); ++v)
			{
				int vertexIndex = m_vertIds[m_vertexOffsets[f] + v];
				m_counts[vertexIndex].fetch_and_increment();
			}
		}
	}

	const vector<int> &m_vertexOffsets;
	const vector<int> &m_vertIds;

	std::vector<tbb::atomic<int> > &m_counts;
};

struct ConnectedFacesByVertexBlock
{
	ConnectedFacesByVertexBlock(
		const std::vector<int>& vertexOffsets,
		const vector<int> &vertIds,
		std::vector<int>& facesByVertex,
		std::vector<tbb::atomic<int> >& counts,
		const std::vector<int> &offsets)
		: m_vertexOffsets(vertexOffsets),
		m_vertIds(vertIds),
		m_facesByVertex(facesByVertex),
		m_counts(counts),
		m_offsets(offsets)
	{}

	void operator()( const tbb::blocked_range<std::size_t>& r ) const
	{
		for (size_t f = r.begin(); f != r.end(); ++f)
		{
			for (size_t v = 0; v < (size_t) (m_vertexOffsets[f + 1] - m_vertexOffsets[f]); ++v)
			{
				int vertexIndex = m_vertIds[m_vertexOffsets[f] + v];
				int index = m_counts[vertexIndex].fetch_and_increment();

				m_facesByVertex[m_offsets[vertexIndex] + index] = f;
			}
		}
	}

	const vector<int> &m_vertexOffsets;
	const vector<int> &m_vertIds;

	std::vector<int> &m_facesByVertex;
	std::vector<tbb::atomic<int> > &m_counts;
	const std::vector<int> &m_offsets;
};

template<typename VectorType>
struct AverageFaceNormalsBlock
{
	AverageFaceNormalsBlock(
		const std::vector<int> &facesByVertex,
		const std::vector<tbb::atomic<int> > &counts,
		const std::vector<int> &offsets,
		const std::vector<VectorType>& faceNormals,
		std::vector<VectorType>& vertexNormals)
		: m_facesByVertex(facesByVertex),
		m_counts(counts),
		m_offsets(offsets),
		m_faceNormals(faceNormals),
		m_vertexNormals(vertexNormals)
	{}

	void operator()( const tbb::blocked_range<std::size_t>& r ) const
	{
		for (size_t v = r.begin(); v != r.end(); ++v)
		{
			m_vertexNormals[v] = VectorType(0);
			for (size_t f = 0; f < m_counts[v]; ++f )
			{
				m_vertexNormals[v] += m_faceNormals[m_facesByVertex[m_offsets[v] + f]];
			}

			m_vertexNormals[v].normalize();
		}
	}

	const std::vector<int >& m_facesByVertex;
	const std::vector<tbb::atomic<int> > &m_counts;
	const std::vector<int> &m_offsets;

	const std::vector<VectorType>& m_faceNormals;
	std::vector<VectorType>& m_vertexNormals;
};

} // namespace

MeshNormalsOp::MeshNormalsOp() : MeshPrimitiveOp( "Calculates vertex normals for a mesh." )
{
	/// \todo Add this parameter to a member variable and update pPrimVarNameParameter() functions.
	StringParameterPtr pPrimVarNameParameter = new StringParameter(
		"pPrimVarName",
		"Input primitive variable name.",
		"P"
	);

	/// \todo Add this parameter to a member variable and update nPrimVarNameParameter() functions.
	StringParameterPtr nPrimVarNameParameter = new StringParameter(
		"nPrimVarName",
		"Output primitive variable name.",
		"N"
	);

	IntParameter::PresetsContainer interpolationPresets;
	interpolationPresets.push_back( IntParameter::Preset( "Vertex", PrimitiveVariable::Vertex ) );
	interpolationPresets.push_back( IntParameter::Preset( "Uniform", PrimitiveVariable::Uniform ) );
	IntParameterPtr interpolationParameter;
	interpolationParameter = new IntParameter(
		"interpolation",
		"The primitive variable interpolation type for the calculated normals.",
		PrimitiveVariable::Vertex,
		interpolationPresets
	);

	parameters()->addParameter( pPrimVarNameParameter );
	parameters()->addParameter( nPrimVarNameParameter );
	parameters()->addParameter( interpolationParameter );
}

MeshNormalsOp::~MeshNormalsOp()
{
}

StringParameter * MeshNormalsOp::pPrimVarNameParameter()
{
	return parameters()->parameter<StringParameter>( "pPrimVarName" );
}

const StringParameter * MeshNormalsOp::pPrimVarNameParameter() const
{
	return parameters()->parameter<StringParameter>( "pPrimVarName" );
}

StringParameter * MeshNormalsOp::nPrimVarNameParameter()
{
	return parameters()->parameter<StringParameter>( "nPrimVarName" );
}

const StringParameter * MeshNormalsOp::nPrimVarNameParameter() const
{
	return parameters()->parameter<StringParameter>( "nPrimVarName" );
}

IntParameter * MeshNormalsOp::interpolationParameter()
{
	return parameters()->parameter<IntParameter>( "interpolation" );
}

const IntParameter * MeshNormalsOp::interpolationParameter() const
{
	return parameters()->parameter<IntParameter>( "interpolation" );
}

struct MeshNormalsOp::CalculateNormals
{
	typedef DataPtr ReturnType;

	CalculateNormals( const IntVectorData *vertsPerFace, const IntVectorData *vertIds, PrimitiveVariable::Interpolation interpolation )
		:	m_vertsPerFace( vertsPerFace ), m_vertIds( vertIds ), m_interpolation( interpolation )
	{
	}

	template<typename T>
	ReturnType operator()( T * data )
	{
		typedef typename T::ValueType VecContainer;
		typedef typename VecContainer::value_type Vec;

		const typename T::ValueType &points = data->readable();
		const vector<int> &vertsPerFace = m_vertsPerFace->readable();
		const vector<int> &vertIds = m_vertIds->readable();

		typename T::Ptr normalsData = new T;
		normalsData->setInterpretation( GeometricData::Normal );
		VecContainer &normals = normalsData->writable();
		if( m_interpolation == PrimitiveVariable::Uniform )
		{
			normals.reserve( vertsPerFace.size() );
		}
		else
		{
			normals.resize( points.size(), Vec( 0 ) );
		}

		// 1) calculate vertex offsets for parallel gather of vertex normals
		std::vector<int> offsets(vertsPerFace.size() + 1); // additional element so we always calculate number of vertices using offset[index + 1] - offset[index]
		IECore::Private::CalculateOffsetsFromCounts<int> calculateVertexOffsets(vertsPerFace, offsets);
		tbb::parallel_scan( tbb::blocked_range<size_t>(0, vertsPerFace.size()), calculateVertexOffsets );

		// calculate the final offset outside the tbb loop to reduce the need for branching in the loop body
		offsets[offsets.size()-1] = offsets[offsets.size() - 2] + vertsPerFace[vertsPerFace.size() - 1];

		// 2) calculate face normals
		std::vector<Vec> faceNormals( vertsPerFace.size() );
		tbb::parallel_for( tbb::blocked_range<size_t>(0, vertsPerFace.size()), CalculateFaceNormals<Vec>(vertsPerFace, offsets, vertIds, points, faceNormals));

		if (m_interpolation == PrimitiveVariable::Vertex)
		{
			std::vector<tbb::atomic<int> > totals( points.size() );
			CountFacesPerVertexBlock countFacesPerVertexBlock(offsets, vertIds, totals);
			tbb::parallel_for( tbb::blocked_range<size_t>(0, vertsPerFace.size()), countFacesPerVertexBlock);

			std::vector<int> vertexOffsets(totals.size() + 1); // additional element so we always calculate number of vertices using offset[index + 1] - offset[index]
			IECore::Private::CalculateOffsetsFromCounts<tbb::atomic<int> > calculateOffsets(totals, vertexOffsets);
			tbb::parallel_scan( tbb::blocked_range<size_t>(0, totals.size()), calculateOffsets );

			// calculate the final offset outside the tbb loop to reduce the need for branching in the loop body
			vertexOffsets[vertexOffsets.size()-1] = vertexOffsets[vertexOffsets.size() - 2] + totals[totals.size() - 1];

			std::vector<int> facesByVertex( calculateOffsets.m_acc );
			std::vector<tbb::atomic<int> > counts( points.size() );

			ConnectedFacesByVertexBlock connectedFacesByVertexBlock(offsets, vertIds, facesByVertex, counts, vertexOffsets);
			tbb::parallel_for( tbb::blocked_range<size_t>(0, vertsPerFace.size()), connectedFacesByVertexBlock);

			AverageFaceNormalsBlock<Vec> averageFaceNormalsBlock(facesByVertex, counts, vertexOffsets, faceNormals, normals );
			tbb::parallel_for( tbb::blocked_range<size_t>(0, points.size()), averageFaceNormalsBlock);
		}
		else if (m_interpolation == PrimitiveVariable::Uniform)
		{
			normals.swap( faceNormals );
		}

		return normalsData;
	}

	private :

		ConstIntVectorDataPtr m_vertsPerFace;
		ConstIntVectorDataPtr m_vertIds;
		PrimitiveVariable::Interpolation m_interpolation;

};

struct MeshNormalsOp::HandleErrors
{
	template<typename T, typename F>
	void operator()( const T *d, const F &f )
	{
		string e = boost::str( boost::format( "MeshNormalsOp : pPrimVarName parameter has unsupported data type \"%s\"." ) % d->typeName() );
		throw InvalidArgumentException( e );
	}
};

void MeshNormalsOp::modifyTypedPrimitive( MeshPrimitive * mesh, const CompoundObject * operands )
{
	const std::string &pPrimVarName = pPrimVarNameParameter()->getTypedValue();
	PrimitiveVariableMap::const_iterator pvIt = mesh->variables.find( pPrimVarName );
	if( pvIt==mesh->variables.end() || !pvIt->second.data )
	{
		string e = boost::str( boost::format( "MeshNormalsOp : MeshPrimitive has no \"%s\" primitive variable." ) % pPrimVarName );
		throw InvalidArgumentException( e );
	}

	if( !mesh->isPrimitiveVariableValid( pvIt->second ) )
	{
		string e = boost::str( boost::format( "MeshNormalsOp : \"%s\" primitive variable is invalid." ) % pPrimVarName );
		throw InvalidArgumentException( e );
	}

	const PrimitiveVariable::Interpolation interpolation = static_cast<PrimitiveVariable::Interpolation>( operands->member<IntData>( "interpolation" )->readable() );

	CalculateNormals f( mesh->verticesPerFace(), mesh->vertexIds(), interpolation );
	DataPtr n = despatchTypedData<CalculateNormals, TypeTraits::IsVec3VectorTypedData, HandleErrors>( pvIt->second.data.get(), f );

	mesh->variables[ nPrimVarNameParameter()->getTypedValue() ] = PrimitiveVariable( interpolation, n );
}
