#!/bin/bash
# This script should be run from the root directory of the amazon-kinesis-video-streams-rtp repo.

if [[ ! -d source ]]; then
    echo "Please run this script from the root directory of the amazon-kinesis-video-streams-rtp repo."
    exit 1
fi

UNIT_TEST_DIR="test/unit-test"
BUILD_DIR="${UNIT_TEST_DIR}/build"

# Create the build directory using CMake:
rm -rf ${BUILD_DIR}/
cmake -S ${UNIT_TEST_DIR} -B ${BUILD_DIR}/ -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_CLONE_SUBMODULES=ON -DCMAKE_C_FLAGS='--coverage -Wall -Wextra -Werror -DNDEBUG -DLIBRARY_LOG_LEVEL=LOG_DEBUG'

# Create the executables:
make -C ${BUILD_DIR}/ all

pushd ${BUILD_DIR}/
# Run the tests for all units
ctest -E system --output-on-failure
popd

# Calculate the coverage
make -C ${BUILD_DIR}/ coverage
