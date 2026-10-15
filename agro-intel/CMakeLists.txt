cmake_minimum_required(VERSION 3.20)
project(agro-intel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(SQLite3 REQUIRED)

# Biblioteca con toda la lógica: así main.cpp y los tests la comparten
add_library(agro_core
  src/storage/Database.cpp
  src/ingestion/CsvPriceImporter.cpp
)
target_include_directories(agro_core PUBLIC include src)
# CMake >= 4.3 usa SQLite3::SQLite3; versiones anteriores, SQLite::SQLite3
if(TARGET SQLite3::SQLite3)
  target_link_libraries(agro_core PUBLIC SQLite3::SQLite3)
else()
  target_link_libraries(agro_core PUBLIC SQLite::SQLite3)
endif()
target_compile_options(agro_core PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

add_executable(agro-intel src/main.cpp)
target_link_libraries(agro-intel PRIVATE agro_core)
