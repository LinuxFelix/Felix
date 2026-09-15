# Felix website

Open `index.html` directly, or serve this directory with any static web server.
All four pages work without JavaScript. Fonts, images, the ISO and its checksum
are local. Upload the entire `web` directory to publish; no server-side code or
package manager is required.

For a local preview from this directory: `python -m http.server 8080`.

After an OS release, run `python tools/build-web.py` from the project root to
refresh the HTML, bundled ISO, checksum, fonts and images. Edit page content in
that generator; edit appearance directly in `web/style.css`.

The gallery shows five actual screenshots from the released Felix 1.0 ISO,
labeled "Felix 1.0 build". Run `python3 tools/capture-release.py` inside the
prepared Linux environment to recapture the desktop and apps, then regenerate
the site. `build/release/screenshots-1.0.json` records the source ISO checksum.

Release-note text and dates in the generator are editorial content; update them
when publishing another release. Font and icon notices are in `assets`.
