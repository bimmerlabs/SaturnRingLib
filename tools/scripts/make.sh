# Linux code here

# 1. Determine if a custom compiler path was provided ($2)
# If provided (even if empty ""), make arguments start from $3. If not, they start from $2.
if [[ $# -ge 2 ]]; then
  if [[ -n "$2" ]]; then
    printf "\033[91mUsing custom compiler path\033[0m\r\n"
    export COMPILER_DIR="$2"
  else
    export COMPILER_DIR=../../Compiler
  fi
  MAKE_ARGS=("${@:3}") # Captures 3rd, 4th, 5th... arguments
else
  export COMPILER_DIR=../../Compiler
  MAKE_ARGS=()
fi

# 2. Environment Setup

# Helper function to safely add to PATH if not already present
add_to_path() {
  if [[ ":$PATH:" != *":$1:"* ]]; then
    export PATH="$1:$PATH"
  fi
}

host_platform="$(uname -s)"
if [ "$host_platform" = "Darwin" ]; then
  add_to_path "${COMPILER_DIR}/mac/sh2eb-elf/bin"
elif [ "$host_platform" = "Linux" ]; then
  add_to_path "${COMPILER_DIR}/linux/sh2eb-elf/bin"
else
  echo "Unsupported host platform: $host_platform"
  exit 1
fi

# 3. Handle default fallback safely
TARGET="${1:-debug}"

# 4. Execute Build Targets with the extra arguments
case "$TARGET" in
  debug)
    printf "\033[30m\033[43mBuilding debug...\033[0m\r\n"
    make all DEBUG=1 "${MAKE_ARGS[@]}" || exit 1
    ;;
  release)
    printf "\033[30m\033[42mBuilding release...\033[0m\r\n"
    make all "${MAKE_ARGS[@]}" || exit 1
    ;;
  clean)
    printf "\033[30m\033[105mCleaning...\033[0m\r\n"
    make clean "${MAKE_ARGS[@]}" || exit 1
    ;;
  *)
    echo "Unknown target: $TARGET"
    exit 1
    ;;
esac

exit
