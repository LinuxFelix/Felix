#!/usr/bin/env python3
"""Generate the static Felix site and copy its local release/assets. No runtime JS."""
from pathlib import Path
from html import escape
import hashlib
import shutil

root = Path(__file__).resolve().parents[1]
web = root / 'web'
theme = root / 'userland/share/theme'
release = root / 'build/release'
for folder in ('assets', 'downloads', 'images'):
    (web / folder).mkdir(parents=True, exist_ok=True)
for name in ('DejaVuSans.ttf', 'DejaVuSansMono.ttf', 'FONT-LICENSE.txt', 'ICONS-LICENSE.txt',
             'terminal.png', 'applications.png', 'settings.png', 'notepad.png'):
    shutil.copy2(theme / name, web / 'assets' / name)
shutil.copy2(theme / 'boot.png', web / 'images/theme-preview.png')
shots = [('felix-1.0-desktop.png', 'Desktop', 'Felix 1.0 build: glass-style desktop, left-side dock and clock.'),
         ('felix-1.0-notepad.png', 'Notepad', 'Felix 1.0 build: text editing with Find and word wrap.'),
         ('felix-1.0-settings.png', 'Settings', 'Felix 1.0 build: desktop and wallpaper preferences.'),
         ('felix-1.0-packages.png', 'Applications', 'Felix 1.0 build: search and install software.'),
         ('felix-1.0-terminal.png', 'Terminal', 'Felix 1.0 build: tinted terminal with DejaVu text.')]
for name, _, _ in shots:
    shutil.copy2(release / name, web / 'images' / name)
iso = release / 'felix-1.1-x86.iso'
digest = hashlib.sha256(iso.read_bytes()).hexdigest()
size = iso.stat().st_size
shutil.copy2(iso, web / 'downloads' / iso.name)
shutil.copy2(root / 'RELEASE-1.1.md', web / 'downloads' / 'RELEASE-1.1.md')
(web / 'downloads' / (iso.name + '.sha256')).write_text(f'{digest}  {iso.name}\n')
pages = [('index.html', 'Home', 'terminal.png'), ('downloads.html', 'Downloads', 'applications.png'),
         ('about.html', 'About', 'settings.png'), ('screenshots.html', 'Screenshots', 'notepad.png')]

def write_page(filename, title, content):
    nav = ''.join(f'<li><a href="{name}"'+(' aria-current="page"' if name == filename else '')+
                  f'><img src="assets/{icon}" width="24" height="24" alt="">{label}</a></li>'
                  for name, label, icon in pages)
    html = f'''<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta name="description" content="Felix: a small 32-bit Linux desktop with TinyX, flwm and FLTK apps. Downloads, project information and screenshots.">
  <title>{title} - Felix</title>
  <link rel="stylesheet" href="style.css">
</head>
<body>
<a class="skip" href="#content">Skip to content</a>
<div class="site">
  <header>
    <div class="titlebar"><span>felix / {title.lower()}</span><span class="window-marks" aria-hidden="true">&#8722; &#9633; &#215;</span></div>
    <div class="masthead"><a class="wordmark" href="index.html">felix<span>small system. your space.</span></a><p>Felix 1.1 / x86<br><span class="muted">A little desktop of your own.</span></p></div>
  </header>
  <div class="layout">
    <aside>
      <nav aria-label="Main navigation"><h2>Directory</h2><ul>{nav}</ul></nav>
      <section class="sidebar-note"><h2>On this disk</h2><p>Linux 6.18<br>TinyX + flwm<br>FLTK apps<br>DejaVu fonts</p></section>
      <p class="badge">32-BIT LINUX</p><p class="badge">100% SCRIPT-FREE</p>
    </aside>
    <main id="content">{content}</main>
  </div>
  <footer><span>Felix &middot; small system. your space.</span><span>Plain HTML + CSS &middot; <a href="about.html#credits">Credits</a></span></footer>
</div>
</body>
</html>
'''
    (web / filename).write_text(html, encoding='utf-8')

