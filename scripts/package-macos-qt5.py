#!/usr/bin/env python3
"""Bundle a prebuilt Qt5 app and audit all Mach-O dependencies before archiving."""
import argparse
import hashlib
import json
import plistlib
import re
import shutil
import subprocess
from pathlib import Path


def run(*args):
    return subprocess.check_output(args, text=True).strip()


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('app', type=Path)
parser.add_argument('output', type=Path)
parser.add_argument('--version', required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
app = output / 'Vaa3D.app'
if app.exists():
    raise SystemExit('Output already contains Vaa3D.app. Use a fresh output directory.')
run('ditto', str(args.app.resolve()), str(app))
qt = Path(run('brew', '--prefix', 'qt@5'))
run(str(qt / 'bin/macdeployqt'), str(app), '-verbose=1')
# Qt5's deploy tool misses the modern WebP sharpyuv dependency.
webp = Path(run('brew', '--prefix', 'webp'))
shutil.copy2(webp / 'lib/libsharpyuv.0.dylib', app / 'Contents/Frameworks/libsharpyuv.0.dylib')
files = [p for p in app.rglob('*') if p.is_file() and not p.is_symlink()
         and 'Mach-O' in run('file', '-b', str(p))]
versions = []
for binary in files:
    if 'arm64' not in run('lipo', '-archs', str(binary)).split():
        raise RuntimeError(f'Missing arm64 architecture: {binary}')
    lines = run('otool', '-L', str(binary)).splitlines()[1:]
    ids = run('otool', '-D', str(binary)).splitlines()[1:]
    for line in lines:
        dep = line.strip().split(' (')[0]
        if dep in ids:
            if dep.startswith('/opt/'):
                run('install_name_tool', '-id', '@rpath/' + binary.name, str(binary))
            continue
        if dep.startswith(('/usr/lib/', '/System/Library/')):
            continue
        if dep.startswith('@executable_path/'):
            target = app / 'Contents/MacOS' / dep[len('@executable_path/'):]
        elif dep.startswith('@loader_path/'):
            target = binary.parent / dep[len('@loader_path/'):]
        elif dep.startswith('@rpath/'):
            target = app / 'Contents/Frameworks' / dep[len('@rpath/'):]
        else:
            raise RuntimeError(f'External dependency: {binary}: {dep}')
        if not target.exists():
            raise RuntimeError(f'Missing dependency: {binary}: {dep}')
    load = run('otool', '-l', str(binary))
    versions.extend(re.findall(r'\bminos\s+([\d.]+)', load))
    for rpath in re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset', load):
        if rpath.startswith(('/opt/', '/Users/')):
            run('install_name_tool', '-delete_rpath', rpath, str(binary))
minimum = max(versions, key=lambda v: tuple(map(int, v.split('.'))))
plist_path = app / 'Contents/Info.plist'
with plist_path.open('rb') as stream:
    plist = plistlib.load(stream)
plist.update(CFBundleName='Vaa3D', CFBundleDisplayName='Vaa3D',
             CFBundleShortVersionString=args.version, CFBundleVersion=args.version,
             LSMinimumSystemVersion=minimum)
with plist_path.open('wb') as stream:
    plistlib.dump(plist, stream)
licenses = app / 'Contents/Resources/Licenses'
licenses.mkdir(parents=True, exist_ok=True)
shutil.copy2(root / 'LICENSE', licenses / 'Vaa3D-LICENSE')
formulae = ['qt@5', 'libaec', 'freetype', 'glib', 'hdf5', 'gettext', 'jpeg-turbo',
           'xz', 'md4c', 'pcre2', 'libpng', 'teem', 'libtiff', 'webp', 'zstd']
for formula in formulae:
    prefix = Path(run('brew', '--prefix', formula)).resolve()
    destination = licenses / formula
    destination.mkdir()
    for item in prefix.iterdir():
        if item.is_file() and re.match(r'LICENSE|COPYING|NOTICE|COPYRIGHT|AUTHORS', item.name, re.I):
            shutil.copy2(item, destination / item.name)
    (destination / 'source.txt').write_text(run('brew', 'info', '--json=v2', formula) + '\n')
run('codesign', '--force', '--deep', '--sign', '-', str(app))
run('codesign', '--verify', '--deep', '--strict', str(app))
manifest = {'version': args.version, 'packaging_source': run('git', '-C', str(root), 'rev-parse', 'HEAD'),
            'input_executable_sha256': hashlib.sha256((args.app / 'Contents/MacOS' / plist['CFBundleExecutable']).read_bytes()).hexdigest(),
            'architecture': 'arm64', 'minimum_macos': minimum, 'macho_files_checked': len(files),
            'signature': 'ad-hoc, not notarized', 'external_non_system_dependencies': 0}
(output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(json.dumps(manifest, indent=2))
