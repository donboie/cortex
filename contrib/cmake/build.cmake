project (Cortex)

include(contrib/cmake/util.cmake)
include(contrib/cmake/globals.cmake)
include(contrib/cmake/deps.cmake)
include(contrib/cmake/makers.cmake)

#=========================================
# Core
#=========================================

standardSharedLibrary(
					LIBRARY_NAME
						IECore
					THIRD_PARTY_INCLUDE_DIRS
						include
						${BOOST_INCLUDE_DIR}
						${TBB_INCLUDE_DIR}
						${OPENEXR_INCLUDE_DIR}
						${FREETYPE_INCLUDE_DIR}
						${PNG_INCLUDE_DIR}
					THIRD_PARTY_LIBRARIES
						Imath
						IlmThread
						IlmImfUtil
						IlmImf
						Iex
						IexMath
						tbb
						tbbmalloc
						freetype
						boost_regex-gcc48-1_55
						boost_filesystem-gcc48-1_55
						boost_iostreams-gcc48-1_55
						boost_date_time-gcc48-1_55
						boost_python-gcc48-1_55
						boost_system-gcc48-1_55
						png
						tiff
					THIRD_PARTY_LIBRARY_DIRS
						${BOOST_LIBRARY_DIR}
						${TBB_LIBRARY_DIR}
						${OPENEXR_LIBRARY_DIR}
						)

standardSharedLibrary(
					LIBRARY_NAME
						IECorePython
					THIRD_PARTY_INCLUDE_DIRS
						include
						${BOOST_INCLUDE_DIR}
						${TBB_INCLUDE_DIR}
						${OPENEXR_INCLUDE_DIR}
						${GLEW_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
					THIRD_PARTY_LIBRARIES
						Imath
						IlmThread
						IlmImfUtil
						IlmImf
						Iex
						IexMath
					DEPENDENCIES
						IECore
					THIRD_PARTY_LIBRARY_DIRS
						${BOOST_LIBRARY_DIR}
						${TBB_LIBRARY_DIR}
						${OPENEXR_LIBRARY_DIR}
						)

standardSharedLibrary(
					LIBRARY_NAME
						IECorePythonModule
					THIRD_PARTY_INCLUDE_DIRS
						include
						${BOOST_INCLUDE_DIR}
						${TBB_INCLUDE_DIR}
						${OPENEXR_INCLUDE_DIR}
						${GLEW_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
					THIRD_PARTY_LIBRARIES
						Imath
						IlmThread
						IlmImfUtil
						IlmImf
						Iex
						IexMath
					DEPENDENCIES
						IECorePython
					THIRD_PARTY_LIBRARY_DIRS
						${BOOST_LIBRARY_DIR}
						${TBB_LIBRARY_DIR}
						${OPENEXR_LIBRARY_DIR}
					)

#=========================================
# OpenGL
#=========================================

standardSharedLibrary(
					LIBRARY_NAME
						IECoreGL
					THIRD_PARTY_INCLUDE_DIRS
						include
						${BOOST_INCLUDE_DIR}
						${TBB_INCLUDE_DIR}
						${OPENEXR_INCLUDE_DIR}
						${GLEW_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
					THIRD_PARTY_LIBRARIES
						Imath
						IlmThread
						IlmImfUtil
						IlmImf
						Iex
						IexMath
					THIRD_PARTY_LIBRARY_DIRS
						${BOOST_LIBRARY_DIR}
						${TBB_LIBRARY_DIR}
						${OPENEXR_LIBRARY_DIR}
						)

standardSharedLibrary(
					LIBRARY_NAME
						IECoreGL
					BINDINGS
					THIRD_PARTY_INCLUDE_DIRS
						include
						${BOOST_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
					DEPENDENCIES
						IECoreGL
					THIRD_PARTY_LIBRARY_DIRS
						${BOOST_LIBRARY_DIR}
)

#=========================================
# Houdini
#=========================================

standardSharedLibrary(
					LIBRARY_NAME
						IECoreHoudini
					THIRD_PARTY_INCLUDE_DIRS
						include
						${HOUDINI_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
						${GLEW_INCLUDE_DIR}
					DEPENDENCIES
						IECore
					COMPILE_FLAGS
						-std=c++11
					PREPROCESSOR_DEFINITIONS
						DLLEXPORT _GNU_SOURCE LINUX AMD64
						SIZEOF_VOID_P=8 SESI_LITTLE_ENDIAN ENABLE_THREADS
						USE_PTHREADS _REENTRANT
						_FILE_OFFSET_BITS=64 GCC4 GCC3 MAKING_DSO
)

standardSharedLibrary(
					LIBRARY_NAME
						IECoreHoudini
					BINDINGS
					THIRD_PARTY_INCLUDE_DIRS
						include
						${HOUDINI_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
						${GLEW_INCLUDE_DIR}
					DEPENDENCIES
						IECoreHoudini
					COMPILE_FLAGS
						-std=c++11
					PREPROCESSOR_DEFINITIONS
						DLLEXPORT _GNU_SOURCE LINUX AMD64
						SIZEOF_VOID_P=8 SESI_LITTLE_ENDIAN ENABLE_THREADS
						USE_PTHREADS _REENTRANT
						_FILE_OFFSET_BITS=64 GCC4 GCC3 MAKING_DSO
)


#=========================================
# Maya
#=========================================

standardSharedLibrary(
					LIBRARY_NAME
						IECoreMaya
					THIRD_PARTY_INCLUDE_DIRS
						include
						${MAYA_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
						${DELIGHT_INCLUDE_DIR}
					DEPENDENCIES
						IECore
					PREPROCESSOR_DEFINITIONS
						LINUX
)

standardSharedLibrary(
					LIBRARY_NAME
						IECoreMaya
					BINDINGS
					THIRD_PARTY_INCLUDE_DIRS
						include
					DEPENDENCIES
						IECoreMaya
					PREPROCESSOR_DEFINITIONS
						LINUX
						BOOST_FILESYSTEM_VERSION=3
						_BOOL
						REQUIRE_IOSTREAM
)

#=========================================
# Nuke
#=========================================

standardSharedLibrary(
					LIBRARY_NAME
						IECoreNuke
					THIRD_PARTY_INCLUDE_DIRS
						include
						${NUKE_INCLUDE_DIR}
						${PYTHON_INCLUDE_DIR}
					DEPENDENCIES
						IECore
					PREPROCESSOR_DEFINITIONS
						LINUX
)

standardSharedLibrary(
					LIBRARY_NAME
						IECoreNuke
					BINDINGS
					THIRD_PARTY_INCLUDE_DIRS
						include
						${PYTHON_INCLUDE_DIR}
					DEPENDENCIES
						IECoreNuke
)
