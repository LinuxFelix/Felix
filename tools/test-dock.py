#!/usr/bin/env python3
"""Boot the rebuilt ISO, click every dock launcher and verify xterm's PTY."""
import importlib.util, pathlib, time, re
from rfb_client import Viewer
spec=importlib.util.spec_from_file_location('apps_test',pathlib.Path(__file__).with_name('test-apps.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def geometry(g,title):
    result=g.command(f"/tmp/xdrive geometry '{title}'")
    match=re.search(r'(\d+) (\d+) (\d+) (\d+) (\d+)',result)
    assert match, result
    return tuple(map(int,match.groups()))

g=module.Guest()
try:
    _,x,y,w,h=geometry(g,'PMDock')
    assert (x,y,w,h)==(408,710,208,52), 'Dock was moved or decorated by the window manager'
    g.command("/tmp/xdrive click PMDock 26 26")
    time.sleep(2)
    geometry(g,'Felix Terminal')
    g.command("/tmp/xdrive focus 'Felix Terminal'; /tmp/xdrive type 'Felix Terminal' 'echo XTERM_PTY_OK > /tmp/xterm-pty-ok'; /tmp/xdrive key 'Felix Terminal' Return")
    assert 'XTERM_PTY_OK' in g.command('cat /tmp/xterm-pty-ok'), 'xterm could not run a command through its PTY'
    g.screenshot('felix-terminal-dock.png')
    g.command('killall xterm')
    for position,title in [(78,'Felix Notepad'),(130,'Felix Settings'),(182,'Felix Applications')]:
        g.command(f'/tmp/xdrive click PMDock {position} 26')
        time.sleep(1)
        geometry(g,title)
        g.command('killall felix-apps')
    # Compare the actual client pixels after repeated overlapping window moves.
    g.command("felix-apps notepad >/tmp/notepad.log 2>&1 &")
    time.sleep(1)
    g.command("/tmp/xdrive move 'Felix Notepad' 20 40; /tmp/xdrive focus 'Felix Notepad'; /tmp/xdrive click 'Felix Notepad' 100 100; /tmp/xdrive type 'Felix Notepad' 'Moving this window must preserve these pixels.'")
    time.sleep(.5)
    _,x,y,w,h=geometry(g,'Felix Notepad')
    def crop():
        v=Viewer(port=5919);pixels=v.frame();stride=v.width*4;v.close()
        # Editor region excludes pointer, titlebar and window manager borders.
        return b''.join(pixels[(y+r)*stride+(x+10)*4:(y+r)*stride+(x+w-10)*4] for r in range(50,90))
    before=crop()
    for dx,dy in [(1,0),(-1,0),(7,3),(-7,-3),(13,9),(-13,9),(0,-11),(0,0)]:
        g.command(f"/tmp/xdrive move 'Felix Notepad' {20+dx} {40+dy}")
        time.sleep(.15)
    g.command("/tmp/xdrive move 'Felix Notepad' 20 40")
    time.sleep(.5)
    assert crop()==before, 'Client pixels changed after overlapping window moves'
    g.screenshot('felix-window-movement.png')
    print('ISO_DOCK_XTERM_MOVEMENT_TEST_OK')
finally:g.close('dock-test.log')
