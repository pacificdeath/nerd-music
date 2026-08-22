#!/usr/bin/bash
set -e  # exit on any error
shopt -s expand_aliases

DEBUG=true

for arg in "$@"; do
    case $arg in
        --release) DEBUG=false ;;
    esac
done

catch_errors() {
    local code=$?
    if [ $code -ne 0 ]; then
        log "Failed horribly ($code)" 31
        exit $code
    fi
}

OUTPUT_EXE="./nerd-music.exe"
INPUT_C="./main.c"

ARGS=()
if $DEBUG; then
    ARGS+=("-g" "-DDEBUG" "-Wall" "-O0" "-fsanitize=address")
else
    ARGS+=("-O2")
fi

ARGS+=(
    "-o" "$OUTPUT_EXE"
    "$INPUT_C"
    "-std=c99"
    "-I./raylib/include/"
    "-L./raylib/lib/"
    "-l:libraylib.a"
    "-lGL"
    "-lX11"
    "-lm"
)

gcc "${ARGS[@]}"
catch_errors

"$OUTPUT_EXE"
catch_errors
