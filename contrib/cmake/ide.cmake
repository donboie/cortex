include_directories(
            "include"
            "${CORTEX_ROOT}/include"
            "contrib/IECoreAlembic/include"
            "contrib/IECoreArnold/include"
            "contrib/IECoreAppleseed/include")

file(GLOB_RECURSE IECORE_PYTHON_FILES .//*.py )

file(GLOB_RECURSE CORTEX_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.h *.inl)

file(GLOB_RECURSE IECORE_SRCS src/IECore/*.cpp src/IECore/*.inl)
add_library (IECore SHARED ${IECORE_SRCS} ${CORTEX_HEADERS} ${IECORE_PYTHON_FILES})
target_compile_definitions(IECore PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0)

file(GLOB_RECURSE IECOREGL_SRCS src/IECoreGL/*.cpp src/IECoreGL/*.inl)
add_library (IECoreGL SHARED ${IECOREGL_SRCS})
target_compile_definitions(IECoreGL PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0)

file(GLOB_RECURSE IECOREHOUDINI_SRCS src/IECoreHoudini/*.cpp src/IECoreHoudini/*.inl)
add_library (IECoreHoudini SHARED ${IECOREHOUDINI_SRCS})
target_compile_definitions(IECoreHoudini PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
        LINUX  _GNU_SOURCE  AMD64  SIZEOF_VOID_P=8  SESI_LITTLE_ENDIAN
        ENABLE_THREADS USE_PTHREADS  _FILE_OFFSET_BITS=64  DLLEXPORT=)

file(GLOB IECOREMAYA_SRCS src/IECoreMaya/*.cpp src/IECoreHoudini/*.inl)
add_library (IECoreMaya SHARED ${IECOREMAYA_SRCS})
target_compile_definitions(IECoreMaya PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
        _BOOL REQUIRE_IOSTREAM LINUX NDEBUG)

file(GLOB_RECURSE IECORENUKE_SRCS src/IECoreNuke/*.cpp src/IECoreNuke/*.inl)
add_library (IECoreNuke SHARED ${IECORENUKE_SRCS})
target_compile_definitions(IECoreNuke PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
        NDEBUG)

file(GLOB_RECURSE IECOREPYTHON_SRCS src/IECorePython/*.cpp src/IECorePython/*.inl)
add_library (IECorePython SHARED ${IECOREPYTHON_SRCS})
target_compile_definitions(IECorePython PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
        NDEBUG BOOST_PYTHON_MAX_ARITY=20)

file(GLOB_RECURSE IECOREPYTHONMODULE_SRCS src/IECorePythonModule/*.cpp src/IECorePythonModule/*.inl)
add_library (IECorePythonModule SHARED ${IECOREPYTHONMODULE_SRCS})
target_compile_definitions(IECorePythonModule PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
        NDEBUG)

#file(GLOB_RECURSE IECORERI_SRC src/IECoreRI/*.cpp)
#add_library (IECoreRI SHARED ${IECORERI_SRC})
#target_compile_definitions(IECoreRI PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
#        NDEBUG)

#file(GLOB_RECURSE IECORETRUELIGHT_SRC src/IECoreTruelight/*.cpp)
#add_library (IECoreTruelight SHARED ${IECORETRUELIGHT_SRC})
#target_compile_definitions(IECoreTruelight PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
#        NDEBUG )

file(GLOB_RECURSE IECORERMAN_DISPLAYS_SRC src/rmanDisplays/*.cpp src/rmanDisplays/*.inl)
add_library (rmanDisplays SHARED ${IECORERMAN_DISPLAYS_SRC})
target_compile_definitions(rmanDisplays PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
        NDEBUG)

file(GLOB_RECURSE IECOREALEMBIC_SRC contrib/IECoreAlembic/*.cpp contrib/IECoreAlembic/*.inl)
add_library (IECoreAlembic SHARED ${IECOREALEMBIC_SRC})
target_compile_definitions(IECoreAlembic PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0)

file(GLOB_RECURSE IECOREARNOLD_SRC contrib/IECoreArnold/*.cpp contrib/IECoreArnold/*.inl)
add_library (IECoreArnold SHARED ${IECOREARNOLD_SRC})
target_compile_definitions(IECoreArnold  PRIVATE IE_CORE_MAJORVERSION=1 IE_CORE_MINORVERSION=0 IE_CORE_PATCHVERSION=0
        DLLEXPORT=)
