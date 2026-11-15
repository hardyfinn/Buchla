#!/usr/bin/env python3
# Draws res/Buchla259t.svg. VCV Rack can't draw SVG <text>, so every label is
# turned into outline paths with fontTools. Positions are in mm and must match
# src/Buchla259t.cpp (the numbers in the LAYOUT block are the same in both).
#   pip install fonttools
#   python3 res/make_panel.py
import os
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

HERE = os.path.dirname(os.path.abspath(__file__))
FONT_BOLD = os.environ.get("PANEL_FONT_BOLD", "/usr/share/fonts/opentype/inter/Inter-Bold.otf")
FONT_MED = os.environ.get("PANEL_FONT_MED", "/usr/share/fonts/opentype/inter/Inter-Medium.otf")

W, H = 172.72, 128.5  # 34 HP

BG = "#1E2230"         # deep blue-black
BG_DARK = "#151824"
INK = "#F2ECDF"
SOFT = "#A7A497"
MODC = "#4FA3D9"       # modulation oscillator: blue
CTRC = "#E9B949"       # modulation routes: amber
PRNC = "#E2603F"       # principal: red-orange
TIMC = "#6CC08B"       # timbre: green

# ---- LAYOUT (same numbers in src/Buchla259t.cpp) ----
COL = [13.0, 34.0, 55.0, 75.5, 97.0, 117.7, 138.7, 159.7]
CTR_X = 86.25
RA, RB, RC = 32.0, 52.0, 73.5
JD, JE, JF = 88.5, 102.5, 116.5

CONTROL = 3.2   # control label size (project standard)
JACK = 2.9      # jack label size (project standard)

fonts = {}


def text(s, x, y, size, font=FONT_BOLD, fill=INK, anchor="middle", spacing=0.04):
    """SVG path for string s; (x, y) is the baseline point in mm."""
    if font not in fonts:
        f = TTFont(font)
        fonts[font] = (f, f.getGlyphSet(), f.getBestCmap(), f["head"].unitsPerEm)
    f, gs, cmap, upm = fonts[font]
    scale = size / upm
    track = spacing * upm
    names = [cmap[ord(c)] for c in s]
    width = sum(gs[n].width + track for n in names) - track
    x0 = {"middle": x - width * scale / 2, "end": x - width * scale}.get(anchor, x)
    pen = SVGPathPen(gs)
    cx = 0.0
    for n in names:
        gs[n].draw(TransformPen(pen, (scale, 0, 0, -scale, x0 + cx * scale, y)))
        cx += gs[n].width + track
    return f'<path fill="{fill}" d="{pen.getCommands()}"/>'


parts = []
add = parts.append
add(f'<rect width="{W}" height="{H}" fill="{BG}"/>')

# section panels (rounded boxes with a coloured top rule)
def section(x0, x1, y0, y1, col, name):
    add(f'<rect x="{x0}" y="{y0}" width="{x1 - x0}" height="{y1 - y0}" rx="1.6" fill="{BG_DARK}"/>')
    add(f'<rect x="{x0}" y="{y0}" width="{x1 - x0}" height="0.9" fill="{col}"/>')
    if name:
        add(text(name, x0 + 2.0, y0 + 4.4, 2.6, FONT_BOLD, col, anchor="start", spacing=0.08))

MOD_X0, MOD_X1 = 2.6, COL[2] + 10.0
CTR_X0, CTR_X1 = MOD_X1 + 1.6, COL[4] + 9.6
PRN_X0, PRN_X1 = CTR_X1 + 1.6, W - 2.6
section(MOD_X0, MOD_X1, 14.6, 122.0, MODC, "MODULATION OSCILLATOR")
section(CTR_X0, CTR_X1, 14.6, 122.0, CTRC, "MODULATION")
section(PRN_X0, PRN_X1, 14.6, 59.0, PRNC, "PRINCIPAL OSCILLATOR")
section(PRN_X0, PRN_X1, 60.4, 122.0, TIMC, "")

# title
add(text("Buchla 259t", 4.0, 10.8, 5.6, FONT_BOLD, INK, anchor="start", spacing=0.03))
add(text("complex waveform generator", W - 4.0, 10.4, 2.6, FONT_MED, SOFT, anchor="end"))
# little waveform glyph: a sine folding over
gx, gy = 41.5, 8.6
add(f'<path d="M{gx} {gy} c1.2 -4 2.4 -4 3 -1.5 c0.6 2.5 1.2 2.5 1.8 0 c0.6 -2.5 1.8 -2.5 3 1.5" '
    f'stroke="{TIMC}" stroke-width="0.55" fill="none" stroke-linecap="round"/>')

def klabel(s, x, y, big=False, col=INK):
    add(text(s, x, y - (9.4 if big else 7.6), CONTROL, fill=col))

def tlabel(s, x, y, col=INK):          # trimpot label
    add(text(s, x, y - 5.4, CONTROL, fill=col))

def swlabel(s, x, y, top, bottom, col=INK):
    add(text(s, x, y - 6.4, CONTROL, fill=col))
    add(text(top, x + 3.6, y - 1.4, 2.3, FONT_MED, SOFT, anchor="start"))
    add(text(bottom, x + 3.6, y + 3.0, 2.3, FONT_MED, SOFT, anchor="start"))

