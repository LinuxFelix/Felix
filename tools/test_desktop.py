"""Run the real TinyX backend and test X11, RFB pixels and optional flwm."""
import os
from pathlib import Path
import subprocess
import time
from rfb_client import Viewer

root = Path(__file__).resolve().parents[1]
build = root / "build/linux"
env = dict(os.environ, DISPLAY=":89", FLWM_TITLEBAR_COLOR="22:55:CC", HOME=str(build / "home"))
Path(env["HOME"]).mkdir(exist_ok=True)
subprocess.run(["cc", "-Os", str(root / "userland/src/felix-root.c"), "-lX11", "-o", str(build / "felix-root")], check=True)
subprocess.run(["cc", "-Os", str(root / "userland/src/felix-about.c"), "-lX11", "-o", str(build / "felix-about")], check=True)
children = []
with (build / "tinyx-test.log").open("w") as log:
    try:
        server = subprocess.Popen([str(build / "tinyx/kdrive/remote/Xtinyremote"), ":89", "-screen", "800x600x24", "-rfbport", "5909", "-fp", "built-ins", "-noreset", "-nolisten", "tcp"], stdout=log, stderr=log, env=env)
        children.append(server)
        for _ in range(50):
            if server.poll() is not None:
                raise RuntimeError((build / "tinyx-test.log").read_text())
            result = subprocess.run([str(build / "felix-root"), "#AEE1FF"], env=env, capture_output=True)
            if result.returncode == 0:
                break
            time.sleep(.1)
        else:
            raise RuntimeError("X11 readiness timeout")
        subprocess.run(["xdpyinfo"], env=env, stdout=subprocess.DEVNULL, check=True)
        for version in (3, 7, 8):
            viewer = Viewer(port=5909, version=version)
            assert (viewer.width, viewer.height) == (800, 600)
            pixels = viewer.frame()
            # The software cursor may cover pixel (0, 0).
            assert pixels.count(bytes.fromhex("ffe1ae00")) > 800 * 600 * .95
            viewer.close()
            time.sleep(.1)
        print("PASS: X11 requests, RFB 3.3/3.7/3.8, exact light-blue pixels")
        if (build / "flwm/flwm").exists():
            children.append(subprocess.Popen([str(build / "flwm/flwm")], env=env, stdout=log, stderr=log))
            time.sleep(.3)
            app = subprocess.Popen([str(build / "felix-about")], env=env, stdout=log, stderr=log)
            children.append(app)
            time.sleep(.5)
            viewer = Viewer(port=5909)
            viewer.pointer(1, 200, 160)
            viewer.pointer(0, 200, 160)
            time.sleep(.2)
            pixels = viewer.frame()
            assert bytes.fromhex("cc552200") in pixels, "No blue titlebar found"
            viewer.screenshot(build / "felix-desktop.png")
            viewer.key(0xff1b, True)
            viewer.key(0xff1b, False)
            app.wait(timeout=5)
            viewer.close()
            print("PASS: flwm blue titlebar, real app, RFB pointer focus and Escape key")
    finally:
        for child in reversed(children):
            if child.poll() is None:
                child.terminate()
                try:
                    child.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait()
