find_program(BASH_EXECUTABLE bash REQUIRED)

set(test_arch "${CMAKE_SYSTEM_PROCESSOR}")
if(test_arch STREQUAL "amd64")
    set(test_arch x86_64)
elseif(test_arch MATCHES "^i.86$")
    set(test_arch ia32)
elseif(test_arch MATCHES "^arm")
    set(test_arch arm)
endif()

add_library(test_efi OBJECT test.c)
target_compile_options(test_efi PRIVATE -fno-stack-protector -fno-pic -fno-pie
    -ffreestanding -fno-asynchronous-unwind-tables -fno-unwind-tables)

if(test_arch STREQUAL "x86_64")
    set(efi_pe_machine 0x8664)
    target_compile_options(test_efi PRIVATE -mno-red-zone)
elseif(test_arch STREQUAL "ia32")
    set(efi_pe_machine 0x14c)
elseif(test_arch STREQUAL "arm")
    set(efi_pe_machine 0x1c2)
    target_compile_options(test_efi PRIVATE -mthumb)
elseif(test_arch STREQUAL "aarch64")
    set(efi_pe_machine 0xaa64)
elseif(test_arch STREQUAL "riscv64")
    set(efi_pe_machine 0x5064)
else()
    message(FATAL_ERROR "Unsupported EFI test architecture: ${test_arch}")
endif()

if(test_arch MATCHES "^(x86_64|aarch64|riscv64)$")
    set(efi_pe_magic 0x20b)
    set(efi_pe_optional_size 240)
    set(efi_pe_characteristics 0x22)
    set(efi_pe_image_base "QUAD(0)")
    set(efi_pe_word QUAD)
else()
    set(efi_pe_magic 0x10b)
    set(efi_pe_optional_size 224)
    set(efi_pe_characteristics 0x102)
    set(efi_pe_image_base "LONG(0) LONG(0)")
    set(efi_pe_word LONG)
endif()

set(efi_script "${CMAKE_CURRENT_BINARY_DIR}/test-efi.lds")
configure_file(test-efi.lds.in "${efi_script}" @ONLY)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test.elf"
    COMMAND "${CMAKE_LINKER}" -nostdlib -Bsymbolic
        -T "${efi_script}" $<TARGET_OBJECTS:test_efi>
        -o "${CMAKE_CURRENT_BINARY_DIR}/test.elf"
    DEPENDS test_efi $<TARGET_OBJECTS:test_efi> "${efi_script}"
    VERBATIM)

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test.pecoff"
    COMMAND "${CMAKE_OBJCOPY}" -j .header -j .text -j .reloc -O binary
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

if(test_arch MATCHES "^(x86_64|ia32)$")
    list(APPEND shell_tests cert-table-header detach-remove)
endif()

foreach(test IN LISTS shell_tests)
    configure_file("${test}.sh" "${test}.sh" COPYONLY)
    add_test(NAME "${test}" COMMAND "${BASH_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test-wrapper.sh" "${test}.sh")
    set_tests_properties("${test}" PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        ENVIRONMENT "srcdir=${CMAKE_CURRENT_SOURCE_DIR}"
        FIXTURES_REQUIRED certificates)
endforeach()