#!/bin/bash

BUILD_METHOD=$1

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}"

CONTRACT_NAME="tonomy"

if [ "$BUILD_METHOD" == "local" ]; then
    WORKING_DIR="."
else
    WORKING_DIR="/contracts"
fi

BUILD_COMMAND="cdt-cpp -abigen -I ${WORKING_DIR}/include -R ${WORKING_DIR}/ricardian -contract ${CONTRACT_NAME} -o ${WORKING_DIR}/${CONTRACT_NAME}.wasm ${WORKING_DIR}/src/${CONTRACT_NAME}.cpp ${WORKING_DIR}/src/native.cpp"

echo $BUILD_COMMAND

mkdir -p "${PARENT_PATH}/include/eosio.tonomy"
cp "${PARENT_PATH}/../eosio.tonomy/include/eosio.tonomy/eosio.tonomy.hpp" "${PARENT_PATH}/include/eosio.tonomy/eosio.tonomy.hpp"

if [ "$BUILD_METHOD" == "local" ]; then
    bash -c "${BUILD_COMMAND}"
else
    docker run -v "${PARENT_PATH}:${WORKING_DIR}" antelope_blockchain bash -c "${BUILD_COMMAND}"
fi