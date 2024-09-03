cmake_minimum_required(VERSION 3.10)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
project(GestureDrawing VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)

include(CTest)
enable_testing()
set(PRJ_HEADERS stb_image.h)
#set(PRJ_SOURCES)
set(PRJ_MAIN main.cpp)
#add_executable(${PROJECT_NAME} ${PRJ_HEADERS} ${PRJ_SOURCES} ${PRJ_MAIN})
#
# set(CMAKE_CXX_STANDARD 11) # CMAKE_ denotes a variable used by cmake
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} ${PRJ_HEADERS} ${PRJ_MAIN})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

find_package(glad CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE imgui::imgui)

find_package(OpenGL REQUIRED) # v3.10
target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)

#find_package(Stb REQUIRED)
#target_include_directories(${PROJECT_NAME} PRIVATE ${Stb_INCLUDE_DIR})
add_executable(demo demo.cpp)
find_package(glad CONFIG REQUIRED)
target_link_libraries(demo PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(demo PRIVATE glfw)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(demo PRIVATE imgui::imgui)

find_package(OpenGL REQUIRED) # v3.10
target_link_libraries(demo PRIVATE OpenGL::GL)


