#!/usr/bin/env python3
"""Register local TIFF associations and/or the vaa3d: URL handler (no sudo)."""
import argparse
import os
from pathlib import Path
import subprocess

# Desktop Exec uses its own quoting, not shell quoting. Literal percent is %%.
def exec_quote(path):
    value = str(path).replace('%', '%%')
    for char in ('\\', '"', '`', '$'):
        value = value.replace(char, '\\' + char)
    return '"' + value.replace('\\', '\\\\') + '"'

def value_escape(path):
    return str(path).replace('\\', '\\\\').replace('\n', '\\n')

def install(root, protocol_only=False):
    if not os.access(root / 'bin/vaa3d', os.X_OK):
        raise SystemExit('Build Vaa3D first with ./build-debug-local.sh')
    applications = Path(os.environ.get('XDG_DATA_HOME', Path.home() / '.local/share')) / 'applications'
    applications.mkdir(parents=True, exist_ok=True)
    entries = []
    if not protocol_only:
        entries.append(('vaa3d-local.desktop', 'Vaa3D',
                        f'/usr/bin/env {exec_quote(root / "scripts/open-vaa3d.sh")} %F',
                        ('image/tiff', 'image/x-tiff')))
    entries.append(('vaa3d-protocol.desktop', 'Vaa3D',
                    f'/usr/bin/python3 {exec_quote(root / "scripts/open-vaa3d-url.py")} %u',
                    ('x-scheme-handler/vaa3d',)))
    for name, title, command, mimes in entries:
        desktop = applications / name
        desktop.write_text(
            '[Desktop Entry]\nType=Application\nVersion=1.0\n'
            f'Name={title}\nComment=Open local images and neurons with Vaa3D\n'
            f'Exec={command}\n'
            f'Icon={value_escape(root / "v3d_main/v3d/v3d_logo.png")}\n'
            'Terminal=false\nStartupNotify=false\nStartupWMClass=vaa3d\nCategories=Science;\n'
            f'MimeType={";".join(mimes)};\n', encoding='utf-8')
        subprocess.run(['desktop-file-validate', str(desktop)], check=True)
    subprocess.run(['update-desktop-database', str(applications)], check=True)
    for name, title, command, mimes in entries:
        for mime in mimes:
            subprocess.run(['xdg-mime', 'default', name, mime], check=True)
        print('Installed:', applications / name)
    print('Default vaa3d: URL handler: Vaa3D')
    if not protocol_only:
        print('Default TIFF viewer: Vaa3D')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--protocol-only', action='store_true',
                        help='register vaa3d: URLs without changing TIFF associations')
    install(Path(__file__).resolve().parent.parent, parser.parse_args().protocol_only)
