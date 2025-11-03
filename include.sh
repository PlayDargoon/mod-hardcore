#!/usr/bin/env bash

## GETS THE CURRENT MODULE ROOT DIRECTORY
MOD_HARDCORE_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/" && pwd )"

source "$MOD_HARDCORE_ROOT/conf/conf.sh.dist"

if [ -f "$MOD_HARDCORE_ROOT/conf/conf.sh" ]; then
    source "$MOD_HARDCORE_ROOT/conf/conf.sh"
fi
