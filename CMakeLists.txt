cmake_minimum_required(VERSION 3.20)
project(bread_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(bread_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(bread_UTILS ${CMAKE_CURRENT_SOURCE_DIR}/utils)
set(bread_DEPENDENCIES ${CMAKE_CURRENT_SOURCE_DIR}/dependencies)

option(bread_USE_TB "use endgame tablebase" ON)
option(bread_EMBED_NN "embed neural network in the executable" ON)
set(bread_EMBED_NN OFF)
message(STATUS "use endgame tablebase: ${bread_USE_TB}")
message(STATUS "embed neural network in executable: ${bread_EMBED_NN}")

# this path is relative to the executable. It can be changed if needed
set(bread_NNUE_MODEL_PATH
    "${bread_DEPENDENCIES}/model_NNUE_2-40960_2x256_8_8_1_quant_layers"
)

# this path is relative to the executable. It can be changed if needed
set(bread_TB_PATH
    "3-4-5_pieces_Syzygy/3-4-5"
)

set(bread_NN_HEADER_PATH
    "${bread_DEPENDENCIES}/neural_network.hpp"
)

set(bread_INCLUDE_DIRS
    ${bread_DEPENDENCIES}
    ${bread_DEPENDENCIES}/extern
    ${bread_DEPENDENCIES}/unit_tests
)

if(bread_USE_TB)
  set(bread_INCLUDE_DIRS ${bread_INCLUDE_DIRS} ${bread_DEPENDENCIES}/extern/Fathom/src)
endif(bread_USE_TB)

set(bread_SOURCE
    ${bread_SRC}/search_board.cpp
    ${bread_SRC}/nnue/nnue.cpp
    ${bread_SRC}/nnue/nnue_board.cpp
    ${bread_SRC}/transposition_table.cpp
    ${bread_SRC}/bread_engine_core.cpp
)

if(bread_USE_TB)
  set(bread_SOURCE ${bread_SOURCE} ${bread_DEPENDENCIES}/extern/Fathom/src/tbprobe.cpp)
endif(bread_USE_TB)


function(bread_configure bread_TARGET)
    cmake_parse_arguments(bread "" "" "SOURCES" ${ARGN})

    add_executable(${bread_TARGET} ${bread_SOURCES})

    target_include_directories(${bread_TARGET} PRIVATE ${bread_INCLUDE_DIRS})

    target_compile_definitions(${bread_TARGET} PRIVATE bread_USE_TB=$<BOOL:${bread_USE_TB}>)
    
    target_compile_definitions(${bread_TARGET} PRIVATE bread_EMBED_NN=$<BOOL:${bread_EMBED_NN}>)
    
    if (TARGET generate_neural_net_header)
        target_compile_definitions(${bread_TARGET} PRIVATE bread_EMBED_NN=false)
    endif()

    target_compile_definitions(${bread_TARGET} PRIVATE bread_TB_PATH="${bread_TB_PATH}")

    target_compile_definitions(${bread_TARGET} PRIVATE bread_NN_HEADER_PATH="${bread_NN_HEADER_PATH}")

    target_compile_definitions(${bread_TARGET} PRIVATE bread_NNUE_MODEL_PATH="${bread_NNUE_MODEL_PATH}")
endfunction(bread_configure)

bread_configure(release SOURCES
    ${bread_SOURCE}
    ${bread_SRC}/uci.cpp
    ${bread_SRC}/run_uci.cpp
)

bread_configure(benchmark_engine SOURCES
    ${bread_SOURCE}
    ${bread_UTILS}/benchmark_engine.cpp
)

bread_configure(tests SOURCES
    ${bread_SOURCE}
    ${bread_UTILS}/unit_tests.cpp
)

bread_configure(parameter_tuning SOURCES
    ${bread_SOURCE}
    ${bread_UTILS}/parameter_optimisation.cpp
)

bread_configure(generate_neural_net_header SOURCES
    ${bread_SOURCE}
    ${bread_UTILS}/generate_nn_header.cpp
)