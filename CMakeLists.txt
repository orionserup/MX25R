
cmake_minimum_required(VERSION 3.10)

if(IDF_VERSION_MAJOR GREATER_EQUAL 4)

    idf_component_register( SRCS_DIR src
                            INCLUDE_DIRS include)

else()

    project(MX25R LANGUAGES C DESCRIPTION "A Hardware agnostic driver for the Low-Power MX25 Series of NOR Flash Chips")

    file(GLOB SOURCES "src/*.c")

    add_library(${PROJECT_NAME} STATIC ${SOURCES})
    target_include_directories(${PROJECT_NAME} PUBLIC include)

    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

endif()