cmake_minimum_required(VERSION 3.15)
project(SearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Используем vcpkg
set(VCPKG_ROOT "C:/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# Явно указываем пути к OpenSSL
set(OPENSSL_ROOT_DIR "${VCPKG_ROOT}/installed/x64-windows")
set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libcrypto.lib")
set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/lib/libssl.lib")

message(STATUS "OpenSSL Root: ${OPENSSL_ROOT_DIR}")
message(STATUS "OpenSSL Include: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL Crypto: ${OPENSSL_CRYPTO_LIBRARY}")
message(STATUS "OpenSSL SSL: ${OPENSSL_SSL_LIBRARY}")

# Находим пакеты
find_package(Boost REQUIRED COMPONENTS system)

# Ручная настройка OpenSSL
if(EXISTS "${OPENSSL_INCLUDE_DIR}" AND EXISTS "${OPENSSL_CRYPTO_LIBRARY}" AND EXISTS "${OPENSSL_SSL_LIBRARY}")
    message(STATUS "Using manually configured OpenSSL")
    set(OPENSSL_FOUND TRUE)
    set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
    set(OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
else()
    message(FATAL_ERROR "OpenSSL not found in vcpkg directory")
endif()

# Ручная настройка PostgreSQL
set(POSTGRESQL_INCLUDE_DIR "${VCPKG_ROOT}/installed/x64-windows/include")
set(POSTGRESQL_LIBRARY_DIR "${VCPKG_ROOT}/installed/x64-windows/lib")

find_library(PQ_LIBRARY 
    NAMES libpq pq
    PATHS ${POSTGRESQL_LIBRARY_DIR}
    NO_DEFAULT_PATH
    REQUIRED
)

find_library(PQXX_LIBRARY 
    NAMES libpqxx pqxx
    PATHS ${POSTGRESQL_LIBRARY_DIR}
    NO_DEFAULT_PATH
    REQUIRED
)

message(STATUS "PostgreSQL PQ: ${PQ_LIBRARY}")
message(STATUS "PostgreSQL PQXX: ${PQXX_LIBRARY}")

# Настройки компилятора
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_options(/wd4100 /wd4267 /wd4996)
endif()

# Spider executable
add_executable(spider
    src/main_spider.cpp
    src/config.cpp
    src/database.cpp
    src/html_parser.cpp
    src/advanced_html_parser.cpp
    src/threaded_spider.cpp
    src/http_client.cpp
)

target_include_directories(spider PRIVATE 
    include 
    ${POSTGRESQL_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(spider PRIVATE 
    Boost::system
    ${PQ_LIBRARY}
    ${PQXX_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ws2_32
    crypt32
    bcrypt
)

# Search Server executable
add_executable(search_server
    src/main_server.cpp
    src/config.cpp
    src/database.cpp
    src/html_parser.cpp
    src/advanced_html_parser.cpp
    src/beast_http_server.cpp
)

target_include_directories(search_server PRIVATE 
    include 
    ${POSTGRESQL_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(search_server PRIVATE 
    Boost::system
    ${PQ_LIBRARY}
    ${PQXX_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ws2_32
    crypt32
    bcrypt
)

# Копируем config.ini
configure_file(config.ini config.ini COPYONLY)