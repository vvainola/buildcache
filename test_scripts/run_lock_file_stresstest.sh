#!/bin/bash

# Note: This script is expected to be run from the build folder.

TESTFILE=/tmp/bc_lock_file_stresstest_data-$$

rm -f "$TESTFILE"

# Run four instances of the stresstest in parallel.
echo "Starting four concurrent processes..."
pids=""
for i in {1..4}; do
  base/lock_file_stresstest "$TESTFILE" &
  pids+=" $!"
done

# Wait for all processes to finish.
got_error=false
for p in $pids ; do
  if ! wait $p ; then
    got_error=true
  fi
done

# Get the result and delete the file.
DATA=$(cat "$TESTFILE")
rm -f "$TESTFILE"

# Did we have an error exit status from any of the processes?
if $got_error ; then
  echo "*** FAIL: At least one of the processes failed."
  exit 1
fi

# Check the data file contents.
EXPECTED_DATA="4000"
if [[ "${DATA}" = "${EXPECTED_DATA}" ]] ; then
  echo "The test passed!"
  exit 0
fi

echo "*** FAIL: The count should be ${EXPECTED_DATA}, but is ${DATA}."
exit 1

