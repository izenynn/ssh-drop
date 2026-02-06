# Don't compile using shared libs
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Disable install rules for embedded libraries (prevents export set conflicts)
set(CMAKE_SKIP_INSTALL_RULES ON)

# mbedtls
# https://github.com/Mbed-TLS/mbedtls.git
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
add_subdirectory(mbedtls EXCLUDE_FROM_ALL)

# Pre-set mbedTLS variables so libssh's FindMbedTLS.cmake finds our embedded build
set(MBEDTLS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mbedtls/include" CACHE PATH "" FORCE)
set(MBEDTLS_SSL_LIBRARY mbedtls CACHE FILEPATH "" FORCE)
set(MBEDTLS_CRYPTO_LIBRARY mbedcrypto CACHE FILEPATH "" FORCE)
set(MBEDTLS_X509_LIBRARY mbedx509 CACHE FILEPATH "" FORCE)
set(MBEDTLS_LIBRARIES mbedtls mbedx509 mbedcrypto)
set(MBEDTLS_FOUND TRUE CACHE BOOL "" FORCE)

# libssh
# https://git.libssh.org/projects/libssh.git
find_package(Threads REQUIRED)
set(WITH_MBEDTLS ON CACHE BOOL "" FORCE)
set(LIBSSH_STATIC ON CACHE BOOL "" FORCE)
set(UNIT_TESTING OFF CACHE BOOL "" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WITH_GSSAPI OFF CACHE BOOL "" FORCE)
set(WITH_NACL OFF CACHE BOOL "" FORCE)
set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
add_subdirectory(libssh EXCLUDE_FROM_ALL)

# Restore install rules for the main project
set(CMAKE_SKIP_INSTALL_RULES OFF)

target_link_libraries(main ssh Threads::Threads)
