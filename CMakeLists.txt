cmake_minimum_required( VERSION 3.1 )

# work in progress cmake build script.
# Please build Cortex using scons
if( ${IDE_CONFIG} )
	include( contrib/cmake/ide.cmake )
else()
	include( contrib/cmake/build.cmake )
endif()

