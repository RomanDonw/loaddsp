# =============================================
cmake_minimum_required(VERSION 3.23)
project(loaddsp LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # required for POSIX.1-2008 extension that required by PipeWire SPA.

option(USE_GLIBC_DLMOPEN "Use safe glibc 'dlmopen' function" ON)
option(BUILD_EXAMPLES "Build example modules" OFF)
option(BUILD_TESTS "Build test modules" ON)

# =============================================

include(GNUInstallDirs)
find_package(PkgConfig REQUIRED)

# =============================================

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.h" "src/*.c")
add_executable(${PROJECT_NAME} ${SOURCES})
unset(SOURCES)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/include")
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<BOOL:${USE_GLIBC_DLMOPEN}>:LOADDSP_USEGLIBCDLMOPEN>)

pkg_check_modules(PipeWire REQUIRED IMPORTED_TARGET libpipewire-0.3)
target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::PipeWire)

# =============================================

install(DIRECTORY "include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# =============================================

macro(mod name sourcefile)
    add_library(${name} SHARED "${CMAKE_CURRENT_SOURCE_DIR}/${sourcefile}")
    target_include_directories(${name} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    set_target_properties(${name} PROPERTIES PREFIX "")
endmacro()

if(BUILD_EXAMPLES)
    set(CMAKE_CURRENT_SOURCE_DIR "examples/")
    include("examples/CMakeLists.txt")
endif()

if(BUILD_TESTS)
    set(CMAKE_CURRENT_SOURCE_DIR "tests/")
    include("tests/CMakeLists.txt")
endif()