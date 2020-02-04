#!/usr/bin/env bash

OPTIONS=(
  "--dump-line=yes"
  "--dump-instr=yes"
  "--compress-strings=no"
  "--compress-pos=no"
  "--collect-jumps=yes"
  "--collect-systime=yes"
)

TOOL="valgrind --tool=callgrind"
BIN="./rex"

${TOOL} ${OPTIONS[*]} ${BIN}
