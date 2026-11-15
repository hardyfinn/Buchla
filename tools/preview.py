#!/usr/bin/env python3
# Mock-up picture of the panel with knobs, switches, jacks and lights drawn in
# at their real sizes, so the layout can be checked without Rack.
#   python3 tools/preview.py out.png      (needs Playwright + Chromium)
import os, sys, math, subprocess
HERE = os.path.dirname(os.path.abspath(__file__))
svg = open(os.path.join(HERE, "..", "res", "Buchla259t.svg")).read()
L = {}
src = open(os.path.join(HERE, "..", "res", "make_panel.py")).read()
exec(src.split("# ---- LAYOUT")[1].split("CONTROL =")[0].split("\n", 1)[1], L)
C, RA, RB, RC, JD, JE, JF, CX = L["COL"], L["RA"], L["RB"], L["RC"], L["JD"], L["JE"], L["JF"], L["CTR_X"]
W = 172.72
g = []
def jack(x, y):
    g.append(f'<circle cx="{x}" cy="{y}" r="4.0" fill="#C9CCD1"/><circle cx="{x}" cy="{y}" r="3.0" fill="#7C8088"/><circle cx="{x}" cy="{y}" r="1.8" fill="#0B0B0C"/>')
def knob(x, y, a, r):
    g.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="#26272B" stroke="#44464d" stroke-width="0.5"/>')
    ang = math.radians(-135 + 270 * a)
    g.append(f'<line x1="{x}" y1="{y}" x2="{x + (r-0.8)*math.sin(ang):.2f}" y2="{y - (r-0.8)*math.cos(ang):.2f}" stroke="#EEE" stroke-width="0.7" stroke-linecap="round"/>')
def switch(x, y, h, pos):  # pos 0 = bottom .. 1 = top
    g.append(f'<rect x="{x-2.37}" y="{y-h/2}" width="4.74" height="{h}" rx="0.6" fill="#2a2c31" stroke="#666" stroke-width="0.3"/>')
    hy = y + h/2 - 2.0 - pos * (h - 4.0)
    g.append(f'<rect x="{x-2.0}" y="{hy-1.6}" width="4.0" height="3.2" rx="0.4" fill="#d8d8d8"/>')
def light(x, y, b, r=1.1, col="#5BE37A"):
    g.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="#20301f"/><circle cx="{x}" cy="{y}" r="{r}" fill="{col}" fill-opacity="{b}"/>')

LG, MD, SM, TR = 6.1, 4.8, 3.84, 3.0
knob(C[0], RA, 0.4, LG); knob(C[1], RA, 0.5, MD); switch(C[2], RA, 7.0, 1)
switch(C[0] - 2.0, RB, 9.6, 0); knob(C[1], RB, 0.5, TR); knob(C[2], RB, 0.0, TR)
light(C[1] + 7.0, RA - 6.5, 1)
knob(CX, RA, 0.67, LG)
for x, y in [(C[3], RB), (C[4], RB), (C[3], RC), (C[4], RC)]: switch(x, y, 7.0, 0)
light(C[4] - 4.6, RC, 0, 0.8, "#FFD23C")
knob(C[3], JE, 1.0, TR); knob(C[4], JE, 0.6, SM)
knob(C[5], RA, 0.4, LG); knob(C[6], RA, 0.5, MD); light(C[6] + 7.0, RA - 6.5, 0.4)
knob(C[5], RB, 0.5, TR); knob(C[6], RB, 0.0, TR); knob(C[7], RB, 1.0, TR)
knob(C[5], RC, 0.3, LG); knob(C[6], RC, 0.5, LG); knob(C[7], RC, 0.0, LG)
for x in (C[0], C[1], C[2], C[3], C[4], C[5], C[6], C[7]): jack(x, JD)
for x in (C[0], C[1], C[2], C[5], C[6], C[7]): jack(x, JE)
for x in (C[3], C[4], C[5], C[6], C[7]): jack(x, JF)
for x in (7.6, W - 7.6 - 5.08):
    for y in (2.5, 126.0): g.append(f'<circle cx="{x}" cy="{y}" r="1.6" fill="#8A8D93"/>')
svg = svg.replace("</svg>", "\n".join(g) + "\n</svg>")
out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "preview.png")
tmp = out + ".svg"; open(tmp, "w").write(svg)
pw, ph = int(W * 5), int(128.5 * 5)
open(tmp + ".html", "w").write(f'<html><body style="margin:0;background:#111"><img src="{os.path.basename(tmp)}" style="width:{pw}px;height:{ph}px"></body></html>')
js = f"""const {{chromium}} = require('playwright');
(async () => {{ const b = await chromium.launch(); const p = await b.newPage({{viewport: {{width: {pw}, height: {ph}}}, deviceScaleFactor: 2}});
await p.goto('file://{tmp}.html'); await p.waitForTimeout(300);
await p.screenshot({{path: '{out}'}}); await b.close(); }})();"""
open(out + ".js", "w").write(js)
subprocess.run(["node", out + ".js"], check=True, cwd="/tmp")
os.remove(out + ".js"); os.remove(tmp); os.remove(tmp + ".html")
print("wrote", out)
