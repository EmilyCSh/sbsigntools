find_program(BASH_EXECUTABLE bash REQUIRED)

add_library(test_efi OBJECT test.c)
target_include_directories(test_efi PRIVATE "${EFI_INCLUDE_DIR}/efi" "${EFI_ARCH_INCLUDE_DIR}")
target_compile_options(test_efi PRIVATE -fshort-wchar -fno-stack-protector -fpic
    -ffreestanding -fno-asynchronous-unwind-tables -fno-unwind-tables)

if(EFI_ARCH MATCHES "^(x86_64|ia32)$")
    if(EFI_ARCH STREQUAL "x86_64")
        set(efi_pe_machine 0x8664)
        set(efi_pe_magic 0x20b)
        set(efi_pe_optional_size 240)
        set(efi_pe_characteristics 0x22)
        set(efi_pe_image_base "QUAD(0)")
        set(efi_pe_word QUAD)
        target_compile_definitions(test_efi PRIVATE GNU_EFI_USE_MS_ABI)
        target_compile_options(test_efi PRIVATE -mno-red-zone)
    else()
        set(efi_pe_machine 0x14c)
        set(efi_pe_magic 0x10b)
        set(efi_pe_optional_size 224)
        set(efi_pe_characteristics 0x102)
        set(efi_pe_image_base "LONG(0) LONG(0)")
        set(efi_pe_word LONG)
    endif()
    target_compile_options(test_efi PRIVATE -fno-pic -fno-pie)
    set(efi_script "${CMAKE_CURRENT_BINARY_DIR}/test-efi.lds")
    configure_file(test-efi.lds.in "${efi_script}" @ONLY)
    set(efi_sections -j .header -j .text -j .reloc)
else()
    if(NOT EFI_ARCH MATCHES "^(arm|aarch64|riscv64)$")
        message(FATAL_ERROR "Unsupported EFI test architecture: ${EFI_ARCH}")
    endif()
    find_file(EFI_CRT "crt0-efi-${EFI_ARCH}.o"
        PATHS /lib /lib64 /usr/lib /usr/lib64 /usr/lib32
        PATH_SUFFIXES efi gnuefi REQUIRED)
    get_filename_component(EFI_LIBRARY_DIR "${EFI_CRT}" DIRECTORY)
    find_file(EFI_LINKER_SCRIPT "elf_${EFI_ARCH}_efi.lds"
        HINTS "${EFI_LIBRARY_DIR}" REQUIRED)
    find_library(EFI_LIBRARY efi HINTS "${EFI_LIBRARY_DIR}" REQUIRED)
    find_library(GNUEFI_LIBRARY gnuefi HINTS "${EFI_LIBRARY_DIR}" REQUIRED)
    set(test_binary_format ON)
    set(efi_startup "${EFI_CRT}")
    set(efi_script "${EFI_LINKER_SCRIPT}")
    set(efi_link_flags -shared --defsym=EFI_SUBSYSTEM=0x0a)
    set(efi_libraries "${EFI_LIBRARY}" "${GNUEFI_LIBRARY}")
    set(efi_sections -j .text -j .sdata -j .data -j .dynamic
        -j .dynsym -j .rel -j .rela -j .reloc)
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test.elf"
    COMMAND "${CMAKE_LINKER}" ${efi_link_flags} -nostdlib -Bsymbolic
        ${efi_startup} -T "${efi_script}" $<TARGET_OBJECTS:test_efi>
        -o "${CMAKE_CURRENT_BINARY_DIR}/test.elf" ${efi_libraries}
    DEPENDS test_efi $<TARGET_OBJECTS:test_efi> ${efi_startup} "${efi_script}"
        ${efi_libraries}
    VERBATIM)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test.pecoff"
    COMMAND "${CMAKE_OBJCOPY}" ${efi_sections} -O binary
        "${CMAKE_CURRENT_BINARY_DIR}/test.elf" "${CMAKE_CURRENT_BINARY_DIR}/test.pecoff"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/test.elf"
    VERBATIM)

add_custom_target(test_image ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/test.pecoff")

find_program(OPENSSL_EXECUTABLE openssl REQUIRED)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ca.cnf"
    "[ca]\nbasicConstraints = critical, CA:true\n")

add_custom_target(test_certificates
    COMMAND "${OPENSSL_EXECUTABLE}" genrsa -out private-key.rsa 2048
    COMMAND "${OPENSSL_EXECUTABLE}" genpkey -algorithm ec
        -pkeyopt ec_paramgen_curve:prime256v1 -out ca-key.ec
    COMMAND "${OPENSSL_EXECUTABLE}" genpkey -algorithm ec
        -pkeyopt ec_paramgen_curve:prime256v1 -out int-key.ec
    COMMAND "${OPENSSL_EXECUTABLE}" req -x509 -days 1 -sha256
        -subj "/CN=CA Key/" -new -key ca-key.ec -out ca-cert.pem
    COMMAND "${OPENSSL_EXECUTABLE}" req -new
        -subj "/CN=Intermediate Certificate/" -key int-key.ec -out int.req
    COMMAND "${OPENSSL_EXECUTABLE}" x509 -req -sha256 -CA ca-cert.pem
        -CAkey ca-key.ec -in int.req -set_serial 1 -days 1
        -extfile ca.cnf -extensions ca -out int-cert.pem
    COMMAND "${OPENSSL_EXECUTABLE}" req -new -subj "/CN=Signer Certificate/"
        -key private-key.rsa -out signer.req
    COMMAND "${OPENSSL_EXECUTABLE}" x509 -req -sha256 -CA int-cert.pem
        -CAkey int-key.ec -in signer.req -set_serial 1 -days 1
        -out public-cert.pem
    COMMAND "${CMAKE_COMMAND}" -E rm -f int.req signer.req
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/private-key.rsa"
        "${CMAKE_CURRENT_BINARY_DIR}/ca-key.ec" "${CMAKE_CURRENT_BINARY_DIR}/int-key.ec"
        "${CMAKE_CURRENT_BINARY_DIR}/ca-cert.pem" "${CMAKE_CURRENT_BINARY_DIR}/int-cert.pem"
        "${CMAKE_CURRENT_BINARY_DIR}/public-cert.pem"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM)

add_test(NAME test-certificates COMMAND "${CMAKE_COMMAND}"
    --build "${PROJECT_BINARY_DIR}" --config "$<CONFIG>" --target test_certificates)
set_tests_properties(test-certificates PROPERTIES FIXTURES_SETUP certificates
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

set(shell_tests sign-verify sign-verify-detached sign-detach-verify
    sign-attach-verify sign-missing-image sign-missing-cert sign-missing-key
    verify-missing-image verify-missing-cert sign-invalidattach-verify
    resign-warning reattach-warning)

if(NOT test_binary_format)
    list(APPEND shell_tests cert-table-header detach-remove)
endif()

foreach(test IN LISTS shell_tests)
    configure_file("${test}.sh" "${test}.sh" COPYONLY)
    add_test(NAME "${test}" COMMAND "${BASH_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test-wrapper.sh" "${test}.sh")
    set_tests_properties("${test}" PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "srcdir=${CMAKE_CURRENT_SOURCE_DIR};TEST_ARCHES=${EFI_ARCH}"
        FIXTURES_REQUIRED certificates)
endforeach()