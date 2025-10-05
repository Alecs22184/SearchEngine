cmake_minimum_required(VERSION 3.15)
project(SearchEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Отключаем все дополнительные анализаторы
set(CMAKE_VS_GLOBALS 
    "EnableCppBuildInsights=false"
    "TrackFileAccess=false"
)

# Явно указываем путь к vcpkg
set(VCPKG_ROOT "C:/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# Находим Boost
find_package(Boost REQUIRED COMPONENTS system)

# Ручная настройка PostgreSQL - используем пути vcpkg
set(POSTGRESQL_INCLUDE_DIR "${VCPKG_ROOT}/installed/x64-windows/include")
set(POSTGRESQL_LIBRARY_DIR "${VCPKG_ROOT}/installed/x64-windows/lib")

# Ищем библиотеки вручную
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

if(PQ_LIBRARY AND PQXX_LIBRARY)
    message(STATUS "Found PostgreSQL libraries:")
    message(STATUS "  PQ: ${PQ_LIBRARY}")
    message(STATUS "  PQXX: ${PQXX_LIBRARY}")
else()
    message(FATAL_ERROR "PostgreSQL libraries not found")
endif()

# Настройки компилятора
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_options(/wd4100)  # Неиспользованный параметр
    add_compile_options(/wd4267)  # Преобразование size_t в int
    add_compile_options(/wd4996)  # Небезопасные функции
else()
    add_compile_options(-Wall -Wextra -Wpedantic -pthread)
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

target_include_directories(spider PRIVATE include ${POSTGRESQL_INCLUDE_DIR})
target_link_libraries(spider 
    PRIVATE 
        Boost::system
        ${PQ_LIBRARY}
        ${PQXX_LIBRARY}
        ws2_32
        crypt32
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

target_include_directories(search_server PRIVATE include ${POSTGRESQL_INCLUDE_DIR})
target_link_libraries(search_server
    PRIVATE 
        Boost::system
        ${PQ_LIBRARY}
        ${PQXX_LIBRARY}
        ws2_32
        crypt32
)

# Копируем config.ini в выходную директорию
configure_file(config.ini config.ini COPYONLY)