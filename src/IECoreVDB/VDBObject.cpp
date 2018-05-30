//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2017, John Haddon. All rights reserved.
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

#include "IECoreVDB/VDBObject.h"

#include "IECore/Exception.h"
#include "IECore/MessageHandler.h"
#include "IECore/MurmurHash.h"
#include "IECore/SimpleTypedData.h"

#include "openvdb/io/Stream.h"
#include "openvdb/openvdb.h"

#include "boost/iostreams/categories.hpp"
#include "boost/iostreams/stream.hpp"

#include <algorithm>

using namespace IECore;
using namespace IECoreVDB;

namespace
{

//! Calculate the worldspace bounds - 0.5 padding required to include the full volume and not the bound of the voxel centres.
template<typename T>
Imath::Box<Imath::Vec3<T> > worldBound( const openvdb::GridBase *grid, float padding = 0.50f )
{
	openvdb::Vec3i min = grid->metaValue<openvdb::Vec3i>( grid->META_FILE_BBOX_MIN );
	openvdb::Vec3i max = grid->metaValue<openvdb::Vec3i>( grid->META_FILE_BBOX_MAX );

	openvdb::Vec3d offset = openvdb::Vec3d( padding );
	openvdb::BBoxd indexBounds = openvdb::BBoxd( min - offset, max + offset );
	openvdb::BBoxd worldBounds = grid->transform().indexToWorld( indexBounds );
	openvdb::Vec3d minBB = worldBounds.min();
	openvdb::Vec3d maxBB = worldBounds.max();

	return Imath::Box<Imath::Vec3<T> >( Imath::Vec3<T>( minBB[0], minBB[1], minBB[2] ), Imath::Vec3<T>( maxBB[0], maxBB[1], maxBB[2] ) );
}


//! allow hashing via a io stream interface.
struct MurmurHashSink
{
	typedef char char_type;
	typedef boost::iostreams::sink_tag category;

	MurmurHashSink(MurmurHash &hash)
	: hash(hash)
	{
	}

	std::streamsize write( const char *s, std::streamsize n )
	{
		hash.append( s, n );
		return n;
	}

	MurmurHash &hash;
};

}

IE_CORE_DEFINEOBJECTTYPEDESCRIPTION( VDBObject );

const unsigned int VDBObject::m_ioVersion = 0;

VDBObject::VDBObject() : m_unmodifiedFromFile( false )
{
}

VDBObject::VDBObject( const std::string &filename ) : m_unmodifiedFromFile( true )
{
	openvdb::initialize(); // safe to call multiple times but has a performance hit of a mutex.

	// note it seems fine for this file object to go out of scope
	// and grids are still able to pull in additional grid data
	m_file.reset( new openvdb::io::File( filename ) );
	// prevents a local tmp copy of the VDB for all file sizes
	// if this is not set then  small VDB files are copied locally before reading
	m_file->setCopyMaxBytes( 0 );
	m_file->open(); //lazy loading of grid data is default enabling OPENVDB_DISABLE_DELAYED_LOAD will load the grids up front

	for(auto it = m_file->beginName(); it != m_file->endName(); ++it )
	{
		openvdb::GridBase::Ptr grid = m_file->readGridMetadata( *it );

		std::string gridUniqueName(*it);
		std::string gridName = gridNames ? grid->getName() : gridUniqueName;

		m_grids[gridName] = HashedGrid( grid, m_file, &gridUniqueName );

		m_gridMask.push_back( *it );
	}
}

VDBObject::VDBObject( VDBObject& other, const std::vector<std::string> *gridNames )
{
	openvdb::initialize(); // safe to call multiple times but has a performance hit of a mutex.

	m_file = other.m_file;

	if ( !m_file )
	{
		return;
	}

	for(auto it = m_file->beginName(); it != m_file->endName(); ++it )
	{
		if (gridNames && std::find(gridNames->begin(), gridNames->end(), *it ) == gridNames->end())
		{
			continue;
		}

		openvdb::GridBase::Ptr grid = m_file->readGridMetadata( *it );

		std::string gridUniqueName(*it);
		std::string gridName = gridNames ? grid->getName() : gridUniqueName;

		m_grids[gridName] = HashedGrid( grid, m_file, &gridUniqueName );

		m_gridMask.push_back( *it );
	}
}

