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

macro(test name sourcefile)
    add_library(${name} SHARED "${CMAKE_SOURCE_DIR}/testmodules/${sourcefile}")
    target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    set_target_properties(${name} PROPERTIES PREFIX "")
endmacro()

test(dspmod0 0.c)
test(dspmod1 1.c)
test(dspmod2 2.c)
test(dspmod3 3.c)
test(dspmod4 4.c)
test(dspmod5 5.c)