def jlabel(s, x, y, col=SOFT):
    add(text(s, x, y - 5.6, JACK, FONT_BOLD, col))

# ---- Modulation oscillator
klabel("FREQ", COL[0], RA, big=True)
klabel("FINE", COL[1], RA)
swlabel("RANGE", COL[2], RA, "HI", "LO")
add(text("WAVE", COL[0] - 2.0, RB - 7.4, CONTROL))
for i, n in enumerate(["SAW", "SQR", "TRI"]):
    add(text(n, COL[0] + 1.8, RB - 2.6 + i * 3.3, 2.3, FONT_MED, SOFT, anchor="start"))
tlabel("CV AMT", COL[1], RB)
tlabel("FM AMT", COL[2], RB)
# decorative: the two oscillators and the arrow between them
add(text("MOD", COL[1], RC - 1.0, 7.0, FONT_BOLD, MODC))
add(text("into the principal  →", COL[1], RC + 4.6, 2.3, FONT_MED, SOFT))
jlabel("V/OCT", COL[0], JD); jlabel("CV", COL[1], JD); jlabel("FM", COL[2], JD)
ox = MOD_X0 + 1.2
add(f'<rect x="{ox}" y="{JE - 9.6}" width="{MOD_X1 - 1.2 - ox}" height="14.8" rx="1.3" fill="{MODC}" fill-opacity="0.16"/>')
jlabel("TRI", COL[0], JE, INK); jlabel("WAVE", COL[1], JE, INK); jlabel("CV", COL[2], JE, INK)
add(text("TRI, WAVE: ±5 V · CV: 0 to 10 V", COL[1], JF - 1.0, 2.3, FONT_MED, SOFT))
add(text("LO range: 0.1 to 27 Hz", COL[1], JF + 2.8, 2.3, FONT_MED, SOFT))

# ---- Centre: modulation routes
klabel("MOD INDEX", CTR_X, RA, big=True, col=CTRC)
add(text("−", CTR_X - 8.4, RA + 8.0, 2.6, FONT_MED, SOFT))
add(text("+", CTR_X + 8.4, RA + 8.0, 2.6, FONT_MED, SOFT))
swlabel("AMPL", COL[3], RB, "ON", "OFF")
swlabel("PITCH", COL[4], RB, "ON", "OFF")
swlabel("TIMBRE", COL[3], RC, "ON", "OFF")
swlabel("LOCK", COL[4], RC, "ON", "OFF")
jlabel("INDEX", COL[3], JD); jlabel("LOCK IN", COL[4], JD)
add(text("CV AMT", COL[3], JE - 6.6, CONTROL))
add(text("STRENGTH", COL[4], JE - 6.6, CONTROL))
gx0 = CTR_X0 + 1.2
add(f'<rect x="{gx0}" y="{JF - 9.6}" width="{CTR_X1 - 1.2 - gx0}" height="14.8" rx="1.3" fill="{CTRC}" fill-opacity="0.14"/>')
jlabel("MOD", COL[3], JF, INK); jlabel("PRIN", COL[4], JF, INK)
add(text("GATES", CTR_X, JF + 1.0, 2.6, FONT_BOLD, CTRC, spacing=0.08))

# ---- Principal oscillator
klabel("FREQ", COL[5], RA, big=True)
klabel("FINE", COL[6], RA)
add(text("A0 – A8", COL[7], RA - 1.2, 2.5, FONT_MED, SOFT))
add(text("± a fifth", COL[7], RA + 2.6, 2.5, FONT_MED, SOFT))
tlabel("CV AMT", COL[5], RB)
tlabel("FM AMT", COL[6], RB)
tlabel("TIMB CV", COL[7], RB, col=TIMC)

# ---- Timbre
klabel("TIMBRE", COL[5], RC, big=True, col=TIMC)
klabel("SYMMETRY", COL[6], RC, big=True, col=TIMC)
klabel("ORDER", COL[7], RC, big=True, col=TIMC)
jlabel("V/OCT", COL[5], JD); jlabel("CV", COL[6], JD); jlabel("FM", COL[7], JD)
jlabel("TIMBRE", COL[5], JE); jlabel("SYMM", COL[6], JE); jlabel("ORDER", COL[7], JE)
px0 = PRN_X0 + 1.2
add(f'<rect x="{px0}" y="{JF - 9.6}" width="{PRN_X1 - 1.2 - px0}" height="14.8" rx="1.3" fill="{PRNC}" fill-opacity="0.18"/>')
jlabel("SINE", COL[5], JF, INK); jlabel("SQUARE", COL[6], JF, INK); jlabel("FINAL", COL[7], JF, PRNC)

svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}mm" height="{H}mm" viewBox="0 0 {W} {H}">\n'
       + "\n".join(p for p in parts if p) + "\n</svg>\n")
open(os.path.join(HERE, "Buchla259t.svg"), "w").write(svg)
print("ok")

# --- Haer maker logo (res/haer_logo.py) on the bottom rail
import sys as _sys
_sys.path.insert(0, HERE)
import haer_logo
haer_logo.add_to_file(os.path.join(HERE, "Buchla259t.svg"), x=W / 2, y=126.0, cap=3.0, bg=BG)
