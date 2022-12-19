cmake_minimum_required(VERSION 3.10)

project(MX25R LANGUAGES C DESCRIPTION "A Hardware agnostic driver for the Low-Power MX25 Series of NOR Flash Chips")

file(GLOB SOURCES "src/*.c")

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC include)