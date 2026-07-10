#!/usr/bin/env bash
# devtools/test_full.sh -- boot QEMU, insmod the module, run the sysfs
# functional test suite (devtools/functional_test.sh), dump dmesg, rmmod, and
# exit with the suite's status.
#
# More thorough than devtools/test.sh, which only checks that insmod/rmmod
# succeed -- this exercises parameter validation, all three lock types, reset,
# and the "refuse while a test is active" (EBUSY) behavior.

set -euo pipefail

DEVTOOLS_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$DEVTOOLS_DIR/.." && pwd)"
. "$DEVTOOLS_DIR/config.defaults"
[ -f "$DEVTOOLS_DIR/config.local" ] && . "$DEVTOOLS_DIR/config.local"

die() { echo "ERROR: $*" >&2; exit 1; }

KO=$(ls "$PROJECT_ROOT"/build/*.ko 2>/dev/null | head -1) || true
[ -n "${KO:-}" ] || die "No .ko in build/. Run devtools/build.sh first."
KOFILE=$(basename "$KO")
# Module name = filename without .ko, with '-' turned into '_' (kernel rule).
MOD=$(basename "$KOFILE" .ko | tr '-' '_')

CMD="insmod $GUEST_MNT/build/$KOFILE || { echo INSMOD_FAILED; exit 1; }; sh $GUEST_MNT/devtools/functional_test.sh; RC=\$?; dmesg | tail -n 100; rmmod $MOD; exit \$RC"
exec "$DEVTOOLS_DIR/boot.sh" --test "$CMD"
