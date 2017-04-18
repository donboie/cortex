function (makeSharedLibrary)
	set(options "")
	set(oneValueArgs LIBRARY_NAME)
	set(multiValueArgs SOURCE_DIRS INCLUDE_DIRS THIRD_PARTY_INCLUDE_DIRS THIRD_PARTY_LIBRARY_DIRS THIRD_PARTY_LIBRARIES DEPENDENCIES COMPILE_FLAGS PREPROCESSOR_DEFINITIONS)
	cmake_parse_arguments(makeSharedLibrary  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	if (${CONFIG_VERBOSE})
		message(STATUS "configuring shared library: ${makeSharedLibrary_LIBRARY_NAME}")
		message(STATUS "==========================================")
	endif()

	file(GLOB ${makeSharedLibrary_LIBRARY_NAME}_SOURCES ${makeSharedLibrary_SOURCE_DIRS})
	file(GLOB ${makeSharedLibrary_LIBRARY_NAME}_INCLUDES ${makeSharedLibrary_INCLUDE_DIRS})

	add_library(${makeSharedLibrary_LIBRARY_NAME} SHARED
		${${makeSharedLibrary_LIBRARY_NAME}_SOURCES}
		${${makeSharedLibrary_LIBRARY_NAME}_INCLUDES})

	locateLibraries(
		RESOLVED_LIBRARY_LIST
			ABSOLUTE_LIB_PATHS
		LIBRARY_DIRS
			${makeSharedLibrary_THIRD_PARTY_LIBRARY_DIRS}
		LIBRARIES
			${makeSharedLibrary_THIRD_PARTY_LIBRARIES}
	)

	target_include_directories(${makeSharedLibrary_LIBRARY_NAME} PUBLIC
		${makeSharedLibrary_THIRD_PARTY_INCLUDE_DIRS}
		)

	target_compile_definitions(${makeSharedLibrary_LIBRARY_NAME}
		PRIVATE IE_CORE_MAJORVERSION=0
		IE_CORE_MINORVERSION=0
		IE_CORE_PATCHVERSION=0
		BOOST_PYTHON_MAX_ARITY=20
		${makeSharedLibrary_PREPROCESSOR_DEFINITIONS}
	)

	foreach(compileFlag ${makeSharedLibrary_COMPILE_FLAGS})
		set_target_properties(${makeSharedLibrary_LIBRARY_NAME} PROPERTIES COMPILE_FLAGS ${compileFlag} )
	endforeach()

	target_link_libraries(${makeSharedLibrary_LIBRARY_NAME} ${ABSOLUTE_LIB_PATHS})
	target_link_libraries(${makeSharedLibrary_LIBRARY_NAME} ${makeSharedLibrary_DEPENDENCIES})
	set(MAJOR_VERSION_SUFFIX -9)
	set_target_properties(${makeSharedLibrary_LIBRARY_NAME} PROPERTIES SUFFIX ${MAJOR_VERSION_SUFFIX}.so)

	install(TARGETS ${makeSharedLibrary_LIBRARY_NAME} LIBRARY DESTINATION ${CORTEX_VERSION}/${ARCH}/${APP}/lib/${COMPILER_NAME}/${COMPILER_VERSION})
	install(FILES ${${makeSharedLibrary_LIBRARY_NAME}_INCLUDES} DESTINATION ${CORTEX_VERSION}/${ARCH}/${APP}/include/${makeSharedLibrary_LIBRARY_NAME})
endfunction()


function (installPythonModule)
	SET(options "")
	set(oneValueArgs MODULE_NAME MODULE_DIR)
	set(multiValueArgs "")

	cmake_parse_arguments(installPythonModule  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	file(GLOB_RECURSE ${installPythonModule_MODULE_NAME}_FILES ${installPythonModule_MODULE_DIR}/*.py)
	install(FILES ${${installPythonModule_MODULE_NAME}_FILES} DESTINATION ${CORTEX_VERSION}/${ARCH}/${APP}/python/2.7/gcc/4.8.3/${installPythonModule_MODULE_NAME})
endfunction()


function (standardSharedLibrary)
	set(options BINDINGS)
	set(oneValueArgs LIBRARY_NAME)
	set(multiValueArgs THIRD_PARTY_INCLUDE_DIRS THIRD_PARTY_LIBRARY_DIRS THIRD_PARTY_LIBRARIES DEPENDENCIES COMPILE_FLAGS PREPROCESSOR_DEFINITIONS)
	cmake_parse_arguments(standardSharedLibrary  "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

	if (${standardSharedLibrary_BINDINGS})
		set(newLibName ${standardSharedLibrary_LIBRARY_NAME}Bindings)
		set(newLibSrc src/${standardSharedLibrary_LIBRARY_NAME}/bindings/*.cpp)
		set(newLibInc
				include/${standardSharedLibrary_LIBRARY_NAME}/bindings/*.h
				include/${standardSharedLibrary_LIBRARY_NAME}/bindings/*.inl)
	else()
		set(newLibName ${standardSharedLibrary_LIBRARY_NAME})
		set(newLibSrc src/${standardSharedLibrary_LIBRARY_NAME}/*.cpp)
		set(newLibInc
				include/${standardSharedLibrary_LIBRARY_NAME}/*.h
				include/${standardSharedLibrary_LIBRARY_NAME}/*.inl)
	endif()

	makeSharedLibrary(
		LIBRARY_NAME
			${newLibName}
		SOURCE_DIRS
			${newLibSrc}
		INCLUDE_DIRS
			${newLibInc}
		DEPENDENCIES
			${standardSharedLibrary_DEPENDENCIES}
		THIRD_PARTY_INCLUDE_DIRS
			${standardSharedLibrary_THIRD_PARTY_INCLUDE_DIRS}
		THIRD_PARTY_LIBRARY_DIRS
			${standardSharedLibrary_THIRD_PARTY_LIBRARY_DIRS}
		THIRD_PARTY_LIBRARIES
			${standardSharedLibrary_THIRD_PARTY_LIBRARIES}
		COMPILE_FLAGS
			${standardSharedLibrary_COMPILE_FLAGS}
		PREPROCESSOR_DEFINITIONS
			${standardSharedLibrary_PREPROCESSOR_DEFINITIONS}
	)

	installPythonModule(
		MODULE_NAME
		${standardSharedLibrary_LIBRARY_NAME}
		MODULE_DIR
		python/${standardSharedLibrary_LIBRARY_NAME}
	)

endfunction()
