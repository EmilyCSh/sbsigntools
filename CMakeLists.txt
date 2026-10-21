cmake_minimum_required(VERSION 3.20)
project(sbsigntools VERSION 0.9.5 LANGUAGES C)

include(GNUInstallDirs)
option(BUILD_TESTING "Build and enable tests" OFF)
option(BUILD_MAN_PAGES "Generate and install manual pages with help2man" OFF)
include(CTest)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Werror -Wpedantic)

find_package(OpenSSL REQUIRED COMPONENTS Crypto)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UUID REQUIRED IMPORTED_TARGET uuid)

add_library(sbcommon STATIC src/idc.c src/image.c src/fileio.c src/util.c)
target_compile_definitions(sbcommon PUBLIC
    PACKAGE="sbsigntool"
    PACKAGE_VERSION="${PROJECT_VERSION}"
    VERSION="${PROJECT_VERSION}")
target_link_libraries(sbcommon PUBLIC OpenSSL::Crypto)

if(OPENSSL_VERSION VERSION_GREATER_EQUAL 3.0)
    target_compile_definitions(sbcommon PUBLIC OPENSSL_API_COMPAT=0x10100000L)
endif()

set(SBSIGNTOOLS sbsign sbverify sbattach sbvarsign sbsiglist)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND SBSIGNTOOLS sbkeysync)
endif()

foreach(tool IN LISTS SBSIGNTOOLS)
    add_executable(${tool} "src/${tool}.c")
    target_link_libraries(${tool} PRIVATE sbcommon)
    set_target_properties(${tool} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/src")
    install(TARGETS ${tool} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endforeach()

foreach(tool sbvarsign sbsiglist)
    target_link_libraries(${tool} PRIVATE PkgConfig::UUID)
endforeach()

if(BUILD_MAN_PAGES)
    add_subdirectory(docs)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()