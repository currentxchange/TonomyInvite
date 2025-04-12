#!/bin/bash

BUILD_METHOD=$1

PARENT_PATH=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "${PARENT_PATH}"

source ../compile_contract.sh

compile_contract "${PARENT_PATH}" "demo.tmy" "${BUILD_METHOD}"
