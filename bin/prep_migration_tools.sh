#!/usr/bin/env bash

set -x
set -e

SCRIPT_PATH=$(dirname "$0")
cd "${SCRIPT_PATH}/.."

platform='unknown'
unamestr=`uname`
if [[ "$unamestr" == 'Linux' ]]; then
   platform='linux'
elif [[ "$unamestr" == 'Darwin' ]]; then
   platform='darwin'
fi

curl -L -O https://storage.googleapis.com/one-graph-migrations/go-migrate/migrate.$platform-amd64.tar.gz
tar -xzvf migrate.$platform-amd64.tar.gz
mv migrate.$platform-amd64 gomigrate
rm migrate.*.tar.gz
./gomigrate -version
