#!/usr/bin/env python3
"""Open a local file or image/SWC/mask set via vaa3d://open?... ."""
import os
import errno
import fcntl
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import socket
import stat
import sys
import time
import uuid
from urllib.parse import parse_qsl, urlsplit


LAUNCHER = Path(__file__).resolve().with_name('open-vaa3d.sh')
EXTENSIONS = {'.swc', '.eswc', '.tif', '.tiff', '.v3dpbd'}


def parameters(url):
    # Reject controls before urlsplit can silently strip them. Decode only once.
    if not url or any(ord(c) < 32 or ord(c) == 127 for c in url):
        raise ValueError('链接中包含无效字符。')
    if re.search(r'%(?![0-9a-fA-F]{2})', url):
        raise ValueError('链接的百分号编码无效。')
    parts = urlsplit(url)
    if (parts.scheme.lower() != 'vaa3d' or parts.netloc.lower() != 'open'
            or parts.path not in ('', '/') or '#' in url):
        raise ValueError('请使用 vaa3d://open?path=经过 URL 编码的绝对路径。')
    return parse_qsl(parts.query, keep_blank_values=True, strict_parsing=True,
                     encoding='utf-8', errors='strict')


def checked_path(value, extensions=EXTENSIONS):
    if any(ord(c) < 32 or ord(c) == 127 for c in value):
        raise ValueError('文件路径中包含无效字符。')
    path = Path(value)
    if not path.is_absolute():
        raise ValueError('文件路径必须是本机可访问的绝对路径。')
    if path.suffix.lower() not in extensions:
        raise ValueError('该角色不支持此文件类型：\n' + value)
    if not path.is_file() or not os.access(path, os.R_OK):
        raise ValueError('文件不存在、不是普通文件或无法读取：\n' + value)
    # Keep the original suffix and symlink spelling for Vaa3D's format dispatch.
    return str(path)


def local_path(url):
    params = parameters(url)
    if len(params) != 1 or params[0][0] != 'path' or not params[0][1]:
        raise ValueError('单文件链接必须且只能包含一个 path 参数。')
    return checked_path(params[0][1])


def launch_arguments(url):
    params = parameters(url)
    if any(key == 'path' for key, _ in params):
        return [local_path(url)]
    roles = {'image': {'.tif', '.tiff', '.v3dpbd'},
             'swc': {'.swc', '.eswc'}, 'mask': {'.tif', '.tiff'}}
    result, seen, counts = ['--open-set'], set(), {}
    for key, value in params:
        counts[key] = counts.get(key, 0) + 1
        if key == 'mesh_density':
            if counts[key] != 1 or not re.fullmatch(r'[0-9]{1,4}', value) or not 1 <= int(value) <= 1000:
                raise ValueError('mesh_density 必须是 1–1000 的整数且只能出现一次。')
            result.extend(['--mesh-density', value])
        elif key in roles:
            path = checked_path(value, roles[key])
            canonical = os.path.realpath(path)
            if canonical in seen:
                raise ValueError('一组文件中不能重复使用同一个文件：\n' + path)
            seen.add(canonical)
            result.extend(['--' + key, path])
        else:
            raise ValueError('不支持的参数：' + key)
    if counts.get('image') != 1:
        raise ValueError('一组文件需要且只能有一个 image 原图，可重复传入 swc 和 mask。')
    if counts.get('mask') and not counts.get('mesh_density'):
        result.extend(['--mesh-density', '400'])
    return result


def report_error(message):
    print('Vaa3D: ' + message, file=sys.stderr)
    zenity = shutil.which('zenity')
    if zenity and (os.environ.get('DISPLAY') or os.environ.get('WAYLAND_DISPLAY')):
        # Paths and messages are always arguments, never shell commands or markup.
        subprocess.run([zenity, '--error', '--no-markup', '--title=Vaa3D',
                        '--text=' + message], check=False)


