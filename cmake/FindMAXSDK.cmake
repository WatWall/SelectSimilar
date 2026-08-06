if(MAXSDK_PATH AND NOT "${MAXSDK_PATH}" STREQUAL "")
    set(MAXSDK_ROOT "${MAXSDK_PATH}")
elseif(EXISTS "C:/Program Files/Autodesk/3ds Max 2027 SDK/maxsdk")
    set(MAXSDK_ROOT "C:/Program Files/Autodesk/3ds Max 2027 SDK/maxsdk")
elseif(EXISTS "C:/Program Files/Autodesk/3ds Max 2025 SDK/maxsdk")
    set(MAXSDK_ROOT "C:/Program Files/Autodesk/3ds Max 2025 SDK/maxsdk")
else()
    message(FATAL_ERROR
        "3ds Max SDK not found. Set MAXSDK_PATH to the SDK's 'maxsdk' directory.\n"
        "Example: -DMAXSDK_PATH=\"C:/Program Files/Autodesk/3ds Max 2027 SDK/maxsdk\"\n"
        "Download it from: https://aps.autodesk.com/developer/overview/3ds-max"
    )
endif()

set(MAXSDK_INCLUDE_DIR "${MAXSDK_ROOT}/include")
set(MAXSDK_LIB_DIR "${MAXSDK_ROOT}/lib/x64/Release")

if(EXISTS "${MAXSDK_INCLUDE_DIR}/maxapi.h" AND EXISTS "${MAXSDK_LIB_DIR}")
    set(MAXSDK_INCLUDE_DIRS
        "${MAXSDK_INCLUDE_DIR}"
        "${MAXSDK_ROOT}"
    )

    file(GLOB MAXSDK_LIBRARIES
        "${MAXSDK_LIB_DIR}/*.lib"
    )

    set(MAXSDK_FOUND TRUE)
    message(STATUS "3ds Max SDK found at: ${MAXSDK_ROOT}")
    message(STATUS "  Include dir: ${MAXSDK_INCLUDE_DIR}")
    message(STATUS "  Lib dir:     ${MAXSDK_LIB_DIR}")
else()
    message(FATAL_ERROR
        "3ds Max SDK not found at: ${MAXSDK_ROOT}\n"
        "  include/maxapi.h missing or lib/x64/Release missing"
    )
endif()
