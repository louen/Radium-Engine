if(${RADIUM_SUBMODULES_BUILD_TYPE} MATCHES Debug)
    set(ASSIMPLIBNAME assimpd)
else()
    set(ASSIMPLIBNAME assimp)
endif()

set(ASSIMP_INCLUDE_DIR ${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/include)
if(APPLE)
    set(ASSIMP_LIBRARIES "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/lib/lib${ASSIMPLIBNAME}.dylib")
elseif(UNIX)
    set(ASSIMP_LIBRARIES "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/lib/lib${ASSIMPLIBNAME}.a")
elseif(MINGW)
    set(ASSIMP_DLL "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/bin/lib${ASSIMPLIBNAME}.dll")
    set(ASSIMP_LIBRARIES "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/lib/lib${ASSIMPLIBNAME}.dll.a")
elseif(MSVC)
    # in order to prevent DLL hell, each of the DLLs have to be suffixed with the major version and msvc prefix
    if(MSVC70 OR MSVC71)
        set(MSVC_PREFIX "vc70")
    elseif(MSVC80)
        set(MSVC_PREFIX "vc80")
    elseif(MSVC90)
        set(MSVC_PREFIX "vc90")
    elseif(MSVC10)
        set(MSVC_PREFIX "vc100")
    elseif(MSVC11)
        set(MSVC_PREFIX "vc110")
    elseif(MSVC12)
        set(MSVC_PREFIX "vc120")
    else()
        set(MSVC_PREFIX "vc140")
    endif()

    # Very ugly fallback, forcing to rename libs and dll generated by assimp with the name used on other OS.
    # The idea is to avoid taking care of this in the FindRadium.cmake
    if(RADIUM_SUBMODULES_BUILD_TYPE MATCHES Debug)
        set(ASSIMP_LIBRARIES_COMPATNAME "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/lib/assimp-${MSVC_PREFIX}-mtd.lib")
        set(ASSIMP_DLL_COMPATNAME "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/bin/assimp-${MSVC_PREFIX}-mtd.dll")
    else()
        set(ASSIMP_LIBRARIES_COMPATNAME "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/lib/assimp-${MSVC_PREFIX}-mt.lib")
        set(ASSIMP_DLL_COMPATNAME "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/bin/assimp-${MSVC_PREFIX}-mt.dll")
    endif()

    set(ASSIMP_LIBRARIES "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/lib/${ASSIMPLIBNAME}.lib")
    set(ASSIMP_DLL "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}/bin/${ASSIMPLIBNAME}.dll")
endif()

# here is defined the way we want to import assimp
ExternalProject_Add(
    assimp
    # where the source will live
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rdPartyLibraries/Assimp"

    # override default behaviours
    UPDATE_COMMAND ""
    GIT_SUBMODULES 3rdPartyLibraries/Assimp

    # set the installation to installed/assimp
    INSTALL_DIR "${RADIUM_SUBMODULES_INSTALL_DIRECTORY}"
    BUILD_BYPRODUCTS "${ASSIMP_LIBRARIES}"
    CMAKE_GENERATOR ${CMAKE_GENERATOR}
    CMAKE_GENERATOR_TOOLSET ${CMAKE_GENERATOR_TOOLSET}
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DBUILD_SHARED_LIBS=False
        -DASSIMP_BUILD_ASSIMP_TOOLS=False
        -DASSIMP_BUILD_SAMPLES=False
        -DASSIMP_BUILD_TESTS=False
        -DASSIMP_INSTALL_PDB=False
        -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
        -DCMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG}
        -DCMAKE_CXX_FLAGS_RELEASE=${CMAKE_CXX_FLAGS_RELEASE}
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO=${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
        -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}
        -DCMAKE_BUILD_TYPE=${RADIUM_SUBMODULES_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -DINSTALL_NAME_DIR=${CMAKE_RUNTIME_OUTPUT_DIRECTORY} #override rpath to solve run bug on MACOSX
)

add_custom_target(assimp_lib
    DEPENDS assimp
)

# ----------------------------------------------------------------------------------------------------------------------

if(MSVC)
    add_custom_target(assimp_prerename_compiled_lib_and_dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSIMP_LIBRARIES_COMPATNAME}" "${ASSIMP_LIBRARIES}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSIMP_DLL_COMPATNAME}" "${ASSIMP_DLL}"
        COMMENT "copy assimp libs and dll using the generic filename used on other OS. This is messy" VERBATIM
        DEPENDS assimp
    )

    add_custom_target(assimp_install_compiled_dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSIMP_DLL_COMPATNAME} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSIMP_DLL} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMENT "copy assimp dll to bin dir" VERBATIM
        DEPENDS assimp_prerename_compiled_lib_and_dll
    )
    add_dependencies(assimp_lib assimp_install_compiled_dll )
endif()

if( MINGW )
    add_custom_target(assimp_install_compiled_dll
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ASSIMP_DLL} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            COMMENT "copy assimp dll to bin dir" VERBATIM
            DEPENDS assimp create_bin_dir
    )
    add_dependencies(assimp_lib assimp_install_compiled_dll)
endif()

# ------------------------------------------------------------------------------
get_filename_component(ASSIMP_LIB_NAME "${ASSIMP_LIBRARIES}" NAME)
