cmake_minimum_required(VERSION 3.20)
project(AtividadesCG VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)
FetchContent_MakeAvailable(stb)

add_executable(Hello3D src/Hello3D.cpp)

target_include_directories(Hello3D PRIVATE ${stb_SOURCE_DIR})
target_link_libraries(Hello3D PRIVATE glfw glm::glm)

if(APPLE)
  target_link_libraries(Hello3D PRIVATE "-framework OpenGL" "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
elseif(WIN32)
  target_link_libraries(Hello3D PRIVATE opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(Hello3D PRIVATE OpenGL::GL)
endif()
