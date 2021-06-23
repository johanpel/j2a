cmake_minimum_required(VERSION 3.16)

project(jsontest
        VERSION 0.0.0
        DESCRIPTION "An example project"
        LANGUAGES CXX
        )

set(CMAKE_CXX_STANDARD 17)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if (COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif ()

find_package(Arrow 3.0.0 CONFIG REQUIRED)

########################################################################################################## CompileUnits
include(FetchContent)
FetchContent_Declare(cmake-modules
        GIT_REPOSITORY https://github.com/abs-tudelft/cmake-modules.git
        GIT_TAG master
        )
FetchContent_MakeAvailable(cmake-modules)
include(CompileUnits)

########################################################################################################## ANTLR4
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/antlr4)
add_definitions(-DANTLR4CPP_STATIC) # required if linking to static library
include(ExternalAntlr4Cpp)  # add external build for antlrcpp
# set variable pointing to the antlr tool that supports C++
# this is not required if the jar file can be found under PATH environment
set(ANTLR_EXECUTABLE /usr/local/lib/antlr-4.9-complete.jar)

find_package(ANTLR REQUIRED) # add macros to generate ANTLR Cpp code from grammar

antlr_target(Battery Battery.g4
        LEXER
        PARSER
        VISITOR
        LISTENER)

message("ANTLR4 Include dirs:")
foreach (I ${ANTLR4_INCLUDE_DIRS})
    message("  ${I}")
endforeach ()
message("ANTLR4 Input:")
foreach (I ${ANTLR_Battery_INPUT})
    message("  ${I}")
endforeach ()
message("ANTLR4 Outputs:")
foreach (I ${ANTLR_Battery_OUTPUTS})
    message("  ${I}")
endforeach ()
message("ANTLR4 CXX Outputs:")
foreach (I ${ANTLR_Battery_CXX_OUTPUTS})
    message("  ${I}")
endforeach ()
message("ANTLR4 Output dir:")
foreach (I ${ANTLR_Battery_OUTPUT_DIR})
    message("  ${I}")
endforeach ()

include_directories(${ANTLR_Battery_OUTPUT_DIR})
include_directories(${ANTLR4_INCLUDE_DIRS}) # add antrl4cpp artifacts to project environment

########################################################################################################## spdlog
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog
        GIT_TAG v1.8.2
        )
FetchContent_MakeAvailable(spdlog)

########################################################################################################## putong
FetchContent_Declare(putong
        GIT_REPOSITORY https://github.com/abs-tudelft/putong.git
        GIT_TAG master)
FetchContent_MakeAvailable(putong)

########################################################################################################## simdjson
FetchContent_Declare(
        simdjson
        GIT_REPOSITORY https://github.com/simdjson/simdjson.git
        GIT_TAG v0.9.0
        GIT_SHALLOW TRUE)
set(SIMDJSON_JUST_LIBRARY ON CACHE INTERNAL "")
set(SIMDJSON_BUILD_STATIC ON CACHE INTERNAL "")
set(SIMDJSON_ENABLE_THREADS ON CACHE INTERNAL "")
FetchContent_MakeAvailable(simdjson)

########################################################################################################## RapidJSON
FetchContent_Declare(
        rapidjson
        GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
        GIT_TAG v1.1.0)
FetchContent_GetProperties(rapidjson)
if (NOT rapidjson_POPULATED)
    FetchContent_Populate(rapidjson)
    include_directories("${rapidjson_SOURCE_DIR}/include")
endif ()

########################################################################################################## illex
FetchContent_Declare(
        illex
        GIT_REPOSITORY https://github.com/teratide/illex.git
        GIT_TAG v0.1.0)
FetchContent_MakeAvailable(illex)

########################################################################################################## Spirit
FetchContent_Declare(
        spirit
        GIT_REPOSITORY https://github.com/boostorg/spirit.git
        GIT_TAG boost-1.75.0)
FetchContent_GetProperties(spirit)
if (NOT spirit_POPULATED)
    FetchContent_Populate(spirit)
    include_directories("${spirit_SOURCE_DIR}/include")
endif ()

########################################################################################################## CLI11
FetchContent_Declare(
        CLI11
        GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
        GIT_TAG v1.9.1)
FetchContent_MakeAvailable(CLI11)

########################################################################################################## fmt
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 8.0.0)
FetchContent_MakeAvailable(fmt)

########################################################################################################## targets

add_compile_unit(
        NAME jsontest
        TYPE EXECUTABLE
        SRCS
        src/main.cpp
        ${ANTLR_Battery_CXX_OUTPUTS}
        # src/schema.cpp
        TSTS
        test/test_arrow.cpp
        DEPS
        simdjson
        arrow_shared
        spdlog::spdlog
        illex::static
        antlr4_static
        CLI11::CLI11
        fmt::fmt-header-only
)

compile_units()