VDBObject::~VDBObject()
{
}

openvdb::GridBase::ConstPtr VDBObject::findGrid( const std::string &name ) const
{
	auto it = m_grids.find(name);
	if ( it != m_grids.end() )
	{
		return it->second.grid();
	}

	return openvdb::GridBase::Ptr();
}

openvdb::GridBase::Ptr VDBObject::findGrid( const std::string &name )
{

	auto it = m_grids.find( name );
	if ( it != m_grids.end() )
	{
		m_unmodifiedFromFile = false;
		it->second.markedAsEdited();
		return it->second.grid();
	}

	return openvdb::GridBase::Ptr();
}

std::vector<std::string> VDBObject::gridNames() const
{
	std::vector<std::string> outputGridNames;
	for( const auto &it : m_grids )
	{
		outputGridNames.push_back( it.first );
	}
	return outputGridNames;
}

void VDBObject::insertGrid( openvdb::GridBase::Ptr grid )
{
	m_unmodifiedFromFile = false;
	m_grids[grid->getName()] = HashedGrid( grid, nullptr );
}

void VDBObject::removeGrid( const std::string &name )
{
	auto it = m_grids.find( name );

	if ( it != m_grids.end() )
	{
		m_unmodifiedFromFile = false;
		m_grids.erase( it );
	}
}

Imath::Box3f VDBObject::bound() const
{
	Imath::Box3f combinedBounds;

	for( const auto &it : m_grids )
	{
		Imath::Box3f gridBounds = worldBound<float>( it.second.metadata().get() );

		if ( !gridBounds.hasVolume() )
		{
			continue;
		}

		combinedBounds.extendBy( gridBounds );
	}

	return combinedBounds;
}

void VDBObject::render( IECoreScene::Renderer *renderer ) const
{
}

IECore::CompoundObjectPtr VDBObject::metadata( const std::string &name )
{
	CompoundObjectPtr metadata = new CompoundObject();

	openvdb::GridBase::ConstPtr grid;

	if ( unmodifiedFromFile() )
	{

		auto it = m_grids.find( name );
		if ( it != m_grids.end() )
		{
			grid = it->second.metadata();
		}
	}
	else
	{
		openvdb::GridBase::Ptr tmpGrid  = findGrid ( name );
		if ( tmpGrid )
		{
			tmpGrid->addStatsMetadata();
		}
		grid = tmpGrid;
	}

	if( !grid )
	{
		return metadata;
	}

	for( auto metaIt = grid->beginMeta(); metaIt != grid->endMeta(); ++metaIt )
	{
		if( metaIt->second->typeName() == openvdb::StringMetadata::staticTypeName() )
		{
			metadata->members()[metaIt->first] = new StringData(
				static_cast<openvdb::StringMetadata *>( metaIt->second.get() )->value()
			);
		}
		else if( metaIt->second->typeName() == openvdb::Int64Metadata::staticTypeName() )
		{
			metadata->members()[metaIt->first] = new Int64Data(
				static_cast<openvdb::Int64Metadata *>( metaIt->second.get() )->value()
			);
		}
		else if( metaIt->second->typeName() == openvdb::Int32Metadata::staticTypeName() )
		{
			metadata->members()[metaIt->first] = new IntData(
				static_cast<openvdb::Int32Metadata *>( metaIt->second.get() )->value()
			);
		}
		else if( metaIt->second->typeName() == openvdb::BoolMetadata::staticTypeName() )
		{
			metadata->members()[metaIt->first] = new BoolData(
				static_cast<openvdb::BoolMetadata *>( metaIt->second.get() )->value()
			);
		}
		else if( metaIt->second->typeName() == openvdb::Vec3IMetadata::staticTypeName() )
		{
			const openvdb::Vec3i &v = static_cast<openvdb::Vec3IMetadata *>( metaIt->second.get() )->value();
			metadata->members()[metaIt->first] = new V3iData( Imath::V3i( v.x(), v.y(), v.z() ) );
		}
		else if( metaIt->second->typeName() == openvdb::Vec3SMetadata::staticTypeName() )
		{
			const openvdb::Vec3s &v = static_cast<openvdb::Vec3SMetadata *>( metaIt->second.get() )->value();
			metadata->members()[metaIt->first] = new V3fData( Imath::V3f( v.x(), v.y(), v.z() ) );
		}
		else
		{
			IECore::msg(
				IECore::MessageHandler::Warning,
				"VDBObject::metadata",
				boost::format( "'%1%' has unsupported metadata type: '%2%'" ) % metaIt->first % metaIt->second->typeName()
			);
		}
	}
	return metadata;
}

