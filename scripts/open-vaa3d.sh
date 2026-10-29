#!/usr/bin/env bash
# Desktop file associations pass local paths; preserve each filename as one argument.
set -euo pipefail
root_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
# This local Anaconda Qt build needs the system C++ runtime (same as GUI tests).
if [[ -r /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ]]; then
    export LD_PRELOAD="/usr/lib/x86_64-linux-gnu/libstdc++.so.6${LD_PRELOAD:+:$LD_PRELOAD}"
fi
if [[ "${1:-}" == --open-set ]]; then
    exec "$root_dir/bin/vaa3d" "$@"
elif (( $# )); then
    exec "$root_dir/bin/vaa3d" -i "$@"
else
    exec "$root_dir/bin/vaa3d"
fi
