#!/usr/bin/env python3
"""Protocol validation and isolated XDG -> handler -> launcher argument regression."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
from urllib.parse import quote, urlencode

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('vaa3d_url', ROOT / 'scripts/open-vaa3d-url.py')
handler = importlib.util.module_from_spec(spec)
spec.loader.exec_module(handler)


def url(path):
    return 'vaa3d://open?path=' + quote(str(path), safe='')


class ProtocolTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='vaa3d-protocol-test-')
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)
        self.file = self.work / '神经元 + #% & $`test` (1).SWC'
        self.file.write_text('1 1 1 1 1 1 -1\n')

    def test_local_paths_and_encoding(self):
        for ext in ('.swc', '.eswc', '.tif', '.tiff', '.v3dpbd', '.TIF'):
            path = self.file.with_suffix(ext)
            path.touch()
            self.assertEqual(handler.local_path(url(path)), str(path))
        # Literal percent escapes in a filename must not be decoded twice.
        path = self.work / 'literal%20%2F.tif'
        path.touch()
        self.assertEqual(handler.local_path(url(path)), str(path))
        self.assertEqual(handler.local_path(url(self.file).replace('open?', 'open/?')),
                         str(self.file))

    def test_invalid_links_do_not_launch(self):
        unsupported = self.work / 'bad.ano'
        unsupported.touch()
        directory = self.work / 'folder.tif'
        directory.mkdir()
        invalid = [
            '', 'http://open?path=/tmp/a.tif', 'vaa3d://other?path=/tmp/a.tif',
            'vaa3d://user@open?path=/tmp/a.tif', 'vaa3d://open:80?path=/tmp/a.tif',
            'vaa3d://open/extra?path=/tmp/a.tif', 'vaa3d://open?path=relative.tif',
            'vaa3d://open?path=file%3A%2F%2F%2Ftmp%2Fa.tif',
            'vaa3d://open?path=https%3A%2F%2Fexample.com%2Fa.tif',
            'vaa3d://open?path=', 'vaa3d://open?path=%GG',
            'vaa3d://open?path=%FF', 'vaa3d://open?path=%00.tif',
            'vaa3d://open?path=%0A.tif', url(self.file) + '#fragment',
            url(self.file) + '&path=' + quote(str(self.file)),
            url(self.file) + '&plugin=anything', url(self.file) + '\n',
            url(self.work / 'missing.tif'), url(unsupported), url(directory),
        ]
        with patch.object(handler.os, 'execv') as launch, patch.object(handler, 'report_error'):
            for value in invalid:
                with self.subTest(value=value):
                    self.assertEqual(handler.main([value]), 2)
            self.assertEqual(handler.main([]), 2)
            self.assertEqual(handler.main([url(self.file), '-x', 'plugin']), 2)
            launch.assert_not_called()
        with patch.object(handler.os, 'access', return_value=False):
            with self.assertRaises(ValueError):
                handler.local_path(url(self.file))

    def test_file_set_roles_and_atomic_validation(self):
        image=self.work/'43199 原图.tif'
        mask=self.work/'43199 mask.tiff'
        other=self.work/'43199 重建.eswc'
        for path in (image,mask,other):path.touch()
        pairs=[('mask',str(mask)),('image',str(image)),('swc',str(self.file)),
               ('swc',str(other)),('mesh_density','80')]
        link='vaa3d://open?'+urlencode(pairs)
        self.assertEqual(handler.launch_arguments(link),['--open-set','--mask',str(mask),
                         '--image',str(image),'--swc',str(self.file),'--swc',str(other),'--mesh-density','80'])
        alias=self.work/'alias.tif';alias.symlink_to(image)
        invalid=[pairs+[('path',str(image))],pairs+[('image',str(mask))],
                 pairs+[('mask',str(mask))],pairs+[('mask',str(alias))],
                 [('swc',str(self.file))],pairs+[('mesh_density','30')],
                 [('image',str(image)),('mask',str(self.file))],
                 [('image',str(image)),('swc',str(mask))],
                 pairs+[('mask',str(self.work/'missing.tif'))]]
        invalid += [[('image',str(image)),('mesh_density',s)] for s in ('','0','1001','1.5','-1','NaN')]
        with patch.object(handler.os,'execv') as launch,patch.object(handler,'report_error'):
            for pairs in invalid:
                with self.subTest(pairs=pairs):
                    self.assertEqual(handler.main(['vaa3d://open?'+urlencode(pairs)]),2)
            launch.assert_not_called()

    def test_xdg_dispatch_preserves_one_literal_argument(self):
        # Relocate the scripts to exercise Desktop Exec quoting, not just parsing.
        checkout = self.work / 'repo 空格 % $ ` " \\'
        (checkout / 'scripts').mkdir(parents=True)
        (checkout / 'bin').mkdir()
        for script in ('install-desktop-integration.py', 'open-vaa3d-url.py', 'open-vaa3d.sh'):
            shutil.copy2(ROOT / 'scripts' / script, checkout / 'scripts' / script)
        fake = checkout / 'bin/vaa3d'
        fake.write_text('#!/usr/bin/python3\nimport json, os, sys\n'
                        'with open(os.environ["VAA3D_TEST_CAPTURE"], "w") as f:\n'
                        '    json.dump(sys.argv[1:], f)\n')
        fake.chmod(0o755)
        config, data = self.work / 'config', self.work / 'data'
        config.mkdir()
        # protocol-only must leave the TIFF default alone.
        mimeapps = config / 'mimeapps.list'
        mimeapps.write_text('[Default Applications]\nimage/tiff=existing-viewer.desktop;\n')
        capture = self.work / 'captured.json'
        env = dict(os.environ, XDG_CONFIG_HOME=str(config), XDG_DATA_HOME=str(data),
                   VAA3D_TEST_CAPTURE=str(capture))
        subprocess.run([sys.executable, str(checkout / 'scripts/install-desktop-integration.py'),
                        '--protocol-only'], env=env, check=True)
        self.assertIn('image/tiff=existing-viewer.desktop;', mimeapps.read_text())
        self.assertFalse((data / 'applications/vaa3d-local.desktop').exists())
        query = subprocess.check_output(['xdg-mime', 'query', 'default',
                                         'x-scheme-handler/vaa3d'], env=env, text=True).strip()
        self.assertEqual(query, 'vaa3d-protocol.desktop')
        # Real GIO dispatch exercises registration, URL expansion, exec and wrapper.
        subprocess.run(['gio', 'open', url(self.file)], env=env, check=True)
        deadline = time.monotonic() + 10
        while not capture.exists() and time.monotonic() < deadline:
            time.sleep(0.05)
        self.assertTrue(capture.exists(), 'registered handler was not launched')
        self.assertEqual(json.loads(capture.read_text()), ['-i', str(self.file)])
        capture.unlink()
        image=self.work/'43199 +原图.v3dpbd';image.touch()
        mask=self.work/'43199 +mask.tif';mask.touch()
        link='vaa3d://open?'+urlencode([('image',str(image)),('swc',str(self.file)),('mask',str(mask))])
        subprocess.run(['gio','open',link],env=env,check=True)
        deadline=time.monotonic()+10
        while not capture.exists() and time.monotonic()<deadline:time.sleep(0.05)
        self.assertEqual(json.loads(capture.read_text()),['--open-set','--image',str(image),
                         '--swc',str(self.file),'--mask',str(mask),'--mesh-density','400'])
        # The existing full installer must still register both TIFF types.
        subprocess.run([sys.executable, str(checkout / 'scripts/install-desktop-integration.py')],
                       env=env, check=True)
        for mime in ('image/tiff', 'image/x-tiff'):
            default = subprocess.check_output(['xdg-mime', 'query', 'default', mime],
                                              env=env, text=True).strip()
            self.assertEqual(default, 'vaa3d-local.desktop')


if __name__ == '__main__':
    unittest.main(verbosity=2)
