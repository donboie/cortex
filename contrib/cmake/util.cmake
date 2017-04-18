
function (parseVersion name version)
	string(REPLACE "." ";" VERSION_LIST ${version})

	list(GET VERSION_LIST 0 MAJOR_VERSION)
	list(GET VERSION_LIST 1 MINOR_VERSION)
	list(GET VERSION_LIST 2 PATCH_VERSION)

	set(${name}_COMPAT_VERSION ${MAJOR_VERSION}.${MINOR_VERSION} PARENT_SCOPE)
	set(${name}_MAJOR_VERSION ${MAJOR_VERSION} PARENT_SCOPE)
	set(${name}_MINOR_VERSION ${MINOR_VERSION} PARENT_SCOPE)
	set(${name}_PATCH_VERSION ${PATCH_VERSION} PARENT_SCOPE)
endfunction()


function (locateLibraries)
	set(options "")
	set(oneValueArgs RESOLVED_LIBRARY_LIST)
	set(multiValueArgs LIBRARY_DIRS LIBRARIES)
	cmake_parse_arguments(locateLibraries  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	if (${CONFIG_VERBOSE})
		message(STATUS "* locating Libraries:")
	endif()

	set(local_list "")

	foreach(lib ${locateLibraries_LIBRARIES})
		find_library(${lib}_result ${lib} PATHS ${locateLibraries_LIBRARY_DIRS})
		if (${CONFIG_VERBOSE})
			message(STATUS ${lib} " -> " ${${lib}_result})
		endif()
		list(APPEND local_list ${${lib}_result})
	endforeach()

	set(${locateLibraries_RESOLVED_LIBRARY_LIST} ${local_list} PARENT_SCOPE)

endfunction()

function (compilerName)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
		set(COMPILER_NAME "gcc" PARENT_SCOPE)
	elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		set(COMPILER_NAME "clang" PARENT_SCOPE)
	endif()
endfunction()

