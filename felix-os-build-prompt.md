# Build Prompt: "Felix" — a smolBSD-based ultra-lightweight GUI OS

You are an expert NetBSD/rump-kernel systems engineer with deep experience in **smolBSD** (https://github.com/NetBSDfr/smolBSD), **TinyX**, and **flwm**. Your task is to design and build a complete, working microvm-based operating system called **Felix**.

Read this entire prompt before writing any code. Ask clarifying questions only if something below is genuinely ambiguous — otherwise proceed and make reasonable, clearly-documented assumptions.

---

## 1. Project goal

Build **Felix**, a graphical NetBSD micro-OS on top of smolBSD that:

- Boots as a smolBSD microvm (QEMU / Firecracker target, PVH boot path preferred for fast startup).
- Starts an X server using **TinyX** (the stripped-down Xorg/Xserver used in embedded contexts — Xfbdev or Xvesa/kdrive-derived server, whichever smolBSD's package set supports).
- Runs **flwm** (the Flexible Window Manager) as the sole window manager.
- Has a **total image size under 29 MB** (this is a hard budget — treat it as a constraint to optimize against at every step, not an afterthought).
- Keeps all userland application/session code inside a folder literally named **`userland/`** in the project tree.
- Ships an flwm configuration that is **deliberately minimal**: no themes, no unnecessary decorations, and a **two-tone blue color scheme** — a **blue** window/titlebar accent color and a **light blue** desktop background.

---

## 2. Deliverables

Produce all of the following, as actual files in a working repository layout (not just descriptions):

1. **`smolerfiles/Dockerfile.felix`** — a SMOLerfile (Docker-style build recipe) that:
   - Uses `FROM base,etc` (or the minimal set combination needed — justify any additional set like `comp` or `xbase` if strictly required, but prefer trimming).
   - Installs TinyX and flwm via `pkgin` from NetBSD pkgsrc, using the smallest possible package set (e.g. `xserver-tinyx` or the closest pkgsrc equivalent, plus `flwm`, plus minimal font/util dependencies like `font-misc-misc`, `mkfontscale`, `xterm` *only if it fits the budget* — otherwise drop xterm).
   - Sets `LABEL smolbsd.service=felix`.
   - Sets `LABEL smolbsd.minimize=y` so the image is trimmed to actual content.
   - Has a `CMD` that launches the Felix session (see `felix-session` below) instead of a shell.

2. **`service/felix/`** directory (matching smolBSD's per-service layout), containing:
   - `README.md` explaining what Felix is, how to build it, how to run it, and the size budget.
   - `options.mk` with `IMGSIZE` set as small as is workable (start at e.g. `48` or `64` and iterate down; document the final chosen value and why).
   - Any rc.d scripts needed to launch the graphical session at boot instead of a login shell.

3. **`userland/`** directory at the project root, containing all Felix-specific userland files:
   - `userland/bin/felix-session` — a small shell script (or minimal C binary if shell adds too much runtime weight — your call, document the tradeoff) that:
     1. Starts the TinyX server on the correct framebuffer/VT.
     2. Waits for the X socket to become available.
     3. Sets the root window background to light blue via `xsetroot` (or the smallest tool capable of this — do not pull in a full `feh`/`nitrogen` just for a solid color).
     4. Launches `flwm` with `userland/etc/flwm/flwmrc` (see below) as its config.
     5. Optionally execs a minimal app (e.g. an `xterm` or a custom launcher) — keep optional and size-gated.
   - `userland/etc/skel/.wmx/` — the minimal flwm menu directory (symlinks/scripts), per Section 3c.
   - `userland/etc/X11/xorg.conf` (or the TinyX-equivalent minimal config file) with only the sections needed: a single framebuffer/VESA-class device, one screen, no unused input/extension sections.
   - `service/felix/patches/flwm-config.h` (or a patch against `tinycorelinux/flwm`'s `config.h`) with `ACTIVE_COLOR` set to blue and `TOPSIDE` enabled, per Section 3a — only needed if you choose the build-from-source path rather than a pkgsrc binary.

4. **A size-budget worksheet** (`SIZE_BUDGET.md`) breaking down, in a table, the approximate contribution of each component to the final image:
   - NetBSD base set (trimmed)
   - rump kernel / boot components
   - TinyX server + minimal fonts
   - flwm binary + libs (Xlib, Xext, etc.)
   - Felix userland scripts/configs
   - Free headroom remaining under the 29 MB cap

   Call out explicitly which pkgsrc packages are the biggest size risks (X servers and font packages usually are) and what you stripped to compensate (e.g. drop all bitmap fonts except one fixed-width core font, strip locale data, strip man pages/docs via smolbsd.minimize, avoid pulling in `xkbcomp`/`xkeyboard-config` if a hardcoded minimal keymap will do).

5. **Build & run instructions**, written as a copy-pasteable shell transcript, e.g.:
   ```
   git clone https://github.com/NetBSDfr/smolBSD
   cd smolBSD
   cp -r /path/to/felix/smolerfiles/Dockerfile.felix smolerfiles/
   cp -r /path/to/felix/service/felix service/
   cp -r /path/to/felix/userland .
   ./smoler.sh build smolerfiles/Dockerfile.felix
   ./smoler.sh run felix-amd64:latest
   ```
   Include the equivalent classic-Makefile workflow too (`bmake SERVICE=felix build`, `./startnb.sh -f etc/felix.conf`).

6. **A short verification checklist** the user can run after boot to confirm success (X server responds, `flwm` process is running, background is light blue via `xsetroot`, window titlebars render blue via `ACTIVE_COLOR`/`TOPSIDE`, right-click menu shows the `.wmx` entries, image file size check via `ls -lh`).

---

## 3. flwm styling requirements — model this on **real TinyCore Linux**, not a fictional `flwmrc`

Important factual correction to build from: **stock flwm has no live text config file** (no `~/.flwmrc` with directives the way fluxbox/openbox have). TinyCore Linux — whose exact look Felix should copy — actually configures flwm through **three separate real mechanisms**. Implement Felix's theme using these three, and name/comment each file so it's obvious which mechanism it maps to:

**a) Compile-time colors via `config.h` (the "TinyCore fork" approach)**
TinyCore ships its own fork of flwm (`tinycorelinux/flwm` on GitHub) with a `config.h` that has hand-editable `#define`s controlling appearance, e.g.:
- `#define ACTIVE_COLOR 0xF0F0F0` — the active-window titlebar / highlighted-menu-item color, as a `0xRRGGBB` hex literal.
- `#define TITLE_FONT_SIZE 14`, border thickness defines (`LEFT`/`RIGHT`/`TOP`/`BOTTOM`), button decorations, etc.
- A `TOPSIDE` build variant, which is what real TinyCore uses (normal horizontal titlebar on top of the window, not flwm's original sideways wm2-style bar) — build with `TOPSIDE` defined to match TinyCore's actual look.
- TinyCore's fork also carries an **`ML_TITLEBAR_COLOR`** feature flag ("use environment variable to set titlebar color") in `config.h`. If you build from `tinycorelinux/flwm` with this flag on, find the exact environment variable name it reads in the source (grep `Colors.C` / `main.C` for the corresponding `getenv()` call) and use that env var to set the titlebar to **blue** at runtime from `felix-session`, rather than guessing the variable name. If you can't confirm it from source, fall back to hardcoding `ACTIVE_COLOR` in `config.h` at build time instead and say so in the README.
- Vendor `service/felix/patches/flwm-config.h.patch` (or a full replacement `config.h`) with `ACTIVE_COLOR` set to a blue value (e.g. `0x2255CC`) as the primary, guaranteed-to-work path; treat the env-var route as a nice-to-have on top of it.

**b) Desktop background via `.xsession`, exactly like TinyCore does**
TinyCore does **not** set the background from flwm at all — it's set immediately before launching flwm in the session startup script, historically:
```sh
#!/bin/sh
xsetroot -solid \#006060   # TinyCore's own default is a dark teal
xrdb .Xresources
flwm &
WindowManager=$!
wait $WindowManager
```
Felix's `userland/bin/felix-session` should follow this exact same pattern but with light blue instead of TinyCore's teal, e.g. `xsetroot -solid \#AEE1FF`, run before `exec flwm` (or `flwm &` + `wait`, matching TinyCore's own logout-on-exit behavior — pick one and document the choice).

**c) Menu items via a `~/.wmx` directory, not a config file**
flwm has no menu-definition file either — right-click/menu entries come from **symlinks or small scripts placed in a `.wmx` directory** in `$HOME` (`WMX_MENU_ITEMS` in `config.h`). For Felix, create `userland/etc/skel/.wmx/` with a minimal set of symlinks/scripts (e.g. one entry to launch a terminal, if a terminal is included at all under the size budget) so the right-click menu isn't empty, exactly mirroring how TinyCore populates its own flwm menu.

**d) Optional authentic touch: `wbar`**
Real TinyCore/"TinyCore" desktop = `Xvesa + Xprogs + aterm + fltk + flwm + wbar`, where **wbar** is the small icon dock/taskbar at the bottom of the screen — it is a separate binary from flwm, not part of it. Decide explicitly whether Felix includes `wbar` for full TinyCore-authenticity or omits it to save space, and justify the choice against the 29 MB budget in `SIZE_BUDGET.md` either way. Default recommendation: **omit it** to protect the size budget, and say so.

**Summary of the required look:**
- Titlebar / active-window accent: **blue** (`0x2255CC` or similar), set via `config.h`'s `ACTIVE_COLOR` (and optionally the runtime env var, if confirmed to exist).
- Desktop background: **light blue** (`#AEE1FF` or similar), set via `xsetroot -solid` in the `.xsession`-equivalent startup script — same mechanism TinyCore itself uses, just a different color.
- `TOPSIDE` titlebar build, minimal borders, no taskbar/pager beyond flwm's own popup menu (unless `wbar` is explicitly kept).
- A populated-but-tiny `.wmx` menu directory so the desktop isn't menu-less.

Do not invent a `flwmrc` text file with directives like `borderColor=` — that format does not exist for flwm. If you find during implementation that a given NetBSD pkgsrc `flwm` package is prebuilt (not TinyCore's fork) and therefore lacks `config.h`-level customization at install time, say so explicitly and propose building flwm from the `tinycorelinux/flwm` source inside the SMOLerfile instead of installing a pkgsrc binary, weighing the size/complexity tradeoff either way.

---

## 4. Constraints & priorities, in order

1. **Correctness first**: it must actually boot, start X, and show a blue-on-light-blue flwm desktop.
2. **Size second**: every added file/package must be justified against the 29 MB cap. When in doubt, cut.
3. **Simplicity third**: prefer the fewest moving parts (no extra compositors, panels, or daemons) over "nice to have" polish.
4. Don't invent smolBSD mechanisms that don't exist — base all Makefile/SMOLerfile/rc.d integration on smolBSD's actual documented structure (`smolerfiles/`, `service/<name>/`, `Makefile`, `mkimg.sh`, `startnb.sh`, `sets/`, `pkgs/`, `smoler.sh build/run/push/pull`). If you're not certain a given pkgsrc package name is correct, say so and give your best real candidate rather than fabricating a package.
5. Flag anywhere the 29 MB target may be genuinely hard to hit with a full X stack, and propose fallback options (e.g. a smaller/older TinyX variant, framebuffer-only X without VESA DDC probing, stripping locale/i18n support from libX11) rather than quietly blowing the budget.

---

## 5. Output format

Respond with the full repository tree first (a simple directory listing), then the full contents of every file listed in Section 2, each in its own clearly labeled code block, followed by `SIZE_BUDGET.md`, then the build/run transcript, then the verification checklist.
