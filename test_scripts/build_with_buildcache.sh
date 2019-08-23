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
    [[ ! -z "${LOG_DIR}" ]] && rm -rf "${LOG_DIR}"
    [[ ! -z "${SYMLINKSDIR}" ]] && rm -rf "${SYMLINKSDIR}"
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
export LOG_DIR=/tmp/.buildcache-logs-$$
rm -rf "${LOG_DIR}" ; mkdir -p "${LOG_DIR}"

# Clone the project.
PROJDIR="/tmp/proj-$$"
SRCDIR="${PROJDIR}/src"
BUILDDIR="${PROJDIR}/out"
rm -rf "${PROJDIR}"
PROJURL=https://github.com/mbitsnbites/buildcache.git
PROJVERSION=master
git clone --branch ${PROJVERSION} --depth 1 ${PROJURL} "${PROJDIR}"

echo "======== First build (cold cache) ========"
buildcache -C

cd "${PROJDIR}"
rm -rf "${BUILDDIR}" ; mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "${SRCDIR}"

time BUILDCACHE_LOG_FILE="${LOG_DIR}/test-pass-1.log" ninja
buildcache -s
echo "--- LOG (pass 1) ---"
cat "${LOG_DIR}/test-pass-1.log"

echo "======== Second build (warm cache) ========"
cd "${PROJDIR}"
rm -rf "${BUILDDIR}" ; mkdir -p "${BUILDDIR}"
cd "${BUILDDIR}"
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release "${SRCDIR}"

time BUILDCACHE_LOG_FILE="${LOG_DIR}/test-pass-2.log" ninja
buildcache -s
echo "--- LOG (pass 2) ---"
cat "${LOG_DIR}/test-pass-2.log"

# Make sure that there were no cache misses during the second pass.
EXIT_CODE=0
if grep -q "Cache miss" "${LOG_DIR}/test-pass-2.log"; then
    echo "*** FAIL: The second build pass contained cache misses."
    EXIT_CODE=1
else
    echo "The test passed!"
fi

# Clean up.
cleanup

exit ${EXIT_CODE}
