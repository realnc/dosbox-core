include_directories (
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GLIB_INCLUDEDIR}
    ${GLIB_INCLUDE_DIRS}
    ${GOBJECT_INCLUDEDIR}
    ${GOBJECT_INCLUDE_DIRS}
    ${SNDFILE_INCLUDEDIR}
    ${SNDFILE_INCLUDE_DIRS}
)

if( WIN32 )
    # disable deprecation warnings 
    add_definitions ( -D_CRT_SECURE_NO_WARNINGS )
endif( WIN32 )

link_directories (
    ${GLIB_LIBDIR}
    ${GLIB_LIBRARY_DIRS}
    ${GOBJECT_LIBDIR}
    ${GOBJECT_LIBRARY_DIRS}
    ${SNDFILE_LIBDIR}
    ${SNDFILE_LIBRARY_DIRS}
)

add_executable ( riff_dump riff_dump.c )
add_dependencies( riff_dump libinstpatch )
target_link_libraries( riff_dump libinstpatch )
