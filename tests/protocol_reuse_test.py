#!/usr/bin/env python3
"""Run under a private xvfb-run; uses numpy/tifffile and the actual GUI binary."""
import importlib.util
import json
import os
from pathlib import Path
import signal
import socket
import subprocess
import sys
import tempfile
import time
from urllib.parse import quote, urlencode

import numpy as np
import tifffile

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'scripts/open-vaa3d-url.py'
spec=importlib.util.spec_from_file_location('handler',SCRIPT)
handler=importlib.util.module_from_spec(spec);spec.loader.exec_module(handler)


def wait_for(condition,seconds=30):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        if condition():return
        time.sleep(.05)
    raise AssertionError('Timed out waiting for GUI state')


def main():
    with tempfile.TemporaryDirectory(prefix='vaa3d-reuse-') as temp:
        work=Path(temp);runtime=work/'r';runtime.mkdir(mode=0o700)
        os.environ.update(XDG_RUNTIME_DIR=str(runtime),XDG_CONFIG_HOME=str(work/'config'),
                          LIBGL_ALWAYS_SOFTWARE='1',LC_ALL='C.UTF-8')
        os.environ['LD_PRELOAD']='/usr/lib/x86_64-linux-gnu/libstdc++.so.6'
        image=work/'43199 中文 +#原图.tif';other=work/'other.tif';mask=work/'43199_mask.tif';swc=work/'43199.swc'
        tifffile.imwrite(image,np.full((16,16,16),77,dtype=np.uint8),photometric='minisblack',metadata=None)
        tifffile.imwrite(other,np.full((16,16,16),88,dtype=np.uint8),photometric='minisblack',metadata=None)
        data=np.zeros((16,16,16),dtype=np.uint8);data[4:12,4:12,4:12]=1
        tifffile.imwrite(mask,data,photometric='minisblack',metadata=None)
        swc.write_text('1 1 5 5 5 1 -1\n2 3 10 10 10 1 1\n')
        single='vaa3d://open?path='+quote(str(other),safe='')
        group='vaa3d://open?'+urlencode([('image',str(image)),('swc',str(swc)),('mask',str(mask)),('mesh_density','20')])
        directory=handler.ipc_directory()
        def live():
            result=[]
            for endpoint in directory.glob('*.sock'):
                try:
                    with socket.socket(socket.AF_UNIX) as connection:
                        connection.settimeout(.2);connection.connect(str(endpoint))
                    result.append(endpoint)
                except OSError:pass
            return result
        def run(link):
            return subprocess.Popen(['/usr/bin/python3',str(SCRIPT),link],stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        pids=set()
        try:
            # Simultaneous cold clicks must start exactly one GUI and queue both files.
            clients=[run(single),run(group)]
            for client in clients:
                output,error=client.communicate(timeout=40)
                assert client.returncode==0,(output,error)
            endpoints=live();assert len(endpoints)==1,endpoints
            endpoint=endpoints[0];pid=int(endpoint.stem);pids.add(pid)
            def loaded():
                text=subprocess.check_output(['xwininfo','-root','-tree'],text=True)
                settings=work/'config/HHMI/Vaa3D.conf'
                return 'Object Manager' in text and image.name in text and settings.exists() and other.name in settings.read_text()
            wait_for(loaded)
            # Repeat the set and open another single file: still the same process.
            for link in (group,single,group):
                client=run(link);out,err=client.communicate(timeout=15);assert client.returncode==0,(out,err)
            assert [int(p.stem) for p in live()]==[pid]
            # The receiver itself rejects unsupported commands; no plugin invocation.
            with socket.socket(socket.AF_UNIX) as connection:
                connection.settimeout(5);connection.connect(str(endpoint))
                connection.sendall((json.dumps({'version':1,'id':'a'*32,'args':['-x','plugin']})+'\n').encode())
                assert json.loads(connection.recv(4096))['status']=='rejected'
            print('PROTOCOL_REUSE_PASS: concurrent cold start, same PID for repeated single/set links, real image/SWC/mask/manager loading, invalid command rejected',flush=True)
        finally:
            for endpoint in directory.glob('*.sock'):
                try:pids.add(int(endpoint.stem))
                except ValueError:pass
            for pid in pids:
                try:os.kill(pid,signal.SIGTERM)
                except ProcessLookupError:pass


if __name__=='__main__':main()
