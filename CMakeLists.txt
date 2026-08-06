cmake_minimum_required(VERSION 3.29)
project(SelectSimilar VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PLUGIN_NAME "SelectSimilar")

option(MAXSDK_PATH "Path to 3ds Max SDK maxsdk root" "C:/Program Files/Autodesk/3ds Max 2027 SDK/maxsdk")
option(MAX_INSTALL_PATH "Path to 3ds Max install" "C:/Program Files/Autodesk/3ds Max 2027")

# Derive the Max version year from the SDK path (e.g. "3ds Max 2027 SDK" -> 2027)
# so each SDK build produces a version-specific DLL (SelectSimilar2027.dlu).
string(REGEX MATCH "3ds Max ([0-9]+)" MAX_VERSION_MATCH "${MAXSDK_PATH}")
if(MAX_VERSION_MATCH)
    set(PLUGIN_OUTPUT_NAME "SelectSimilar${CMAKE_MATCH_1}")
else()
    set(PLUGIN_OUTPUT_NAME "SelectSimilar2027")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(MAXSDK REQUIRED)

add_library(${PLUGIN_NAME} SHARED
    src/core/DllEntry.cpp
    src/core/PluginDescriptor.cpp
    src/core/SelectSimGUP.cpp
    src/selectsim/SimilarParams.cpp
    src/selectsim/SelectSimilarAction.cpp
    src/selectsim/SelectSimilarAdjuster.cpp
    src/selectsim/SelectSimilarComputations.cpp
    src/selectsim/SelectSimilarEngine.cpp
    src/selectsim/SelectSimilarMenu.cpp
    src/core/SelectSimPlugin.rc
)

target_include_directories(${PLUGIN_NAME} PRIVATE
    ${MAXSDK_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(${PLUGIN_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _ADESK_3DSMAX_WINDOWS_
    WINVER=0x0601
    _WIN32_WINNT=0x0601
    _WIN32_IE=0x0800
    _WINDOWS
    _CRT_SECURE_NO_DEPRECATE
    _CRT_NONSTDC_NO_DEPRECATE
    _SCL_SECURE_NO_DEPRECATE
    ISOLATION_AWARE_ENABLED=1
    MODULE_NAME="${PLUGIN_OUTPUT_NAME}.dlu"
    UNICODE
    _UNICODE
)

target_compile_options(${PLUGIN_NAME} PRIVATE
    /GR
    /Zm1000
    /Zc:__cplusplus
    /Zc:lambda
    /Zc:inline
    /EHsc
    /fp:precise
    /W3
    /bigobj
)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${MAXSDK_LIBRARIES}
    comctl32.lib
    shell32.lib
    user32.lib
    gdi32.lib
)

set_target_properties(${PLUGIN_NAME} PROPERTIES
    OUTPUT_NAME ${PLUGIN_OUTPUT_NAME}
    PREFIX ""
    SUFFIX ".dlu"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/output"
)

add_custom_target(deploy
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PLUGIN_NAME}>"
        "${MAX_INSTALL_PATH}/Plugins/$<TARGET_FILE_NAME:${PLUGIN_NAME}>"
    COMMENT "Deploying plugin to 3ds Max Plugins folder (run as admin)"
    DEPENDS ${PLUGIN_NAME}
    VERBATIM
)

install(TARGETS ${PLUGIN_NAME}
    RUNTIME DESTINATION "Plugins"
    LIBRARY DESTINATION "Plugins"
)
