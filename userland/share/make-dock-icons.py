#!/usr/bin/env python3
"""Write small, code-defined XPM icons in Felix's existing two-tone palette."""
from pathlib import Path
out=Path(__file__).with_name('dock');out.mkdir(exist_ok=True)
def save(name,pixels):
    rows=['/* XPM */','/* Felix, MIT license */','static const char *icon[] = {',f'"{len(pixels[0])} {len(pixels)} 4 1",','". c None",','"b c #2255CC",','"l c #AEE1FF",','"w c #FFFFFF",']
    rows += ['"'+''.join(row)+'"'+(',' if i+1<len(pixels) else '') for i,row in enumerate(pixels)]
    (out/(name+'.xpm')).write_text('\n'.join(rows+['};','']))
tile=[['l']*52 for _ in range(52)]
for i in range(52):tile[0][i]=tile[51][i]=tile[i][0]=tile[i][51]='b'
save('tile',tile)
patterns={
 'terminal':['bbbbbbbbbbbb','bwwwwwwwwwwb','bbbbbbbbbbbb','b..........b','b.b........b','b..b.......b','b...b......b','b..b.......b','b.b..bbbb..b','b..........b','b..........b','bbbbbbbbbbbb'],
 'notepad':['..bbbbbbbb..','..bwwwwwwbb.','..bwwwwwwwwb','..bwbbbbbbwb','..bwwwwwwwwb','..bwbbbbbbwb','..bwwwwwwwwb','..bwbbbbbbwb','..bwwwwwwwwb','..bwbbbbbbwb','..bwwwwwwwwb','..bbbbbbbbbb'],
 'settings':['..bb....bb..','..bb....bb..','..bb...wwww.','.wwww...bb..','..bb....bb..','..bb....bb..','..bb....bb..','..bb....bb..','..bb....bb..','.wwww...bb..','..bb....bb..','..bb....bb..'],
 'applications':['....bbbb....','...bwwwwb...','..bbbbbbbb..','.bllbblllb..','blllbblllllb','blllbblllllb','bllllllllllb','blllwwwwlllb','bllllwwllllb','bllllllllllb','bllllllllllb','bbbbbbbbbbbb']}
for name,pattern in patterns.items():
    assert all(len(row)==12 for row in pattern)
    pixels=[['.']*32 for _ in range(32)]
    for y,row in enumerate(pattern):
        for x,c in enumerate(row):
            for dy in range(2):
                for dx in range(2):pixels[4+y*2+dy][4+x*2+dx]=c
    save(name,pixels)
