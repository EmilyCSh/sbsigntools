find_program(HELP2MAN_EXECUTABLE help2man REQUIRED)

foreach(tool IN LISTS SBSIGNTOOLS)
    set(man_page "${CMAKE_CURRENT_BINARY_DIR}/${tool}.1")
    add_custom_command(OUTPUT "${man_page}"
        COMMAND "${HELP2MAN_EXECUTABLE}" --no-info
            -i "${CMAKE_CURRENT_SOURCE_DIR}/${tool}.1.in"
            -o "${man_page}" "$<TARGET_FILE:${tool}>"
        DEPENDS ${tool} "${CMAKE_CURRENT_SOURCE_DIR}/${tool}.1.in"
        VERBATIM)
    list(APPEND man_pages "${man_page}")
    install(FILES "${man_page}" DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
endforeach()

add_custom_target(man ALL DEPENDS ${man_pages})