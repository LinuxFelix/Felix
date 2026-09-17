#!/usr/bin/env python3
"""Generate Felix's simple gradient branding; copy unmodified Tango app icons."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import shutil
import math
root=Path(__file__).resolve().parents[1]
out=root/'userland/share/theme'
out.mkdir(parents=True,exist_ok=True)
builder=Path('/var/tmp/felix-linux/alpine')
font=builder/'usr/share/fonts/dejavu/DejaVuSans.ttf'
shutil.copyfile(font,out/'DejaVuSans.ttf')
icons=builder/'usr/share/icons/Tango/32x32'
for name,source in {'terminal':'apps/utilities-terminal.png','notepad':'apps/accessories-text-editor.png','settings':'categories/preferences-system.png','applications':'apps/system-software-update.png','wifi':'devices/network-wireless.png'}.items():
    Image.open(icons/source).resize((24,24),Image.Resampling.LANCZOS).save(out/(name+'.png'),optimize=True)
def glass_wallpaper(top, bottom):
    """Pre-render reflections and ordered RGB565 dithering; no desktop timer."""
    image=Image.new('RGB',(1024,768))
    pixels=image.load()
    bayer=((0,8,2,10),(12,4,14,6),(3,11,1,9),(15,7,13,5))
    curves=[430+70*math.sin(x/420) for x in range(1024)]
    for y in range(768):
        t=y/767
        base=[a+(b-a)*t for a,b in zip(top,bottom)]
        for x in range(1024):
            distance=y-curves[x]
            reflection=max(0,1-abs(distance)/145)*13
            if 0<=distance<2:reflection+=9
            threshold=(bayer[y&3][x&3]+.5)/16
            # Quantize before packaging so 16-bit framebuffer conversion
            # preserves the spatial dither instead of restoring broad bands.
            values=[]
            for channel,levels in zip(base,(31,63,31)):
                value=max(0,min(255,channel+reflection))
                q=min(levels,int(value*levels/255+threshold))
                values.append(round(q*255/levels))
            pixels[x,y]=tuple(values)
    return image
image=glass_wallpaper((30,87,137),(139,207,227));draw=ImageDraw.Draw(image)
font_big=ImageFont.truetype(str(font),66)
box=draw.textbbox((0,0),'felix',font=font_big);x=(1024-box[2])/2;y=330
draw.text((x+1,y+2),'felix',font=font_big,fill=(60,121,159))
draw.text((x,y),'felix',font=font_big,fill=(235,248,255))
small=ImageFont.truetype(str(font),11)
text='small system. your space.'
b=draw.textbbox((0,0),text,font=small)
draw.text(((1024-b[2])/2,414),text,font=small,fill=(220,242,252))
image.save(out/'wallpaper.png',optimize=True)
for filename,top,bottom in (
    ('midnight.png',(13,25,49),(54,84,114)),
    ('silver.png',(107,133,156),(209,227,235))):
    variant=glass_wallpaper(top,bottom)
    label=ImageDraw.Draw(variant)
    label.text((x,y),'felix',font=font_big,fill=(235,248,255))
    variant.save(out/filename,optimize=True)
splash=Image.new('RGB',(640,480));d=ImageDraw.Draw(splash)
for y in range(480):
    t=y/479;d.line((0,y,639,y),fill=tuple(round(a+(b-a)*t) for a,b in zip((34,88,140),(117,183,216))))
d.text((58,32),'felix',font=ImageFont.truetype(str(font),42),fill=(223,230,236))
d.text((60,85),'VERSION 1.1  /  32-BIT',font=small,fill=(161,181,200))
splash.save(out/'boot.png')
dock=Image.new('RGBA',(128,64));d=ImageDraw.Draw(dock)
d.rounded_rectangle((1,1,126,62),radius=14,fill=(166,202,227,175),outline=(244,252,255,215),width=1)
d.rounded_rectangle((4,3,123,30),radius=11,fill=(240,251,255,120))
d.line((17,3,110,3),fill=(255,255,255,220),width=1)
dock.save(out/'dock.png')
shutil.copyfile(font.parent/'DejaVuSansMono.ttf',out/'DejaVuSansMono.ttf')
for name in ('wallpaper.png','midnight.png','silver.png'):
    size=(out/name).stat().st_size
    if size>600000:raise RuntimeError(f'{name} exceeds the 600 KB wallpaper budget')
    print(f'{name}: {size} bytes')
print('THEME_ASSETS_OK')
