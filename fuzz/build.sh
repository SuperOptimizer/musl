#!/bin/bash
# Build script for musl fuzz targets
# Strategy: Build musl with prefixed symbols to avoid glibc conflicts

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MUSL_DIR="$(dirname "$SCRIPT_DIR")"
VARIANT="${1:-nosan}"

# Compiler setup
AFL_CLANG_LTO="${AFL_CLANG_LTO:-/usr/local/bin/afl-clang-lto}"
AFL_CLANG_LTO_PP="${AFL_CLANG_LTO_PP:-/usr/local/bin/afl-clang-lto++}"

# Common flags for musl build (NO AFL instrumentation)
MUSL_CFLAGS="-w -Wno-error -g3 -march=native -fno-omit-frame-pointer -O2"
MUSL_LDFLAGS="-fuse-ld=lld"

# Common flags for harness build (WITH AFL instrumentation)
BASE_CFLAGS="-w -Wno-error -g3 -march=native -fno-omit-frame-pointer -O3 -flto=full"
BASE_LDFLAGS="-fuse-ld=lld -Wl,--threads=32 -flto=full"

# Variant-specific flags for harness
case "$VARIANT" in
    nosan)
        SANITIZER_FLAGS=""
        export AFL_USE_ASAN=
        export AFL_USE_UBSAN=
        ;;
    asan_ubsan)
        SANITIZER_FLAGS="-fsanitize=address,undefined -fsanitize-address-use-after-return=always -fsanitize-address-use-after-scope -fno-sanitize-recover=undefined"
        export AFL_USE_ASAN=1
        export AFL_USE_UBSAN=1
        export AFL_UBSAN_VERBOSE=1
        ;;
    laf)
        SANITIZER_FLAGS=""
        export AFL_LLVM_LAF_ALL=1
        ;;
    redqueen)
        SANITIZER_FLAGS=""
        export AFL_LLVM_CMPLOG=1
        ;;
    sand_asan_ubsan)
        SANITIZER_FLAGS="-fsanitize=address,undefined -fsanitize-address-use-after-return=always -fsanitize-address-use-after-scope -fno-sanitize-recover=undefined"
        export AFL_SAN_NO_INST=1
        export AFL_UBSAN_VERBOSE=1
        ;;
    *)
        echo "Unknown variant: $VARIANT"
        echo "Available: nosan, asan_ubsan, laf, redqueen, sand_asan_ubsan"
        exit 1
        ;;
esac

HARNESS_CFLAGS="$BASE_CFLAGS $SANITIZER_FLAGS"
HARNESS_LDFLAGS="$BASE_LDFLAGS $SANITIZER_FLAGS"

BUILD_DIR="$MUSL_DIR/build_$VARIANT"
BIN_DIR="$BUILD_DIR/bin"
MUSL_BUILD_DIR="$MUSL_DIR/build_musl_base"

echo "[BUILD] musl/$VARIANT - Setting up..."

mkdir -p "$BUILD_DIR" "$BIN_DIR"

# Build musl as a static library (only once, shared across variants)
if [ ! -f "$MUSL_BUILD_DIR/lib/libc_prefixed.a" ]; then
    echo "[BUILD] Building musl static library (shared across variants)..."
    mkdir -p "$MUSL_BUILD_DIR"
    cd "$MUSL_BUILD_DIR"

    if [ ! -f "lib/libc.a" ]; then
        CC="clang" \
        CFLAGS="$MUSL_CFLAGS" \
        LDFLAGS="$MUSL_LDFLAGS" \
        "$MUSL_DIR/configure" \
            --prefix="$MUSL_BUILD_DIR/install" \
            --disable-shared \
            --enable-static \
            --disable-optimize

        make -j$(nproc)
    fi

    # Create a prefixed version of libc.a
    echo "[BUILD] Creating prefixed musl library..."
    cp lib/libc.a lib/libc_prefixed.a

    # Prefix all global symbols with musl_
    llvm-objcopy --prefix-symbols=musl_ lib/libc_prefixed.a
fi

echo "[BUILD] musl/$VARIANT - Building fuzz targets..."

# Build each fuzzer
FUZZ_SOURCES=(
    "fuzz-regex"
    "fuzz-printf"
    "fuzz-scanf"
    "fuzz-strptime"
    "fuzz-multibyte"
    "fuzz-fnmatch"
    "fuzz-strtod"
    "fuzz-glob"
)

for fuzz in "${FUZZ_SOURCES[@]}"; do
    echo "[BUILD] musl/$VARIANT - Building $fuzz..."

    # Compile harness with AFL
    # Use -DMUSL_PREFIX to enable musl_ prefixed function calls
    "$AFL_CLANG_LTO" $HARNESS_CFLAGS \
        -DMUSL_PREFIX \
        -c "$SCRIPT_DIR/$fuzz.c" \
        -o "$BUILD_DIR/$fuzz.o"

    # Link with prefixed musl and system libraries
    "$AFL_CLANG_LTO" $HARNESS_LDFLAGS \
        "$BUILD_DIR/$fuzz.o" \
        "$MUSL_BUILD_DIR/lib/libc_prefixed.a" \
        -lm -lpthread \
        -o "$BIN_DIR/$fuzz"
done

echo "[BUILD] musl/$VARIANT - Done"
ls -la "$BIN_DIR"

# Test one of the built binaries
echo ""
echo "[TEST] Quick sanity check..."
echo "test" | timeout 2 "$BIN_DIR/fuzz-strtod" /dev/stdin 2>/dev/null && echo "fuzz-strtod: OK" || echo "fuzz-strtod: needs input file"