def ipc_directory():
    executable = str((LAUNCHER.parent.parent / 'bin/vaa3d').resolve())
    session = os.environ.get('DISPLAY', '') + '|' + os.environ.get('WAYLAND_DISPLAY', '')
    key = hashlib.sha256((executable + '\n' + session).encode('utf-8')).hexdigest()[:16]
    base = os.environ.get('XDG_RUNTIME_DIR', '')
    def private_dir(path):
        try:
            info = os.lstat(path)
        except OSError:
            return False
        return stat.S_ISDIR(info.st_mode) and info.st_uid == os.getuid() and not info.st_mode & 0o077
    if not base or not os.path.isabs(base) or not private_dir(base):
        base = '/tmp'
    folder = Path(base) / ('vaa3d-open-' + str(os.getuid()) + '-' + key)
    try:
        folder.mkdir(mode=0o700)
    except FileExistsError:
        pass
    if not private_dir(folder):
        raise ValueError('Vaa3D 本机通信目录权限无效：' + str(folder))
    return folder


def send_request(folder, payload):
    # Most recently active compatible GUI instance wins. Never start another
    # process after a connection succeeds but acknowledgement is uncertain.
    endpoints = []
    for endpoint in folder.glob('*.sock'):
        try:
            info = endpoint.stat()
            if stat.S_ISSOCK(info.st_mode):
                endpoints.append((info.st_mtime_ns, endpoint))
        except FileNotFoundError:
            continue
    for _, endpoint in sorted(endpoints, reverse=True):
        with socket.socket(socket.AF_UNIX) as connection:
            connection.settimeout(10)
            try:
                connection.connect(str(endpoint))
            except OSError as error:
                if error.errno in (errno.ENOENT, errno.ECONNREFUSED):
                    continue
                raise
            connection.sendall(payload)
            response = b''
            while b'\n' not in response:
                part = connection.recv(4096)
                if not part or len(response) > 16384:
                    raise ValueError('Vaa3D 未确认请求；为避免重复打开，不会启动另一个程序。')
                response += part
            reply = json.loads(response.split(b'\n', 1)[0])
            if reply.get('status') != 'accepted':
                raise ValueError(reply.get('error', 'Vaa3D 拒绝了打开请求。'))
            return True
    return False


def legacy_gui_running():
    executable = str((LAUNCHER.parent.parent / 'bin/vaa3d').resolve())
    for entry in Path('/proc').iterdir():
        if not entry.name.isdigit():
            continue
        try:
            if entry.stat().st_uid != os.getuid():
                continue
            actual = os.readlink(entry / 'exe').removesuffix(' (deleted)')
            if actual != executable:
                continue
            args = (entry / 'cmdline').read_bytes().split(b'\0')
            if any(option in args for option in (b'-cmd', b'-f', b'-h', b'--help')):
                continue
            env = dict(item.split(b'=', 1) for item in (entry / 'environ').read_bytes().split(b'\0') if b'=' in item)
            if all(env.get(key.encode(), b'').decode() == os.environ.get(key, '')
                   for key in ('DISPLAY', 'WAYLAND_DISPLAY')):
                return True
        except (OSError, ValueError):
            continue
    return False


def deliver(arguments):
    folder = ipc_directory()
    payload = (json.dumps({'version': 1, 'id': uuid.uuid4().hex, 'args': arguments},
                          ensure_ascii=False) + '\n').encode('utf-8')
    if len(payload) > 1024 * 1024:
        raise ValueError('文件组路径过长。')
    # The lock covers startup and delivery, so simultaneous browser clicks do
    # not launch a second GUI before the first has registered its receiver.
    descriptor = os.open(folder / 'launch.lock', os.O_CREAT | os.O_RDWR | os.O_NOFOLLOW, 0o600)
    with os.fdopen(descriptor, 'w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if send_request(folder, payload):
            return
        if legacy_gui_running():
            raise ValueError('检测到未提供链接接收接口的 Vaa3D。请保存工作后启动新版一次；为避免新开程序，本次未加载。')
        process = subprocess.Popen([str(LAUNCHER)], start_new_session=True,
                                   stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if send_request(folder, payload):
                return
            if process.poll() is not None:
                raise ValueError('Vaa3D 启动失败，未收到本机接收接口。')
            time.sleep(0.1)
        raise ValueError('等待 Vaa3D 启动超时；不会重复启动，请检查程序窗口。')


def main(argv=None):
    args = sys.argv[1:] if argv is None else argv
    try:
        if len(args) != 1:
            raise ValueError('需要一个 vaa3d://open?path=... 链接。')
        arguments = launch_arguments(args[0])
        deliver(arguments)
        return 0
    except (ValueError, OSError) as error:
        report_error(str(error))
        return 2


if __name__ == '__main__':
    sys.exit(main())
