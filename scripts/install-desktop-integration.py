#!/usr/bin/env python3
"""Register this checkout as the current user's default TIFF viewer (no sudo)."""
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent.parent
if not os.access(root / 'bin/vaa3d', os.X_OK):
    raise SystemExit('Build Vaa3D first with ./build-debug-local.sh')
applications = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share')) / 'applications'
applications.mkdir(parents=True, exist_ok=True)
# Desktop Exec uses its own quoting, not shell quoting. Literal percent is %%.
def exec_quote(path):
    value = str(path).replace('%', '%%')
    for char in ('\\', '"', '`', '$'):
        value = value.replace(char, '\\' + char)
    return '"' + value.replace('\\', '\\\\') + '"'

def value_escape(path):
    return str(path).replace('\\', '\\\\').replace('\n', '\\n')

desktop = applications / 'vaa3d-local.desktop'
desktop.write_text(
    '[Desktop Entry]\nType=Application\nVersion=1.0\nName=Vaa3D\n'
    'Comment=View TIFF volumes with Vaa3D\n'
    f'Exec=/usr/bin/env {exec_quote(root / "scripts/open-vaa3d.sh")} %F\n'
    f'Icon={value_escape(root / "v3d_main/v3d/v3d_logo.png")}\n'
    'Terminal=false\nStartupNotify=false\nStartupWMClass=vaa3d\nCategories=Science;\n'
    'MimeType=image/tiff;image/x-tiff;\n', encoding='utf-8')
subprocess.run(['desktop-file-validate', str(desktop)], check=True)
subprocess.run(['update-desktop-database', str(applications)], check=True)
for mime in ('image/tiff', 'image/x-tiff'):
    subprocess.run(['xdg-mime', 'default', desktop.name, mime], check=True)
print('Installed:', desktop)
print('Default TIFF viewer: Vaa3D')
