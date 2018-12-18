cmake_minimum_required(VERSION 2.8)

project(fec CXX)

set(CMAK_CXX_COMPILER g++)
set(CXX_FLAG -g -Wall -Os)
add_definitions(-std=c++11)

set(PROJECT_DIR ..)
include_directories(${PROJECT_DIR}/src)
include_directories(${PROJECT_DIR}/third_party/openfec)

set(ENCODER_SRC ${PROJECT_DIR}/src/encoder_unittest.cc ${SOURCE_DIR}/src/fec_encoder.cc)
add_executable(FecEncoder ${ENCODER_SRC})
target_link_libraries(FecEncoder openfec)

set(DECODER_SRC ${PROJECT_DIR}/src/decoder_unittest.cc ${PROJECT_DIR}/src/fec_decoder.cc ${PROJECT_DIR}/src/fec_packet.cc)
add_executable(FecDecoder ${DECODER_SRC})
target_link_libraries(FecDecoder openfec pthread)