write_page('index.html', 'Home', f'''
<p class="eyebrow">WELCOME TO FELIX</p>
<h1>A small desktop.<br>A place to start.</h1>
<p class="intro">Felix is a small 32-bit Linux system with a real desktop, a few useful apps, and room to make it yours.</p>
<p>Write a note. Open a terminal. Change your wallpaper. Start with the live CD, or install Felix on a hard drive or SSD.</p>
<p class="download-line"><a class="button" href="downloads.html">Get Felix &raquo;</a><span>{size/1e6:.2f} MB ISO &middot; BIOS / legacy boot</span></p>
<div class="rule-heading"><h2>What's inside?</h2><span>small tools, familiar windows</span></div>
<dl class="features">
<dt>A lightweight desktop</dt><dd>TinyX, flwm and a wbar dock on the left. Pale Aqua-inspired surfaces, DejaVu text and a softly tinted terminal.</dd>
<dt>Apps for the everyday</dt><dd>Notepad, Settings, WiFConfig, an Applications installer and a terminal.</dd>
<dt>A home on your disk</dt><dd>The installer creates an ext4 system with a user account and password-protected sudo access.</dd>
</dl>
<section class="news"><h2>Felix 1.1 is here</h2><p class="date">17 September 2026</p><p>Broader Ethernet support, Wi-Fi hardware assistance, smaller PNG artwork and gentle window animations with a reduced-motion option. A smaller image with the familiar Aqua-inspired desktop.</p><p><a href="downloads.html#release-notes">Read the release notes &raquo;</a></p></section>
<p class="small">Curious first? Visit <a href="about.html">About Felix</a> or browse the <a href="screenshots.html">screenshots</a>.</p>
''')

write_page('downloads.html', 'Downloads', f'''
<p class="eyebrow">TAKE A COPY</p><h1>Downloads</h1>
<p class="intro">One small ISO. A complete Felix desktop.</p>
<section class="download-box"><h2>Felix 1.1 / x86</h2>
<p><a class="button" href="downloads/{iso.name}" download>Download ISO &raquo;</a></p>
<p>{size/1e6:.2f} MB &middot; {size:,} bytes &middot; 32-bit x86</p>
<p><a href="downloads/{iso.name}.sha256" download>SHA-256 checksum file</a></p></section>
<h2>Before you boot</h2>
<table><caption>Current boot target</caption><tbody>
<tr><th scope="row">Boot mode</th><td>BIOS / legacy. UEFI-only boot is not supported.</td></tr>
<tr><th scope="row">Tested VM setup</th><td>512 MB RAM, standard VGA, PS/2 input and an Intel e1000 Ethernet adapter.</td></tr>
<tr><th scope="row">Network</th><td>Ethernet, plus selected Wi-Fi adapters through wpa_supplicant.</td></tr>
<tr><th scope="row">Storage</th><td>Live ISO, or a whole-disk ext4 installation.</td></tr>
</tbody></table>
<p>The live system runs in RAM. Without persistence, files and settings disappear when it shuts down. A disk installation keeps them on disk.</p>
<h2>Install on a drive</h2><p>Boot the ISO, connect Ethernet and select <strong>Install Felix</strong> in the desktop menu. The wizard guides you through disk selection, your account and the ext4 layout.</p>
<p class="notice"><strong>The installer erases the entire selected drive.</strong> Back up its contents first. It supports eligible 2 GiB–2 TiB drives and requires Ethernet to download installation dependencies.</p>
<h2>Verify your download</h2><p>Save the ISO and checksum file in the same directory, then run:</p>
<pre><code>sha256sum -c {iso.name}.sha256</code></pre>
<p class="small">SHA-256:</p><p class="checksum"><code>{digest}</code></p>
<section id="release-notes"><h2>Release notes / 17 September 2026</h2>
<ul><li>More Ethernet drivers, automatic DHCP recovery and adapter status in Settings.</li><li>Wi-Fi hardware identification and optional firmware downloads, with explicit driver reload.</li><li>Lightweight opening and closing animations, plus a Reduce motion setting.</li><li>Smaller PNG wallpapers and icons, Notepad Find next and Save As shortcuts.</li><li>Fixes for stale DHCP process IDs and Wi-Fi hardware-window administrator launch.</li></ul>
<p><a href="downloads/RELEASE-1.1.md">Full release notes and supported configuration</a></p>
<p class="small">Release checks cover desktop and console boot, popup memory safety, six virtual Ethernet models, and disk installation. Physical Ethernet and Wi-Fi hardware remain unverified. Screenshots show the labeled 1.0 build.</p></section>
''')

