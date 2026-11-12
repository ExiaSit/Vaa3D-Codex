#!/usr/bin/env python3
"""Build released neuron/SWC plugins from a pinned vaa3d_tools checkout.
Adaptation occurs in an isolated source copy. Individual failures are recorded.
"""
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess

REVISION = '475e4ca92d4e51de10f1c05d80cef6615432c087'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('tools_source', type=Path)
parser.add_argument('build_dir', type=Path)
parser.add_argument('--only', nargs='+')
parser.add_argument('--app-build-dir', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source = args.tools_source.resolve()
build = args.build_dir.resolve()
build.mkdir(parents=True, exist_ok=True)
if subprocess.check_output(['git', '-C', str(source), 'rev-parse', 'HEAD'], text=True).strip() != REVISION:
    raise SystemExit('Expected vaa3d_tools revision ' + REVISION)
if subprocess.run(['git', '-C', str(source), 'diff', '--quiet', 'HEAD', '--', 'released_plugins', 'hackathon', 'bigneuron_ported']).returncode:
    raise SystemExit('Use an unmodified pinned vaa3d_tools checkout for reproducibility.')
plugins = source / 'released_plugins/v3d_plugins'
names = sorted(p.name for p in plugins.iterdir() if p.is_dir() and
               re.search(r'neuron|trac|swc|bigneuron', p.name, re.I))
staged = build / 'source/released_plugins'
# Retain shared source/resource files while excluding unrelated binary datasets.
allowed = {'.h', '.hpp', '.hxx', '.cpp', '.c', '.cc', '.cxx', '.pro', '.pri',
           '.txx', '.ui', '.qrc', '.png', '.svg', '.txt', '.in', '.inc', '.tpp'}
for p in (source / 'released_plugins').rglob('*'):
    if p.is_file() and (p.suffix.lower() in allowed or p.name.startswith(('LICENSE', 'COPYING'))):
        dst = staged / p.relative_to(source / 'released_plugins')
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(p, dst)
if not (build / 'source/v3d_main').exists():
    (build / 'source/v3d_main').symlink_to(root / 'v3d_main', target_is_directory=True)
if not (build / 'source/hackathon').exists():
    (build / 'source/hackathon').symlink_to(source / 'hackathon', target_is_directory=True)
for p in staged.rglob('*'):
    if not p.is_file() or p.suffix not in {'.cpp', '.h', '.hpp'}:
        continue
    s = p.read_text(errors='replace').lstrip('\ufeff')
    s = re.sub(r'Q_EXPORT_PLUGIN2\s*\([^)]*\)[ \t]*;?', '', s)
    s = re.sub(r'(Q_INTERFACES\s*\(V3DPluginInterface[^)]*\))',
               r'Q_PLUGIN_METADATA(IID "com.janelia.v3d.plugin")\n\1', s)
    if 'bigneuron_miroslav_r_Advantra' in p.parts:
        s = re.sub(r'(?<!::)\bround\s*\(', 'advantra_round(', s)
    if p.name == 'pca1.cpp':
        s = re.sub(r'\bsignals\b', 'pca_signals', s)
    s = s.replace('.toAscii()', '.toLatin1()').replace('QInputDialog::getInteger', 'QInputDialog::getInt')
    if (p.suffix == '.cpp' and p.name != 'svm.cpp') or 'Q_OBJECT' in s:
        s = '#include <QtWidgets>\n' + s
    p.write_text(s)
for project in staged.rglob('*'):
    if project.suffix in {'.pro', '.pri'}:
        text = project.read_text(errors='replace')
        text = re.sub(r'(?m)^\s*VAA3DPATH\s*[+]?=.*$',
                      lambda match: 'VAA3DPATH = ' + str(root / 'v3d_main' if 'v3d_main' in match.group() else root), text)
        text = re.sub(r'(?m)^\s*V3DMAINPATH\s*[+]?=.*$', 'V3DMAINPATH = ' + str(root / 'v3d_main'), text)
        text = re.sub(r'(?<![/\$])(?:\.\./)+(?:v3d_external/)?v3d_main', str(root / 'v3d_main'), text)
        text = re.sub(r'(?<![/\$])(?:\.\./)+v3d_external', str(root), text)
        text = text.replace(str(root / 'v3d_main') + '/../../vaa3d_tools', str(build / 'source'))
        text = text.replace('$$V3DMAINPATH/../../vaa3d_tools', str(build / 'source'))
        text = re.sub(r'-lv3dfftw3f(?:_threads)?\b', '', text)
        text = text.replace('$$VAA3DPATH/../vaa3d_tools', str(build / 'source'))
        text = text.replace('$$VAA3DPATH/../../vaa3d_tools', str(build / 'source'))
        text = text.replace('$$VAA3DPATH/v3d_main/../../vaa3d_tools', str(build / 'source'))
        project.write_text(text)
for link, target in [(build / 'v3d_external', root), (build / 'source/bigneuron_ported', source / 'bigneuron_ported')]:
    if not link.exists():
        link.symlink_to(target, target_is_directory=True)
qt = Path(subprocess.check_output(['brew', '--prefix', 'qt@5'], text=True).strip())
native = os.environ['MAC_QT5_NATIVE_LIB_DIR']
boost = os.environ['MAC_QT5_BOOST_INCLUDE_DIR']

def run_bounded(command, directory, log, timeout):
    process = subprocess.Popen(command, cwd=directory, stdout=log, stderr=log, start_new_session=True)
    try:
        return process.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        process.wait()
        return 124


def compile_plugin(name):
    folder = build / 'build' / name
    folder.mkdir(parents=True, exist_ok=True)
    for old_library in (build / 'plugins/neuron_tracing' / name).glob('*.dylib'):
        old_library.unlink()
    projects = sorted((staged / 'v3d_plugins' / name).glob('*.pro'))
    if name == 'neurontracing_vn2':
        projects = [root / 'plugins/neurontracing_vn2/vaa3dneuron2.pro']
    result = {'name': name, 'projects': [], 'libraries': []}
    for project in projects:
        log_path = folder / (project.stem + '.log')
        command = [str(qt / 'bin/qmake'), str(project), '-after', 'QT+=widgets',
                   'CONFIG+=c++17', 'CONFIG-=x86_64', 'QMAKE_APPLE_DEVICE_ARCHS=arm64',
                   'DEFINES+=USE_Qt5 _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR',
                   'QMAKE_CFLAGS+=-Wno-implicit-int -Wno-implicit-function-declaration', 'QMAKE_CXXFLAGS+=-Wno-register -Wno-dynamic-exception-spec -Wno-c++11-narrowing',
                   'INCLUDEPATH+=' + str(args.app_build_dir.resolve()) + ' ' + str(root / 'v3d_main/jba/newmat11') + ' ' + str(root / 'v3d_main/basic_c_fun') + ' ' + boost + ' /opt/homebrew/include /opt/homebrew/opt/libtiff/include',
                   'LIBS+=-L' + native,
                   'DESTDIR=' + str(build / 'plugins/neuron_tracing' / name)]
        if name == 'neuron_assemble_live':
            command += ['CONFIG-=c++17', 'CONFIG+=c++14']
        with log_path.open('w') as log:
            code = run_bounded(command, folder, log, 60)
            if code == 0:
                code = run_bounded(['make', '-j2'], folder, log, 300)
        result['projects'].append({'project': project.name, 'exit_code': code, 'log': str(log_path)})
    result['libraries'] = [str(p) for p in (build / 'plugins/neuron_tracing' / name).glob('*.dylib')]
    print(name + ': ' + ('BUILT' if result['libraries'] else 'BLOCKED'), flush=True)
    return result

with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    results = list(pool.map(compile_plugin, args.only or names))
if args.only and (build / 'build-results.json').exists():
    previous = json.loads((build / 'build-results.json').read_text())['results']
    results += [r for r in previous if r['name'] not in args.only]
(build / 'build-results.json').write_text(json.dumps({'upstream': REVISION, 'results': results}, indent=2) + '\n')
print('Built', sum(bool(r['libraries']) for r in results), 'of', len(results), flush=True)
