#!/bin/bash

# Check if we're running on Windows (this is mostly a trick to get the script
# working on the Travis-CI Windows slaves that run Git bash).
function is_windows() {
    if [ "$(expr substr $(uname -s) 1 5)" == "MINGW" ]; then
        return 0
    fi
    return 1
}

function cleanup() {
    [[ ! -z "${PROJDIR}" ]] && rm -rf "${PROJDIR}"
    [[ ! -z "${BUILDCACHE_DIR}" ]] && rm -rf "${BUILDCACHE_DIR}"
}

# Treat errors with exit code 1.
function trap_handler() {
    MYSELF="$0"
    LASTLINE="$1"
    LASTERR="$2"
    echo "ERROR: ${MYSELF}: line ${LASTLINE}: exit status of last command: ${LASTERR}"

    cleanup
    exit "${LASTERR}"
}
trap 'trap_handler ${LINENO} ${$?}' ERR INT TERM

# Set up a working environment for Windows if needed.
EXESUFFIX=
if is_windows; then
    EXESUFFIX=".exe"
fi

# Note: This script is expected to be run from the BuildCache build folder.
BUILDCACHEDIR="$( realpath "$(pwd)" )"

# Install BuildCache and symlinks to BuildCache for common compilers.
BUILDCACHEEXE="${BUILDCACHEDIR}/buildcache${EXESUFFIX}"
echo "Buildcache executable: ${BUILDCACHEEXE}"
SYMLINKSDIR="${BUILDCACHEDIR}/symlinks"
echo "Install symlinks in: ${SYMLINKSDIR}"
rm -rf "${SYMLINKSDIR}" ; mkdir -p "${SYMLINKSDIR}"
ln -s "${BUILDCACHEEXE}" "${SYMLINKSDIR}/cc${EXESUFFIX}"
ln -s "${BUILDCACHEEXE}" "${SYMLINKSDIR}/c++${EXESUFFIX}"
ln -s "${BUILDCACHEEXE}" "${SYMLINKSDIR}/gcc${EXESUFFIX}"
ln -s "${BUILDCACHEEXE}" "${SYMLINKSDIR}/g++${EXESUFFIX}"
ln -s "${BUILDCACHEEXE}" "${SYMLINKSDIR}/clang${EXESUFFIX}"
ln -s "${BUILDCACHEEXE}" "${SYMLINKSDIR}/clang++${EXESUFFIX}"
ln -s "${BUILDCACHEEXE}" "${SYMLINKSDIR}/cl${EXESUFFIX}"
export PATH="${SYMLINKSDIR}:${BUILDCACHEDIR}:${PATH}"

# Configure BuildCache.
export BUILDCACHE_DIR=/tmp/.buildcache-$$
rm -rf "${BUILDCACHE_DIR}" ; mkdir -p "${BUILDCACHE_DIR}"
export BUILDCACHE_DEBUG=2

# Clone the project.
PROJDIR="/tmp/proj-$$"
SRCDIR="${PROJDIR}"
BUILDDIR="${PROJDIR}/out"
rm -rf "${PROJDIR}"
PROJURL=https://github.com/mzucker/miniray.git
PROJVERSION=master
git clone --branch ${PROJVERSION} --depth 1 ${PROJURL} "${PROJDIR}"

echo "======== First build (cold cache) ========"
buildcache -C

cd "${PROJDIR}"
rm -rf "${BUILDDIR}" ; mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "${SRCDIR}"

time ninja
buildcache -s

echo "======== Second build (warm cache) ========"
cd "${PROJDIR}"
rm -rf "${BUILDDIR}" ; mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "${SRCDIR}"

time ninja
buildcache -s

echo "======== Cleaning up ========"
cleanup
