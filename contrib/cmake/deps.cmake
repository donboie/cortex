
if (${SITE_CS})

	set(TBB_VERSION 4.3)
	set(OPENEXR_VERSION 2.2.0)
	set(FREETYPE_VERSION 2.7.1)
	set(PNG_VERSION  1.11.0)
	set(BOOST_VERSION 1.55.0)
	set(GLEW_VERSION 1.7.0)
	set(PYTHON_VERSION 2.7.9)

	parseVersion(PYTHON ${PYTHON_VERSION})

	set(EXTERNAL_DEP_ROOT "/home/don/dev/docker-kiln/industrial/kiln/install/cinesite.platform/1.2/CentOS_Linux_7.0.1406/gcc4.8.5_glibc2.17_x86_64/CY2014/")
	set(TBB_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/tbb/${TBB_VERSION}/include/)
	set(OPENEXR_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/openexr/${OPENEXR_VERSION}/include)
	set(FREETYPE_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/freetype/${FREETYPE_VERSION}/include/freetype2)
	set(PNG_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/png/${PNG_VERSION}/include)
	set(BOOST_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/boost/${BOOST_VERSION}/include)
	set(GLEW_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/glew/${GLEW_VERSION}/include)
	set(PYTHON_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/python${PYTHON_VERSION}/${PYTHON_VERSION}/include/python${PYTHON_COMPAT_VERSION})

else()

	set(TBB_VERSION 4.3)
	set(OPENEXR_VERSION 2.2.0)
	set(FREETYPE_VERSION 2.7.1)
	set(PNG_VERSION  1.11.0)
	set(BOOST_VERSION 1.55.0)
	set(GLEW_VERSION 1.7.0)
	set(PYTHON_VERSION 2.7.9)

	parseVersion(PYTHON ${PYTHON_VERSION})

	set(TBB_INCLUDE_DIR /software/apps/tbb/${TBB_VERSION}/${ARCH}/${COMPILER_NAME}/${COMPILER_VERSION}/include)
	set(OPENEXR_INCLUDE_DIR /software/apps/openexr/${OPENEXR_VERSION}/${ARCH}/${COMPILER_NAME}/${COMPILER_VERSION}/include)

	set(FREETYPE_INCLUDE_DIR /usr/include/freetype2)
	set(PNG_INCLUDE_DIR ${EXTERNAL_DEP_ROOT}/png/${PNG_VERSION}/include)

	set(BOOST_INCLUDE_DIR /software/tools/include/${ARCH}/boost/${BOOST_VERSION})
	set(GLEW_INCLUDE_DIR /software/tools/include/glew/${GLEW_VERSION})
	set(PYTHON_INCLUDE_DIR /software/apps/python/${PYTHON_COMPAT_VERSION}/${ARCH}/gcc/${COMPILER_VERSION}/include/python${PYTHON_COMPAT_VERSION})

	set(BOOST_LIBRARY_DIR /software/tools/lib/${ARCH}/${COMPILER_NAME}/${COMPILER_VERSION})
	set(TBB_LIBRARY_DIR /software/apps/tbb/${TBB_VERSION}/${ARCH}/${COMPILER_NAME}/${COMPILER_VERSION}/lib)
	set(OPENEXR_LIBRARY_DIR /software/apps/openexr/${OPENEXR_VERSION}/${ARCH}/${COMPILER_NAME}/${COMPILER_VERSION}/lib)

	set(HOUDINI_INCLUDE_DIR /software/apps/houdini/15.5.596/cent7.x86_64/toolkit/include/)
	set(MAYA_INCLUDE_DIR /software/apps/maya/2016.sp6/cent7.x86_64/include/)
	set(DELIGHT_INCLUDE_DIR /software/apps/3delight/12.0.75/cent7.x86_64/include)
	set(NUKE_INCLUDE_DIR /software/apps/nuke/10.0v4/cent7.x86_64/include)

endif()


if (${CONFIG_VERBOSE})

	message(STATUS "dependency locations")
	message(STATUS "====================")

	message(STATUS ${TBB_INCLUDE_DIR} )
	message(STATUS ${OPENEXR_INCLUDE_DIR} )
	message(STATUS ${FREETYPE_INCLUDE_DIR} )
	message(STATUS ${PNG_INCLUDE_DIR} )
	message(STATUS ${BOOST_INCLUDE_DIR} )
	message(STATUS ${GLEW_INCLUDE_DIR} )
	message(STATUS ${PYTHON_INCLUDE_DIR} )

endif()
