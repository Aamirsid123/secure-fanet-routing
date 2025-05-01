# Make sure to explicitly use Homebrew OpenSSL instead of Anaconda
set(CMAKE_PREFIX_PATH "/opt/homebrew/Cellar/openssl@3/3.4.1" ${CMAKE_PREFIX_PATH})
add_definitions(-DOPENSSL_API_COMPAT=0x10100000L)
# Find OpenSSL package
find_package(OpenSSL REQUIRED)
include_directories(BEFORE ${OPENSSL_INCLUDE_DIR})

build_lib(
  LIBNAME ecc-security
  SOURCE_FILES
    model/ecc-crypto.cc
    model/secure-socket.cc
    model/secure-socket-helper.cc
  HEADER_FILES
    model/ecc-crypto.h
    model/ecc-key-exchange.h
    model/secure-socket.h
    model/secure-socket-helper.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${libapplications}
    ${libinternet}
    ${OPENSSL_LIBRARIES}
)
