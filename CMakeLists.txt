# =============================================
cmake_minimum_required(VERSION 3.23)
project(loaddsp LANGUAGES C)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

#set(CMAKE_C_STANDARD 99)
#set(CMAKE_C_STANDARD_REQUIRED ON)
#set(CMAKE_C_EXTENSIONS OFF)

# =============================================

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

# =============================================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})
unset(SOURCES)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")

pkg_check_modules(PipeWire REQUIRED IMPORTED_TARGET libpipewire-0.3)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::PipeWire)

# =============================================

install(DIRECTORY "include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# =============================================