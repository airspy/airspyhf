#!/bin/bash
V=$(git describe --dirty --always --tags)
echo git version: $V
echo writing version to libairspyhf/src/git_version.c

cat - <<EOF >libairspyhf/src/git_version.c
#include "git_version.h"
const char* git_version = "${V}";
EOF

echo "contents:"
cat libairspyhf/src/git_version.c
