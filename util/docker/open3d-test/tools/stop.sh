#!/bin/bash

REAL_PATH=$(dirname $(realpath ${0}))

. ${REAL_PATH}/set_variables.sh

echo "stopping ${CONTAINER_NAME}..."
docker container stop -t 0 ${CONTAINER_NAME} >/dev/null 2>&1
