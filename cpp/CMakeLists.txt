cmake_minimum_required(VERSION 3.16)
project(polysleep_acq LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_SIMULATOR "Usar driver simulado ADS1256" ON)

add_library(acq_core
    src/ads1256_driver.cpp
    $<$<NOT:$<BOOL:${ENABLE_SIMULATOR}>>:src/ads1256_hw.cpp>
    src/data_server.cpp
)
target_include_directories(acq_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(acq_core PRIVATE $<$<BOOL:${ENABLE_SIMULATOR}>:ADS1256_SIMULATOR>)

add_executable(adc_daemon src/main.cpp)
target_link_libraries(adc_daemon PRIVATE acq_core pthread)

# Probe tool (somente em modo hardware)
if (NOT ENABLE_SIMULATOR)
    add_executable(ads1256_probe src/spi_probe.cpp)
    target_link_libraries(ads1256_probe PRIVATE acq_core pthread)
endif()

install(TARGETS adc_daemon RUNTIME DESTINATION bin)
