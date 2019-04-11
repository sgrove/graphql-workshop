#!/usr/bin/env sh

set -e

DB_URL=$1
if [ -z "$DB_URL" ]; then
    echo "Provide database url as first argument. For example:"
    echo "./bin/run_migrations.sh 'postgres://localhost:5432/cc_local?user=postgres&password=&sslmode=disable'"
    exit 1
fi

SCRIPT_PATH=$(dirname "$0")
cd "${SCRIPT_PATH}/.."

./gomigrate -path migrations -database $DB_URL up
