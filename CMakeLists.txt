cmake_minimum_required(VERSION 3.9 FATAL_ERROR)
project(isocore_project VERSION 2.31 LANGUAGES C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/")

include(IsolarioSetup)

option(ISOLARIO_SKIP_TESTS "Skip building tests" ${ISOLARIO_SKIP_TESTS})
option(ISOLARIO_SKIP_BENCHMARKS "Skip building benchmarks" ${ISOLARIO_SKIP_BENCHMARKS})

if (NOT ${ISOLARIO_SKIP_TESTS})
    set(ISOLARIO_ENABLE_TESTS On)
endif ()
if (${ISOLARIO_ENABLE_TESTS})
    message(STATUS "Configuring Isolario to build tests.")
else ()
    message(STATUS "Configuring Isolario to avoid building tests.")
endif ()

if (NOT ${ISOLARIO_SKIP_BENCHMARKS})
    set(ISOLARIO_ENABLE_BENCHMARKS ON)
endif ()
if (${ISOLARIO_ENABLE_BENCHMARKS})
    message(STATUS "Configuring Isolario to build benchmarks.")
else ()
    message(STATUS "Configuring Isolario to avoid building benchmarks.")
endif ()

mark_as_advanced(ISOLARIO_ENABLE_TESTS ISOLARIO_ENABLE_BENCHMARKS)

# add utility targets
find_package(PythonInterp 2.7)
set(PROJECT_TRDPARTY_REGEX "external")
if (${PYTHONINTERP_FOUND})
    add_custom_target(check-isocore-copyright ALL
        ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/add_copyright.py
        --check
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Check copyright")

    add_custom_target(add-isocore-copyright
      ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/add_copyright.py
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Add copyright")

else ()

    message(WARNING "Python interpreter not found, necessary for check-isolario-copyright and add-isolario-copyright targets")

endif ()

# find system dependencies
find_package(CURL 7.51 REQUIRED)
find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(LibUnwind 1.1 REQUIRED)
find_package(CUnit REQUIRED)

set(isocore_src
    src/hexdump.c
    src/log.c
    src/parse.c
    src/strutil.c
    src/uint128_t.c
    src/backtrace.c
    src/cache.c
    src/endian.c
)

mark_as_advanced(isocore_src)

add_library(isocore ${isocore_src})
target_link_libraries(isocore
    PRIVATE BZip2::BZip2
            ${CURL_LIBRARIES}
            ZLIB::ZLIB
            LibUnwind::LibUnwind
)
target_include_directories(isocore PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>

    PRIVATE ${CURL_INCLUDE_DIRS})
isolario_default_compile_options(TARGET isocore PURE)
target_compile_definitions(isocore
    PRIVATE _LARGEFILE_SOURCE
            _LARGEFILE64_SOURCE
            _FILE_OFFSET_BITS=64
            _POSIX_C_SOURCE=200809L)

install(TARGETS isocore EXPORT IsolarioCore
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT IsolarioCore DESTINATION share/IsolarioCore/cmake)
export(TARGETS isocore FILE IsolarioCoreConfig.cmake)

# testing and benchmarking targets
if (${ISOLARIO_ENABLE_TESTS})
    include(CTest)

    add_executable(strutil_test
        test/strutil/main.c
        test/strutil/splitjoin.c
        test/strutil/trim.c
    )

    isolario_default_compile_options(TARGET strutil_test PURE)
    target_link_libraries(strutil_test PRIVATE isocore CUnit::CUnit)

    add_test(strutil_test_suite strutil_test WORKING_DIRECTORY test/strutil)

    add_executable(core_test
        test/core/hexdump.c
        test/core/main.c
        test/core/uint128.c
    )

    isolario_default_compile_options(TARGET core_test PURE)
    target_link_libraries(core_test PRIVATE isocore CUnit::CUnit)

    add_test(core_test_suite core_test WORKING_DIRECTORY test/core)

endif ()

