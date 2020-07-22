#! /bin/bash
#
# A clang-tidy lint script. Usage:
# ./lint.sh
#   Runs clang-tidy on the current git stage
# ./lint.sh revision..range
#   Runs clang-tidy on the specified range of commits

set -eo pipefail

[ -z "${CLANG_TIDY}" ] && CLANG_TIDY="clang-tidy"

if [ ! -d "${BUILD_DIR}" ]; then
  echo >&2 "BUILD_DIR not set";
  exit 1;
fi;

if [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
  echo >&2 "${BUILD_DIR}/compile_commands.json not found - is BUILD_DIR set correctly?"
  exit 1;
fi;

if [ -z "$(command -v ${CLANG_TIDY})" ]; then
  echo >&2 "clang tidy binary \"${CLANG_TIDY}\" not found"
  exit 1;
fi;

# Default to 'cached', or the revision passed as an argument
GIT_REVISION=${1:---cached}

MODIFIED_FILES=$(git diff --name-only --diff-filter=ACMRTUXB ${GIT_REVISION})


RET=0

for f in ${MODIFIED_FILES}; do
  # Skip any non C++ files
  if ! echo "${f}" | egrep -q "[.](cpp)$"; then
    continue;
  fi
  echo "Running clang-tidy on ${f}"

  ${CLANG_TIDY} -p "${BUILD_DIR}" "${f}" || true
  RESULT=$?
  if [ ${RESULT} != 0 ]; then
    RET=1;
    echo "File ${f} failed clang-tidy checks"
  fi;
done;

echo "All specificed files checked"

exit ${RET};

