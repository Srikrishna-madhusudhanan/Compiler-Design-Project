#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
AS="${RISCV_AS:-riscv64-linux-gnu-as}"
OC="${RISCV_OBJCOPY:-riscv64-linux-gnu-objcopy}"
RVAS="${ROOT}/rvas"
if [[ ! -x "$RVAS" ]]; then
  make -C "$ROOT" rvas
fi
if ! command -v "$AS" >/dev/null || ! command -v "$OC" >/dev/null; then
  echo "Need $AS and $OC in PATH" >&2
  exit 1
fi

pass=0
fail=0
for s in "$ROOT"/snippets/*.s; do
  [[ -f "$s" ]] || continue
  name="$(basename "$s")"
  "$RVAS" -o "/tmp/rvas_${name}.o" "$s"
  "$AS" -march=rv64im -o "/tmp/gas_${name}.o" "$s"
  "$OC" -O binary -j .text "/tmp/rvas_${name}.o" "/tmp/rvas_${name}.bin"
  "$OC" -O binary -j .text "/tmp/gas_${name}.o" "/tmp/gas_${name}.bin"
  if cmp -s "/tmp/rvas_${name}.bin" "/tmp/gas_${name}.bin"; then
    echo "OK  $name (.text bytes match GNU as)"
    pass=$((pass + 1))
  else
    echo "FAIL $name" >&2
    fail=$((fail + 1))
  fi
done

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi
echo "All $pass snippet(s) matched."
