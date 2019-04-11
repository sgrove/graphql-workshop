#!/usr/bin/env sh

set -e

MIGRATION_NAME=$1

if [ -z "$MIGRATION_NAME" ]; then
    echo "Provide migration name as first argument. For example:"
    echo "./bin/create_blank_migration.sh ship_love"
    exit 1
fi

SCRIPT_PATH=$(dirname "$0")
cd "${SCRIPT_PATH}/.."

./gomigrate create -ext .sql -dir migrations/ $MIGRATION_NAME

echo "Blank migration files added to migrations/"
