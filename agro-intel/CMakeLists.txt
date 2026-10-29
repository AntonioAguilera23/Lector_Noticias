cmake_minimum_required(VERSION 3.15)
project(agro_intel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(AGRO_ENABLE_PDF_IMPORT "Habilita la importación de boletines PDF mediante PDFium" OFF)

find_package(SQLite3 REQUIRED)

set(AGRO_CORE_SOURCES
    src/storage/Database.cpp
    src/storage/IPriceRepository.cpp
    src/ingestion/CsvPriceImporter.cpp
    src/analysis/MarketTrendAnalyzer.cpp
)

if(AGRO_ENABLE_PDF_IMPORT)
    find_package(pdfium CONFIG REQUIRED)
    list(APPEND AGRO_CORE_SOURCES src/ingestion/MapaBulletinImporter.cpp)
endif()

add_library(agro_core ${AGRO_CORE_SOURCES})
target_include_directories(agro_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(agro_core PUBLIC SQLite::SQLite3)

if(AGRO_ENABLE_PDF_IMPORT)
    if(TARGET pdfium)
        target_link_libraries(agro_core PUBLIC pdfium)
    elseif(TARGET PDFium::PDFium)
        target_link_libraries(agro_core PUBLIC PDFium::PDFium)
    else()
        message(FATAL_ERROR
            "PDFium se encontró, pero no exporta el target 'pdfium' ni 'PDFium::PDFium'. "
            "Ajusta el nombre del target según el paquete instalado.")
    endif()
    target_compile_definitions(agro_core PUBLIC AGRO_ENABLE_PDF_IMPORT=1)
endif()

add_executable(agro_tracker src/main.cpp)
set_target_properties(agro_tracker PROPERTIES OUTPUT_NAME "agro-intel")
target_link_libraries(agro_tracker PRIVATE agro_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(agro_intel_tests tests/DatabaseRepositoryTests.cpp)
    target_link_libraries(agro_intel_tests PRIVATE agro_core)
    add_test(NAME agro_intel_database_repository COMMAND agro_intel_tests)
endif()