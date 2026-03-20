cmake_minimum_required(VERSION 3.20)
project(AtividadesCG VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# GLFW is fetched automatically so the project builds on a clean machine.
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

add_executable(Hello3D src/Hello3D.cpp)

target_link_libraries(Hello3D PRIVATE glfw)

if(APPLE)
  target_link_libraries(Hello3D PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(WIN32)
  target_link_libraries(Hello3D PRIVATE opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(Hello3D PRIVATE OpenGL::GL)
endif()
