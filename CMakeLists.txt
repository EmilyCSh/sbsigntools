cmake_minimum_required(VERSION 3.20)
project(sbsigntools VERSION 0.9.5 LANGUAGES C)

include(GNUInstallDirs)
option(BUILD_TESTING "Build and enable tests" OFF)
option(BUILD_MAN_PAGES "Generate and install manual pages with help2man" OFF)
option(REQUIRE_SBKEYSYNC "Require sbkeysync to be built" OFF)
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

find_library(EFIVAR_LIBRARY NAMES efivar)
find_path(EFIVAR_INCLUDE_DIR NAMES efivar.h PATH_SUFFIXES efivar)

if(EFIVAR_LIBRARY AND EFIVAR_INCLUDE_DIR)
    set(EFIVAR_LIBRARIES "${EFIVAR_LIBRARY}")
    find_library(GEOM_LIBRARY NAMES geom)
    if(GEOM_LIBRARY)
        list(APPEND EFIVAR_LIBRARIES "${GEOM_LIBRARY}")
    endif()

    set(efivar_check_source "
        #include <stdint.h>
        #include <sys/types.h>
        #include <efivar.h>
        int main(void) {
            efi_guid_t guid = {0};
            uint8_t data = 0;
            return efi_set_variable(guid, \"db\", &data, 1, 0")

    set(efivar_check_dir "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/efivar-check")
    file(WRITE "${efivar_check_dir}/with-mode.c"
        "${efivar_check_source}, 0600); }\n")
    try_compile(EFIVAR_SET_VARIABLE_HAS_MODE "${efivar_check_dir}/with-mode"
        "${efivar_check_dir}/with-mode.c"
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${EFIVAR_INCLUDE_DIR}"
        LINK_LIBRARIES ${EFIVAR_LIBRARIES}
        OUTPUT_VARIABLE efivar_with_mode_log)

    if(NOT EFIVAR_SET_VARIABLE_HAS_MODE)
        file(WRITE "${efivar_check_dir}/without-mode.c"
            "${efivar_check_source}); }\n")
        try_compile(EFIVAR_SET_VARIABLE_WITHOUT_MODE
            "${efivar_check_dir}/without-mode"
            "${efivar_check_dir}/without-mode.c"
            CMAKE_FLAGS "-DINCLUDE_DIRECTORIES:STRING=${EFIVAR_INCLUDE_DIR}"
            LINK_LIBRARIES ${EFIVAR_LIBRARIES}
            OUTPUT_VARIABLE efivar_without_mode_log)
    endif()

    if(EFIVAR_SET_VARIABLE_HAS_MODE OR EFIVAR_SET_VARIABLE_WITHOUT_MODE)
        list(APPEND SBSIGNTOOLS sbkeysync)
        message(STATUS "sbkeysync enabled: ${EFIVAR_LIBRARY}")
    else()
        message(STATUS "sbkeysync disabled: incompatible libefivar API")
        message(STATUS "libefivar check with mode:\n${efivar_with_mode_log}")
        message(STATUS "libefivar check without mode:\n${efivar_without_mode_log}")
    endif()
else()
    message(STATUS "sbkeysync disabled: libefivar library or headers not found")
endif()

if(REQUIRE_SBKEYSYNC AND NOT "sbkeysync" IN_LIST SBSIGNTOOLS)
    message(FATAL_ERROR "REQUIRE_SBKEYSYNC=ON requires sbkeysync, but libefivar or its compatible headers and API are unavailable")
endif()

foreach(tool IN LISTS SBSIGNTOOLS)
    add_executable(${tool} "src/${tool}.c")
    target_link_libraries(${tool} PRIVATE sbcommon)
    set_target_properties(${tool} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/src")
    install(TARGETS ${tool} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endforeach()

if(TARGET sbkeysync)
    target_link_libraries(sbkeysync PRIVATE ${EFIVAR_LIBRARIES})
    target_include_directories(sbkeysync SYSTEM PRIVATE "${EFIVAR_INCLUDE_DIR}")
    
    if(EFIVAR_SET_VARIABLE_HAS_MODE)
        target_compile_definitions(sbkeysync PRIVATE EFIVAR_SET_VARIABLE_HAS_MODE)
    endif()
endif()

foreach(tool sbvarsign sbsiglist)
    target_link_libraries(${tool} PRIVATE PkgConfig::UUID)
endforeach()

if(BUILD_MAN_PAGES)
    add_subdirectory(docs)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()