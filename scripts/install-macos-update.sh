#!/bin/bash
# Arguments are passed directly by QProcess, never evaluated as shell source.
set -eu
pid="$1"
app="$2"
stage="$3"
backup="$4"
[[ "$pid" =~ ^[0-9]+$ && "$app" == /*.app && "$stage" == /*/.vaa3d-update-* && "$backup" == /*.zip ]] || exit 2
[[ "$(dirname "$app")" == "$(dirname "$stage")" && -d "$app" && -d "$stage/Vaa3D.app" && ! -L "$app" && ! -L "$stage/Vaa3D.app" ]] || exit 2
exec >>"$backup.log" 2>&1
lock="$(dirname "$app")/.vaa3d-install.lock"
mkdir "$lock" || { echo 'Another installer is active.'; exit 3; }
moved=0
installed=0
finish() {
    result=$?
    if [[ "$moved" == 1 && "$installed" == 0 && ! -e "$app" ]]; then
        /bin/mv "$stage/Previous.app" "$app" || true
    fi
    rmdir "$lock" || true
    if [[ "$result" != 0 ]]; then
        echo "Installation failed ($result). Previous app restored when possible. Staging directory: $stage"
        /usr/bin/open -t "$backup.log" || true
    fi
}
trap finish EXIT
# Never terminate the user's process or close unsaved work.
for ((i=0; i<600; i++)); do
    if ! kill -0 "$pid" 2>/dev/null; then break; fi
    sleep 1
done
if kill -0 "$pid" 2>/dev/null; then echo 'Timed out waiting for Vaa3D to quit.'; exit 4; fi
/usr/bin/codesign --verify --deep --strict "$stage/Vaa3D.app"
/usr/bin/ditto -c -k --sequesterRsrc --keepParent "$app" "$backup"
/usr/bin/unzip -tq "$backup"
/bin/mv "$app" "$stage/Previous.app"
moved=1
/bin/mv "$stage/Vaa3D.app" "$app"
installed=1
if ! /usr/bin/open -n "$app"; then
    /bin/mv "$app" "$stage/Failed.app"
    installed=0
    exit 5
fi
# The previous bundle is retained in the verified backup ZIP.
/bin/rm -rf "$stage/Previous.app"
/bin/rm -f "$stage/update.zip" "$stage/releases.json" "$stage/install-update.sh"
rmdir "$stage" 2>/dev/null || true
echo 'Installed and launched successfully.'
