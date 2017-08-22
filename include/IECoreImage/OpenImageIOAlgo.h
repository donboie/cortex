//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2017, John Haddon. All rights reserved.
//  Copyright (c) 2017, Image Engine Design Inc. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//      * Redistributions of source code must retain the above
//        copyright notice, this list of conditions and the following
//        disclaimer.
//
//      * Redistributions in binary form must reproduce the above
//        copyright notice, this list of conditions and the following
//        disclaimer in the documentation and/or other materials provided with
//        the distribution.
//
//      * Neither the name of John Haddon nor the names of
//        any other contributors to this software may be used to endorse or
//        promote products derived from this software without specific prior
//        written permission.
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

#ifndef IECOREIMAGE_OIIOALGO_H
#define IECOREIMAGE_OIIOALGO_H

#include "OpenImageIO/color.h"
#include "OpenImageIO/imageio.h"
#include "OpenImageIO/paramlist.h"
#include "OpenImageIO/typedesc.h"

#include "IECore/GeometricTypedData.h"

namespace IECoreImage
{

namespace OpenImageIOAlgo
{

OIIO::TypeDesc::VECSEMANTICS vecSemantics( IECore::GeometricData::Interpretation interpretation );
IECore::GeometricData::Interpretation geometricInterpretation( OIIO::TypeDesc::VECSEMANTICS );

/// Returns a space separated string of supported file types
std::string extensions();

/// Returns the global ColorConfig for OpenImageIO
OIIO::ColorConfig *colorConfig();

/// Returns the expected colorspace of data based only
/// on the file format (e.g. "openexr", "jpeg", "tiff")
/// and the OIIO::ImageSpec. Note that this is a naive
/// approach to color management and should only be used
/// for rudimentary operations on "normally" encoded files.
std::string colorSpace( const std::string &fileFormat, const OIIO::ImageSpec &spec );

struct DataView
{

	DataView();
	/// If the data is StringData, `createUStrings` creates an
	/// `OIIO::ustring` and refers to the storage from that instead.
	DataView( const IECore::Data *data, bool createUStrings = false, bool copyData = false );
	DataView( const OIIO::ParamValue &param );

	OIIO::TypeDesc type;
	IECore::DataPtr data;
	const void *rawData;

	private :

		const char *m_charPointer;

};

} // namespace OpenImageIOAlgo

} // namespace IECoreImage

#endif // IECOREIMAGE_OIIOALGO_H