write_page('about.html', 'About', '''
<p class="eyebrow">A LITTLE ABOUT THE PROJECT</p><h1>This is Felix.</h1>
<p class="intro">A small Linux desktop built around simple, native tools.</p>
<p>Felix combines a tinyconfig-based, 32-bit Linux 6.18 kernel with TinyX, the flwm window manager and FLTK applications. The bootable ISO stays below 29 MB.</p>
<p>The desktop uses pale Aqua-inspired windows with a wbar dock on the left. DejaVu Sans handles the windows and controls; DejaVu Sans Mono handles your text. The terminal has a tinted wallpaper background.</p>
<h2>A few useful applications</h2>
<dl class="features"><dt>Notepad</dt><dd>Create, open and save plain text documents.</dd>
<dt>Settings</dt><dd>Adjust the mouse, background, keyboard, wired network, date and time.</dd>
<dt>WiFConfig</dt><dd>Scan for Wi-Fi, connect and manage saved profiles using wpa_supplicant.</dd>
<dt>Applications</dt><dd>Find and install packages from compatible Alpine 3.23 x86 repositories.</dd>
<dt>Terminal</dt><dd>Alpine rxvt-unicode, with xterm also available.</dd>
<dt>Install Felix</dt><dd>Set up a hard drive or SSD with ext4, a user account and sudo.</dd></dl>
<h2>Live or installed</h2><p>The live ISO is a quick way to try Felix. Installed Felix boots from its drive and uses a regular disk-backed root filesystem. Your documents, packages and settings stay there across reboots.</p>
<h2>Keep it small</h2><p>Felix uses framebuffer graphics without DRM or a compositor. Its minimal kernel and X server do not support every device or every modern desktop application. The current target is BIOS/legacy boot.</p>
<section id="credits"><h2>Built with</h2><p>Linux, Alpine Linux, TinyX, flwm, FLTK, wbar, rxvt-unicode, xterm, wpa_supplicant, Syslinux and the work of their contributors.</p>
<p>This site uses Felix's DejaVu fonts and Tango icons. <a href="assets/FONT-LICENSE.txt">Font notices</a> &middot; <a href="assets/ICONS-LICENSE.txt">Icon notices</a></p>
<p class="small">This website is plain HTML and CSS. No JavaScript, trackers, external fonts or build step needed to serve it.</p></section>
''')

gallery = ''.join(f'''<figure><a href="images/{name}"><img src="images/{name}" alt="{escape(desc)}" loading="lazy"></a><figcaption><strong>{title}</strong> &mdash; {desc} <a href="images/{name}">Full size</a></figcaption></figure>''' for name,title,desc in shots)
write_page('screenshots.html', 'Screenshots', f'''
<p class="eyebrow">A LOOK AROUND</p><h1>Screenshots</h1>
<p class="intro">Felix 1.0 build</p>
<p>Actual screenshots captured from the released Felix 1.0 ISO, showing the desktop and included applications.</p>
<p class="small">Click an image to open its full-size version.</p>{gallery}
''')
print(f'WEB_BUILT: 4 HTML pages, local fonts/images, {size:,}-byte ISO; SHA256 {digest}')
