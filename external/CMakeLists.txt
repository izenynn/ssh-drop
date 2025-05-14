# Don't compile using shared libs
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# libssh
# https://git.libssh.org/projects/libssh.git
find_package(Threads REQUIRED)
set(LIBSSH_STATIC ON CACHE BOOL "" FORCE)
set(UNIT_TESTING OFF CACHE BOOL "" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WITH_GSSAPI OFF CACHE BOOL "" FORCE)
set(WITH_NACL OFF CACHE BOOL "" FORCE)
set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
add_subdirectory(libssh)
target_link_libraries(main ssh Threads::Threads)
