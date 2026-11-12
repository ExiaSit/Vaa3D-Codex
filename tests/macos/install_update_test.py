#!/usr/bin/env python3
"""Exercise installation after exit and rejection of a damaged signed bundle."""
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import tempfile
import time
import zipfile

helper = Path(__file__).resolve().parents[2] / 'scripts/install-macos-update.sh'

def bundle(path, marker):
    (path / 'Contents/MacOS').mkdir(parents=True)
    shutil.copyfile('/usr/bin/true', path / 'Contents/MacOS/test-executable')
    (path / 'Contents/MacOS/test-executable').chmod(0o755)
    (path / 'Contents/Info.plist').write_bytes(plistlib.dumps({
        'CFBundleIdentifier': 'org.exiasit.vaa3d.updater-test',
        'CFBundleExecutable': 'test-executable', 'CFBundlePackageType': 'APPL',
        'CFBundleVersion': '1', 'CFBundleName': 'Vaa3D Updater Test'}))
    (path / 'Contents/marker').write_text(marker)
    subprocess.run(['codesign', '--force', '--deep', '--sign', '-', str(path)], check=True, capture_output=True)

with tempfile.TemporaryDirectory(prefix='vaa3d-install-test-') as folder:
    root = Path(folder)
    app = root / 'Test.app'
    stage = root / '.vaa3d-update-test'
    incoming = stage / 'Vaa3D.app'
    bundle(app, 'old'); bundle(incoming, 'new')
    old_process = subprocess.Popen(['/bin/sleep', '2'])
    backup = root / 'backup.zip'
    install = subprocess.Popen(['/bin/bash', str(helper), str(old_process.pid), str(app), str(stage), str(backup)])
    time.sleep(0.4)
    assert (app / 'Contents/marker').read_text() == 'old'
    old_process.wait()
    assert install.wait(timeout=30) == 0
    assert (app / 'Contents/marker').read_text() == 'new'
    with zipfile.ZipFile(backup) as archive:
        assert archive.read('Test.app/Contents/marker') == b'old'
    assert not (root / '.vaa3d-install.lock').exists()
    # Invalid signature must stop before touching the installed app.
    stage = root / '.vaa3d-update-corrupt'; incoming = stage / 'Vaa3D.app'
    bundle(incoming, 'damaged'); (incoming / 'Contents/marker').write_text('tampered')
    install = subprocess.run(['/bin/bash', str(helper), '99999999', str(app), str(stage), str(root / 'failed.zip')], timeout=30)
    assert install.returncode != 0
    assert (app / 'Contents/marker').read_text() == 'new'
    assert not (root / '.vaa3d-install.lock').exists()
print('MAC_INSTALL_TEST_PASS: waits for exit, installs, backs up, rejects damaged app')
