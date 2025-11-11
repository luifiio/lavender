cmake_minimum_required(VERSION 3.16)
project(lavender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules ${CMAKE_MODULE_PATH})

# ext libs
set(CMAKE_PREFIX_PATH "/users/lui/Qt/6.8.2/macos")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Qml Quick QuickWidgets Sql MultiMedia)
find_package(SQLite3 REQUIRED)
find_package(CURL REQUIRED)
find_package(Taglib REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CHROMAPRINT REQUIRED IMPORTED_TARGET libchromaprint)
#find_package(Python3 COMPONENTS Interpreter Development)

# Sources
set(SOURCES
    src/main.cpp
    src/introMenu.cpp
    src/introMenu.h
    src/mainMenu.cpp
    src/mainMenu.h
    src/albumMenu.cpp
    src/albumMenu.h
    src/songMenu.cpp
    src/songMenu.h
    src/playback.cpp
    src/playback.h
    src/mainwindow.h
    src/mainwindow.cpp
    src/recoMenu.cpp
    src/recoMenu.h
    src/dbManager.cpp
    src/dbManager.h
    src/libScan.cpp
    src/libScan.h
    src/apiFetch.cpp
    src/apiFetch.h
    src/audiofingerprint.cpp
    src/audiofingerprint.h
)
set(RESOURCE_FILES
    resources/placeholder.jpeg
    resources/Info.plist
)

qt6_wrap_cpp(MOC_SOURCES src/mainwindow.h src/introMenu.h src/albumMenu.h src/songMenu.h src/mainMenu.h src/playback.h src/apiFetch.h src/recoMenu.h src/audiofingerprint.h src/libScan.h src/dbManager.h)
qt6_add_resources(RESOURCES resources.qrc)

file(COPY ${CMAKE_SOURCE_DIR}/scripts/recoEngine.py DESTINATION ${CMAKE_BINARY_DIR}/lavender.app/Contents/MacOS/scripts)
add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES} ${MOC_SOURCES} ${RESOURCE_FILES})

# Include directories
target_include_directories(lavender PRIVATE ${TAGLIB_INCLUDE_DIR} ${Qt6Widgets_INCLUDE_DIRS})


# link libs 
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        SQLite::SQLite3
        ${TAGLIB_LIBRARY}
        CURL::libcurl
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickWidgets
        Qt6::Sql
        Qt6::Multimedia
        PkgConfig::CHROMAPRINT
)


# macos apple bundle
if(APPLE)
    # ressources
    set_source_files_properties(${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # info.plist
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/Info.plist
    )
endif()

#--- tests ---#



enable_testing()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test_data)

add_executable(test_audiofingerprint
    tests/test_audiofingerprint.cpp
    src/audiofingerprint.h
    src/audiofingerprint.cpp
)

target_link_libraries(test_audiofingerprint
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Test
        PkgConfig::CHROMAPRINT
        CURL::libcurl
)


add_executable(benchmark_audiofingerprint
    tests/benchmark_audiofingerprint.cpp
    src/audiofingerprint.h
    src/audiofingerprint.cpp
)

target_link_libraries(benchmark_audiofingerprint
    PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::Test
        PkgConfig::CHROMAPRINT
        CURL::libcurl
)

# Register tests with CTest
add_test(
    NAME test_audiofingerprint
    COMMAND test_audiofingerprint
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_test(
    NAME benchmark_audiofingerprint
    COMMAND benchmark_audiofingerprint
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# LibScan test
add_executable(test_libscan
    tests/testLibscan.cpp
    src/libScan.h
    src/libScan.cpp
)

target_link_libraries(test_libscan
    PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Test
        ${TAGLIB_LIBRARY}
)

add_test(
    NAME test_libscan
    COMMAND test_libscan
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# SongDetail test
add_executable(test_songdetail
    tests/test_songdetail.cpp
    src/songMenu.cpp
    src/songMenu.h
    src/audiofingerprint.cpp
    src/audiofingerprint.h
)

target_link_libraries(test_songdetail
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Test
        ${TAGLIB_LIBRARY}
        PkgConfig::CHROMAPRINT
        CURL::libcurl
)

add_test(
    NAME test_songdetail
    COMMAND test_songdetail
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_executable(benchmark_lavender
    tests/benchmark.cpp
    src/dbManager.cpp
    src/libScan.cpp
    src/audiofingerprint.cpp
    src/playback.cpp
)

target_link_libraries(benchmark_lavender
    PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Network
        Qt6::Multimedia
        Qt6::Test
        ${TAGLIB_LIBRARY}
        PkgConfig::CHROMAPRINT
        CURL::libcurl
)

add_test(
    NAME benchmark_lavender
    COMMAND benchmark_lavender
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Copy analysis script if it exists
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/fingerprint.py")
    configure_file(
        ${CMAKE_SOURCE_DIR}/tests/fingerprint.py
        ${CMAKE_BINARY_DIR}/analyze_results.py
        COPYONLY
    )
endif()

# Simple target to run all tests with verbose output
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -V
    DEPENDS test_audiofingerprint benchmark_audiofingerprint
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests with verbose output..."
)
