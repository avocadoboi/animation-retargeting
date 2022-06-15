
find_package(LibXml2 REQUIRED)

find_path(FBX_SDK_INCLUDE_DIRS fbxsdk.h HINTS ENV FBX_ROOT PATH_SUFFIXES include)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PROCESSOR_TYPE x64)
else ()
    set(PROCESSOR_TYPE x86)
endif ()

function(create_fbx_sdk_path_suffixes_for_build_type BUILD_TYPE)
    string(TOUPPER ${BUILD_TYPE} BUILD_TYPE_UPPERCASE)
    set(FBX_SDK_PATH_SUFFIXES_${BUILD_TYPE_UPPERCASE}
        # Windows
        lib/vs2023/${PROCESSOR_TYPE}/${BUILD_TYPE}
        lib/vs2021/${PROCESSOR_TYPE}/${BUILD_TYPE}
        lib/vs2019/${PROCESSOR_TYPE}/${BUILD_TYPE}
        lib/vs2017/${PROCESSOR_TYPE}/${BUILD_TYPE}
        lib/vs2015/${PROCESSOR_TYPE}/${BUILD_TYPE}
        # Linux
        lib/gcc/${PROCESSOR_TYPE}/${BUILD_TYPE}
        # MacOS
        lib/clang/${BUILD_TYPE}
        PARENT_SCOPE)
endfunction()

create_fbx_sdk_path_suffixes_for_build_type(debug)
create_fbx_sdk_path_suffixes_for_build_type(release)

find_library(FBX_SDK_LIBRARIES_RELEASE
    NAMES libfbxsdk.a libfbxsdk-mt.lib
    HINTS ENV FBX_ROOT 
    PATH_SUFFIXES ${FBX_SDK_PATH_SUFFIXES_RELEASE}
    REQUIRED)

find_library(FBX_SDK_LIBRARIES_DEBUG
    NAMES libfbxsdk.a libfbxsdk-mt.lib
    HINTS ENV FBX_ROOT 
    PATH_SUFFIXES ${FBX_SDK_PATH_SUFFIXES_DEBUG}
    REQUIRED)

if (FBX_SDK_INCLUDE_DIRS AND (FBX_SDK_LIBRARIES_RELEASE OR FBX_SDK_LIBRARIES_DEBUG))
    set(FBX_SDK_FOUND TRUE)
    if (NOT TARGET FbxSdk::fbx_sdk)
        add_library(FbxSdk::fbx_sdk UNKNOWN IMPORTED)
        set_target_properties(FbxSdk::fbx_sdk PROPERTIES 
            INTERFACE_INCLUDE_DIRECTORIES ${FBX_SDK_INCLUDE_DIRS}
            INTERFACE_LINK_LIBRARIES LibXml2::LibXml2)
        if (FBX_SDK_LIBRARIES_RELEASE)
            set_property(TARGET FbxSdk::fbx_sdk APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(FbxSdk::fbx_sdk PROPERTIES IMPORTED_LOCATION_RELEASE ${FBX_SDK_LIBRARIES_RELEASE})
        endif ()
        if (FBX_SDK_LIBRARIES_DEBUG)
            set_property(TARGET FbxSdk::fbx_sdk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(FbxSdk::fbx_sdk PROPERTIES IMPORTED_LOCATION_DEBUG ${FBX_SDK_LIBRARIES_DEBUG})
        endif ()
    endif ()
else ()
    set(FBX_SDK_FOUND FALSE)
endif ()
