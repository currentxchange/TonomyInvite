#!/bin/bash

function compile_contract {
    PARENT_PATH=$1
    CONTRACT_NAME=$2
    BUILD_METHOD=$3

    if [ "$BUILD_METHOD" == "local" ]; then
        WORKING_DIR="."
    else
        WORKING_DIR="/contracts"
    fi
    
    BUILD_COMMAND="cdt-cpp -abigen -I ${WORKING_DIR}/include -R ${WORKING_DIR}/ricardian -contract ${CONTRACT_NAME} -o ${WORKING_DIR}/${CONTRACT_NAME}.wasm ${WORKING_DIR}/src/${CONTRACT_NAME}.cpp"
    echo $BUILD_COMMAND

    if [ "$BUILD_METHOD" == "local" ]; then
        bash -c "${BUILD_COMMAND}"
    else
        docker run -v "${PARENT_PATH}:${WORKING_DIR}" antelope_blockchain bash -c "${BUILD_COMMAND}"
    fi
}