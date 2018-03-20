##########################################################################
#
#  Copyright (c) 2007-2011, Image Engine Design Inc. All rights reserved.
#
#  Copyright (c) 2010, John Haddon. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#
#     * Neither the name of Image Engine Design nor the names of any
#       other contributors to this software may be used to endorse or
#       promote products derived from this software without specific prior
#       written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
##########################################################################

## \defgroup python Python functionality
#
# Some parts of the IECore library are defined purely in Python. These are shown below.

# Turn on RTLD_GLOBAL to avoid the dreaded cross module RTTI
# errors on Linux. This causes libIECore etc to be loaded into
# the global symbol table and those symbols to be shared between
# modules. Without it, different python modules and/or libraries
# can end up with their own copies of symbols, which breaks a
# great many things.
#
# We use the awkward "__import__" approach to avoid importing sys
# and ctypes into the IECore namespace.

__import__( "sys" ).setdlopenflags(
	__import__( "sys" ).getdlopenflags() | __import__( "ctypes" ).RTLD_GLOBAL
)

__import__( "imath" )

from _IECore import *

# access by a shorter name for convenience
Msg = MessageHandler
from registerRunTimeTyped import registerRunTimeTyped
from registerObject import registerObject
from Log import *
from Formatter import Formatter
from WrappedTextFormatter import WrappedTextFormatter
import StringUtil
from DataTraits import *
from FileSequenceFunctions import *
from ClassLoader import ClassLoader
from SequenceCpOp import SequenceCpOp
from SequenceLsOp import SequenceLsOp
from SequenceMvOp import SequenceMvOp
from SequenceRmOp import SequenceRmOp
from SequenceCatOp import SequenceCatOp
from SequenceRenumberOp import SequenceRenumberOp
from SequenceConvertOp import SequenceConvertOp
from FormattedParameterHelp import formatParameterHelp
from ClassLsOp import ClassLsOp
from OptionalCompoundParameter import OptionalCompoundParameter
from Struct import Struct
import Enum
from LsHeaderOp import LsHeaderOp
from curry import curry
from MenuItemDefinition import MenuItemDefinition
from MenuDefinition import MenuDefinition
from ParameterParser import ParameterParser
from SearchReplaceOp import SearchReplaceOp
from CapturingMessageHandler import CapturingMessageHandler
from LayeredDict import LayeredDict
from CompoundVectorParameter import CompoundVectorParameter
from SequenceMergeOp import SequenceMergeOp
from DateTimeParameterParser import *
from SubstitutedDict import SubstitutedDict
from ClassParameter import ClassParameter
from ClassVectorParameter import ClassVectorParameter
from CompoundStream import CompoundStream
from IgnoredExceptions import IgnoredExceptions
import ParameterAlgo

# importing internal utility modules and class overwrites
from ObjectOverwriting import *
from OpOverwriting import *
from ParameterOverwriting import *
from ParameterisedOverwriting import *
from MessageHandlerOverwriting import *

from ConfigLoader import loadConfig
from Preset import Preset
from BasicPreset import BasicPreset
from RelativePreset import RelativePreset
from TestCase import TestCase

loadConfig( "CORTEX_STARTUP_PATHS", subdirectory = "IECore" )
