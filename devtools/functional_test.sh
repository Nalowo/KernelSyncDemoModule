#!/bin/sh
# devtools/functional_test.sh -- functional test suite for kernel_sync_demo's
# /sys/module/kernel_sync_demo/parameters/ interface.
#
# Runs INSIDE the QEMU guest (BusyBox ash) after the module is already
# insmod'd -- see devtools/test_full.sh / `make qemu-test-full`. Does not
# insmod/rmmod itself, so it can also be run by hand from an interactive
# `make qemu-boot` shell:
#   sh /mnt/host/devtools/functional_test.sh
#
# Exits 0 if every check passes, 1 otherwise.

PARAM_DIR=/sys/module/kernel_sync_demo/parameters
FAIL=0

pass() { echo "PASS: $*"; }
fail() { echo "FAIL: $*"; FAIL=1; }
warn() { echo "WARN: $*"; }

# Write $val to parameter $file and expect the write to succeed.
expect_ok() {
	desc="$1"; file="$2"; val="$3"
	if echo "$val" > "$PARAM_DIR/$file" 2>/dev/null; then
		pass "$desc"
	else
		fail "$desc (expected write to succeed)"
	fi
}

# Write $val to parameter $file and expect the write to be rejected (EINVAL/EBUSY/...).
expect_fail() {
	desc="$1"; file="$2"; val="$3"
	if echo "$val" > "$PARAM_DIR/$file" 2>/dev/null; then
		fail "$desc (expected write to be rejected)"
	else
		pass "$desc"
	fi
}

[ -d "$PARAM_DIR" ] || { echo "FAIL: $PARAM_DIR not found -- is the module loaded?"; exit 1; }

echo "=== validation: num_threads (expected range 1..32) ==="
expect_ok   "num_threads=1 accepted"  num_threads 1
expect_ok   "num_threads=32 accepted" num_threads 32
expect_fail "num_threads=0 rejected"  num_threads 0
expect_fail "num_threads=33 rejected" num_threads 33

echo "=== validation: iterations (expected range 1..1000000) ==="
expect_ok   "iterations=1 accepted"       iterations 1
expect_ok   "iterations=1000000 accepted" iterations 1000000
expect_fail "iterations=0 rejected"       iterations 0
expect_fail "iterations=1000001 rejected" iterations 1000001

echo "=== validation: lock_type (0=spinlock, 1=mutex, 2=semaphore) ==="
expect_ok   "lock_type=0 (spinlock) accepted"  lock_type 0
expect_ok   "lock_type=1 (mutex) accepted"     lock_type 1
expect_ok   "lock_type=2 (semaphore) accepted" lock_type 2
expect_fail "lock_type=3 rejected"             lock_type 3

# Run a full test cycle with a given lock type and check counter/result/stats.
run_and_check() {
	lock_name="$1"; lock_val="$2"
	echo "=== run: lock_type=$lock_name ==="
	echo 1     > "$PARAM_DIR/reset"
	echo 8     > "$PARAM_DIR/num_threads"
	echo 5000  > "$PARAM_DIR/iterations"
	echo "$lock_val" > "$PARAM_DIR/lock_type"
	echo 1     > "$PARAM_DIR/run"

	result=$(cat "$PARAM_DIR/result")
	stats=$(cat "$PARAM_DIR/stats")
	echo "result: $result"
	echo "stats:  $stats"

	case "$result" in
		*counter=0*) pass "$lock_name: shared_counter returned to 0" ;;
		*) fail "$lock_name: shared_counter did not return to 0 ($result)" ;;
	esac
	case "$result" in
		*result=0*) pass "$lock_name: last_run_result == SD_OK" ;;
		*) fail "$lock_name: last_run_result != 0 ($result)" ;;
	esac
}

run_and_check spinlock  0
run_and_check mutex     1
run_and_check semaphore 2

echo "=== reset ==="
echo 1 > "$PARAM_DIR/reset"
result=$(cat "$PARAM_DIR/result")
stats=$(cat "$PARAM_DIR/stats")
case "$result" in
	*counter=0*) pass "reset: counter cleared" ;;
	*) fail "reset: counter not cleared ($result)" ;;
esac
case "$stats" in
	*contention=0*) pass "reset: contention cleared" ;;
	*) fail "reset: contention not cleared ($stats)" ;;
esac

echo "=== busy: run/reset must be refused while a test is active ==="
# Configure a run long enough to overlap with the checks below. This is a
# timing-dependent check: on a very fast host the background run may finish
# before the probes fire, in which case treat it as inconclusive (WARN), not
# a failure.
echo 32     > "$PARAM_DIR/num_threads"
echo 900000 > "$PARAM_DIR/iterations"
echo 1      > "$PARAM_DIR/lock_type"

( echo 1 > "$PARAM_DIR/run" ) &
RUN_PID=$!
sleep 1

if kill -0 "$RUN_PID" 2>/dev/null; then
	if echo 1 > "$PARAM_DIR/reset" 2>/dev/null; then
		fail "reset accepted while test active (expected EBUSY)"
	else
		pass "reset rejected while test active"
	fi
	if echo 1 > "$PARAM_DIR/run" 2>/dev/null; then
		fail "run accepted while test active (expected EBUSY)"
	else
		pass "run rejected while test active"
	fi
else
	warn "background run finished before busy-checks fired -- inconclusive, increase iterations/num_threads"
fi
wait "$RUN_PID" 2>/dev/null

if [ "$FAIL" -eq 0 ]; then
	echo "=== ALL TESTS PASSED ==="
else
	echo "=== TESTS FAILED ==="
fi
exit "$FAIL"
