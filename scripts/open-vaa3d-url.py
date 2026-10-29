#!/usr/bin/env python3
"""Open one local volume/neuron via vaa3d://open?path=<encoded absolute path>."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
from urllib.parse import parse_qsl, urlsplit


LAUNCHER = Path(__file__).resolve().with_name('open-vaa3d.sh')
EXTENSIONS = {'.swc', '.eswc', '.tif', '.tiff', '.v3dpbd'}


def local_path(url):
    # Reject controls before urlsplit can silently strip them. Decode only once.
    if not url or any(ord(c) < 32 or ord(c) == 127 for c in url):
        raise ValueError('链接中包含无效字符。')
    if re.search(r'%(?![0-9a-fA-F]{2})', url):
        raise ValueError('链接的百分号编码无效。')
    parts = urlsplit(url)
    if (parts.scheme.lower() != 'vaa3d' or parts.netloc.lower() != 'open'
            or parts.path not in ('', '/') or '#' in url):
        raise ValueError('请使用 vaa3d://open?path=经过 URL 编码的绝对路径。')
    params = parse_qsl(parts.query, keep_blank_values=True, strict_parsing=True,
                       encoding='utf-8', errors='strict')
    if len(params) != 1 or params[0][0] != 'path' or not params[0][1]:
        raise ValueError('链接必须且只能包含一个 path 参数。')
    value = params[0][1]
    if any(ord(c) < 32 or ord(c) == 127 for c in value):
        raise ValueError('文件路径中包含无效字符。')
    path = Path(value)
    if not path.is_absolute():
        raise ValueError('文件路径必须是本机可访问的绝对路径。')
    if path.suffix.lower() not in EXTENSIONS:
        raise ValueError('仅支持 SWC、ESWC、TIF、TIFF 和 V3DPBD 文件。')
    if not path.is_file() or not os.access(path, os.R_OK):
        raise ValueError('文件不存在、不是普通文件或无法读取：\n' + value)
    # Keep the original suffix and symlink spelling for Vaa3D's format dispatch.
    return str(path)


def report_error(message):
    print('Vaa3D: ' + message, file=sys.stderr)
    zenity = shutil.which('zenity')
    if zenity and (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')):
        # Paths and messages are always arguments, never shell commands or markup.
        subprocess.run([zenity, '--error', '--no-markup', '--title=Vaa3D',
                        '--text=' + message], check=False)


def main(argv=None):
    args = sys.argv[1:] if argv is None else argv
    try:
        if len(args) != 1:
            raise ValueError('需要一个 vaa3d://open?path=... 链接。')
        path = local_path(args[0])
        # Reuse the desktop launcher's runtime setup and normal -i loading.
        # An absolute path cannot become a CLI option; execv never uses a shell.
        os.execv(str(LAUNCHER), [str(LAUNCHER), path])
    except (ValueError, OSError) as error:
        report_error(str(error))
        return 2


if __name__ == '__main__':
    sys.exit(main())
