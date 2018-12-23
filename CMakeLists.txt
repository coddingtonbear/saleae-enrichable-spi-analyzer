cmake_minimum_required (VERSION 3.11)
project(scriptable_spi_analyzer)

# custom CMake Modules are located in the cmake directory.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(ExternalAnalyzerSDK)

set(SOURCES 
src/ScriptableSpiAnalyzer.cpp
src/ScriptableSpiAnalyzer.h
src/ScriptableSpiAnalyzerResults.cpp
src/ScriptableSpiAnalyzerResults.h
src/ScriptableSpiAnalyzerSettings.cpp
src/ScriptableSpiAnalyzerSettings.h
src/ScriptableSpiSimulationDataGenerator.cpp
src/ScriptableSpiSimulationDataGenerator.h
)

add_analyzer_plugin(scriptable_spi_analyzer SOURCES ${SOURCES})