bool VDBObject::isEqualTo( const IECore::Object *other ) const
{
	if( !IECoreScene::VisibleRenderable::isNotEqualTo( other ) )
	{
		return false;
	}

	const VDBObject *vdbObject = runTimeCast<const VDBObject>( other );

	if( !vdbObject )
	{
		return false;
	}

	if (m_grids.size() != vdbObject->m_grids.size())
	{
		return false;
	}

	for (const auto& it : m_grids)
	{
		const auto itOther = vdbObject->m_grids.find( it.first );
		if ( itOther == vdbObject->m_grids.end() )
		{
			return false;
		}

		if (itOther->second.hash() != it.second.hash())
		{
			return false;
		}
	}

	return true;
}

void VDBObject::hash( IECore::MurmurHash &h ) const
{
	IECoreScene::VisibleRenderable::hash( h );

	if( unmodifiedFromFile() && m_file )
	{
		h.append( m_file->filename() );

		for (const auto gridName : m_gridMask)
		{
			h.append( gridName );
		}
		return;
	}

	for( const auto& it : m_grids )
	{
		h.append( it.second.hash() );
	}
}

void VDBObject::copyFrom( const IECore::Object *other, IECore::Object::CopyContext *context )
{
	IECoreScene::VisibleRenderable::copyFrom( other, context );

	const VDBObject *vdbObject = runTimeCast<const VDBObject>( other );

	if( !vdbObject )
	{
		return;
	}

	m_grids = vdbObject->m_grids;
	m_file = vdbObject->m_file;
	m_gridMask = vdbObject->m_gridMask;
	m_unmodifiedFromFile = vdbObject->m_unmodifiedFromFile;
}

void VDBObject::save( IECore::Object::SaveContext *context ) const
{
	IECoreScene::VisibleRenderable::save( context );
	throw IECore::NotImplementedException( "VDBObject::save" );
}

void VDBObject::load( IECore::Object::LoadContextPtr context )
{
	IECoreScene::VisibleRenderable::load( context );
	throw IECore::NotImplementedException( "VDBObject::load" );
}

void VDBObject::memoryUsage( IECore::Object::MemoryAccumulator &acc ) const
{
	IECoreScene::VisibleRenderable::memoryUsage( acc );

	for( const auto it : m_grids )
	{
		acc.accumulate( it.second.grid().get(), it.second.grid()->memUsage() );
	}
}

bool VDBObject::unmodifiedFromFile() const
{
	return m_unmodifiedFromFile;
}

std::string VDBObject::fileName() const
{
	if ( m_file )
	{
		return m_file->filename();
	}
	else
	{
		return std::string();
	}
}


openvdb::GridBase::Ptr VDBObject::HashedGrid::metadata() const
{
	return m_grid;
}

openvdb::GridBase::Ptr VDBObject::HashedGrid::grid() const
{
	if ( m_file )
	{
		m_grid = m_file->readGrid( !m_name.empty() ? m_name : m_grid->getName() );
		m_file.reset();
	}
	return m_grid;
}

IECore::MurmurHash VDBObject::HashedGrid::hash() const
{
	if( !m_hashValid )
	{
		m_hash = IECore::MurmurHash();

		MurmurHashSink sink( m_hash );
		boost::iostreams::stream<MurmurHashSink> hashStream( sink );

		openvdb::io::StreamMetadata::Ptr streamMetadata ( new openvdb::io::StreamMetadata() );
		openvdb::io::setStreamMetadataPtr( hashStream, streamMetadata );

		m_grid->writeMeta( hashStream );
		m_grid->writeTopology( hashStream );
		m_grid->writeBuffers( hashStream );
		m_grid->writeTransform( hashStream );

		m_hashValid = true;
	}

	return m_hash;
}

void VDBObject::HashedGrid::markedAsEdited()
{
	if( m_grid.use_count() > 1 )
	{
		m_grid = m_grid->deepCopyGrid();
		m_hash = IECore::MurmurHash();
		m_hashValid = false;
	}
}
