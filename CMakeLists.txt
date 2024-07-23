cmake_minimum_required(VERSION 3.13)
project("service")

find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL)

include_directories(${PostgreSQL_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIRS})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cpphttp/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/soci/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/jwt-cpp/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-core/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-common/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-identity/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-blobs/include")
include_directories(${CURL_INCLUDE_DIRS})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libxml2/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/liblzma/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/dependencies/zlib/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

file (GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/cpphttp/src/request.cpp")

add_executable(assignment ${SOURCES})

set_target_properties(assignment PROPERTIES CXX_STANDARD 20)

target_link_libraries(assignment 
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/soci/lib/libsoci_postgresql.a
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/soci/lib/libsoci_core.a
${PostgreSQL_LIBRARIES}
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-blobs/lib/libazure-storage-blobs.a
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-common/lib/libazure-storage-common.a
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-core/lib/libazure-core.a
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/azure-identity/lib/libazure-identity.a
${CURL_LIBRARIES}
${OPENSSL_LIBRARIES}
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/libxml2/lib/libxml2.a
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/liblzma/lib/liblzma.a
${CMAKE_CURRENT_SOURCE_DIR}/dependencies/zlib/lib/libz.a
)

set_property(TARGET assignment PROPERTY CXX_STANDARD 20)