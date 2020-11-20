#! /bin/bash
#
# A clang-format lint script. Usage:
# ./lint.sh
#   Runs clang-format on the current git stage
# ./lint.sh revision..range
#   Runs clang-format on the specified range of commits

set -eo pipefail

[ -z "${CLANG_FORMAT}" ] && CLANG_FORMAT="clang-format"

if [ -z "$(command -v ${CLANG_FORMAT})" ]; then
  echo >&2 "clang format binary \"${CLANG_FORMAT}\" not found"
  exit 1;
fi;

echo "Clang-format version:"
${CLANG_FORMAT} --version

# Default to 'cached', or the revision passed as an argument
GIT_REVISION=${1:---cached}

MODIFIED_FILES=$(git diff --name-only --diff-filter=ACMRTUXB ${GIT_REVISION})


RET=0

for f in ${MODIFIED_FILES}; do
  # Skip any non C++ files
  if ! echo "${f}" | egrep -q "[.](cpp|h)$"; then
    continue;
  fi

  OUTPUT=$(${CLANG_FORMAT} ${f} | (diff -u  "${f}" - || true))
  if [ -n "${OUTPUT}" ]; then
    echo "ERROR: File \"${f}\" doesn't match expected format, diff:"
	echo
	echo "${OUTPUT}"
	RET=1;
  fi;
done;

exit ${RET};

