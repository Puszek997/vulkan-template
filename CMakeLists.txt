cmake_minimum_required(VERSION 4.4.2)

project(
    vulkan-template
    VERSION      0.1.0
    DESCRIPTION  ""
    HOMEPAGE_URL ""
    LANGUAGES    CXX
)

add_subdirectory(third-party)

add_executable(${PROJECT_NAME})

include(CheckPIESupported)
check_pie_supported(OUTPUT_VARIABLE pie_output LANGUAGES CXX)
if(NOT CMAKE_CXX_LINK_PIE_SUPPORTED)
    message(
        WARNING
        "PIE is not supported at link time: ${pie_output}"
        "PIE link options will not be passed to linker"
    )
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT ipo_output LANGUAGES CXX)
if(result)
    set_property(
        TARGET
        ${PROJECT_NAME}
        PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
    )
else()
    message(
        WARNING
        "IPO is not supported: ${ipo_output}"
        "IPO link options will not be passed to linker"
    )
endif()

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    CXX_STANDARD              26
    CXX_STANDARD_REQUIRED     ON
    CXX_EXTENSIONS            OFF
    CXX_VISIBILITY_PRESET     hidden
    VISIBILITY_INLINES_HIDDEN ON
    COMPILE_WARNING_AS_ERROR  ON
    POSITION_INDEPENDENT_CODE ON
    CXX_MODULE_STD            ON
)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
    FILE_SET SOURCES
    BASE_DIRS
        src
    FILES
        src/main.cpp
)

target_compile_options(
    ${PROJECT_NAME}
    PRIVATE
    ${compile_definitions}
    ${compile_flags}
    ${compile_warnings}
)

target_link_options(
    ${PROJECT_NAME}
    PRIVATE
)

# https://gitlab.kitware.com/cmake/cmake/-/issues/24629
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    vulkan-module
)

include(GNUInstallDirs)
install(
    TARGETS
    ${PROJECT_NAME}
    RUNTIME DESTINATION "$<CONFIG>/${CMAKE_INSTALL_BINDIR}"
)
