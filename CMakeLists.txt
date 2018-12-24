cmake_minimum_required (VERSION 3.11)
project(enrichable_spi_analyzer)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/EnrichableSpiAnalyzer.cpp
src/EnrichableSpiAnalyzer.h
src/EnrichableSpiAnalyzerResults.cpp
src/EnrichableSpiAnalyzerResults.h
src/EnrichableSpiAnalyzerSettings.cpp
src/EnrichableSpiAnalyzerSettings.h
src/EnrichableSpiSimulationDataGenerator.cpp
src/EnrichableSpiSimulationDataGenerator.h
)

add_analyzer_plugin(enrichable_spi_analyzer SOURCES ${SOURCES})
