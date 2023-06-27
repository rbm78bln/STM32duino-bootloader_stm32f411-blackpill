#!/bin/sh

UPLOADER="$1"
shift  1

echo "${UPLOADER}" "$@"
"${UPLOADER}" "$@"
RC=$?

[ "${RC}" -eq "74" ] && RC=0    # LIBUSB_ERROR_OTHER
[ "${RC}" -eq "251" ] && RC=0   # whatever...

exit ${RC}
