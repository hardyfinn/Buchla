#!/usr/bin/env python3
# Builds the Buchla 259t guide PDF:  python3 docs/make_guide.py out.pdf
# (needs Playwright + Chromium; uses panel-preview.png from the repo root)
import os, sys, base64, subprocess
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
out = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "Buchla259t-Guide.pdf")
img = base64.b64encode(open(os.path.join(ROOT, "panel-preview.png"), "rb").read()).decode()

html = f"""<!doctype html><html><head><meta charset="utf-8"><style>
@page {{ size: A4; margin: 15mm 16mm; }}
body {{ font-family: Inter, 'Helvetica Neue', Arial, sans-serif; color: #1d1d1f; font-size: 10.3pt; line-height: 1.45; }}
h1 {{ font-size: 25pt; margin: 0 0 1mm; }}
h1 span {{ color: #E2603F; }}
h2 {{ font-size: 13.5pt; margin: 6mm 0 2mm; border-bottom: 1.5px solid #E9B949; padding-bottom: 1mm; }}
h3 {{ font-size: 11pt; margin: 3.5mm 0 1mm; }}
.lede {{ font-size: 11.3pt; color: #444; margin-top: 0; }}
img.panel {{ width: 100%; border-radius: 2mm; margin: 2mm 0 1mm; }}
table {{ border-collapse: collapse; width: 100%; font-size: 9.5pt; }}
td, th {{ border-bottom: 1px solid #e3e0d8; padding: 1.2mm 2mm; text-align: left; vertical-align: top; }}
th {{ background: #f4f1ea; }}
td:first-child {{ font-weight: 700; white-space: nowrap; }}
.sec {{ display: inline-block; width: 3mm; height: 3mm; border-radius: 0.6mm; margin-right: 1.5mm; vertical-align: -0.3mm; }}
.recipe {{ background: #f7f5ef; border-left: 3px solid #E9B949; padding: 2.3mm 4mm; margin: 2.8mm 0; page-break-inside: avoid; }}
.recipe b.t {{ display: block; font-size: 11pt; margin-bottom: 0.8mm; }}
.recipe .v {{ color: #555; font-size: 9.4pt; margin-top: 1mm; }}
.flow {{ font-weight: 700; background: #1E2230; color: #F2ECDF; padding: 2.3mm 4mm; border-radius: 1.5mm; }}
.small {{ font-size: 9pt; color: #555; }}
.break {{ page-break-before: always; }}
</style></head><body>

<h1>Buchla <span>259t</span></h1>
<p class="lede">A west-coast complex oscillator for VCV Rack, laid out after the Buchla / Tiptop 259t: one oscillator
that plays another. Smooth sines that fold into brass, bells and buzz, all under voltage control.</p>
<img class="panel" src="data:image/png;base64,{img}">

<p class="flow">MODULATION OSC &rarr; (AMPL / PITCH / TIMBRE &times; MOD INDEX) &rarr; PRINCIPAL OSC &rarr; TIMBRE &middot; SYMMETRY &middot; ORDER &rarr; FINAL</p>

<h2>Quick start</h2>
<p>Patch <b>FINAL</b> into a VCA or low-pass gate (the 259t has no envelope of its own, like the real one) and a sequencer's pitch into the
principal's <b>V/OCT</b>. With <b>TIMBRE</b> at zero you hear a plain sine. Turn TIMBRE up: the sine folds over on itself and grows
brighter and more vocal. Then flick <b>PITCH</b> on in the middle section and raise <b>MOD INDEX</b>: the modulation oscillator now
FMs the principal. That is the whole idea; everything else is variations on it.</p>

<h2 class="break">The controls</h2>
<table>
<tr><th>Control</th><th>What it does</th></tr>
<tr><td><span class="sec" style="background:#4FA3D9"></span>FREQ / FINE</td><td>Modulation oscillator pitch, A0 to A8 (FINE &plusmn; a fifth). In <b>LO</b> range it is an LFO, 0.1 to 27 Hz.</td></tr>
<tr><td>WAVE</td><td>Saw, square or triangle. This is the shape that modulates the principal, and what comes out of WAVE and CV.</td></tr>
<tr><td>CV AMT / FM AMT</td><td>Attenuverter for the CV input (1 V/oct at full), and depth for the audio FM input (linear, through zero).</td></tr>
<tr><td>TRI, WAVE, CV, GATE</td><td>Triangle always; the WAVE-switch shape (&plusmn;5 V); the same shape as unipolar 0&ndash;10 V; a gate high for the first half of each cycle.</td></tr>
<tr><td><span class="sec" style="background:#E9B949"></span>MOD INDEX</td><td>How hard the mod osc works on the principal, for all three routes. Bipolar: left of centre flips the modulation.</td></tr>
<tr><td>AMPL</td><td>Amplitude modulation. At full index the principal's level follows the mod wave; with an audio-rate mod osc you get sidebands.</td></tr>
<tr><td>PITCH</td><td>Frequency modulation. Linear and through zero by default, so FM stays in tune across the keyboard (right-click for exponential FM).</td></tr>
<tr><td>TIMBRE</td><td>The mod osc moves the TIMBRE knob, so the folding opens and closes at the mod osc's rate.</td></tr>
<tr><td>LOCK + STRENGTH</td><td>Phase lock: each cycle of the mod osc (or of whatever is in LOCK IN) pulls the principal onto the nearest whole cycle. Detuned pairs snap into a clean harmonic relationship. Not the same as hard sync: it is gentler and keeps the principal's own pitch roughly where it is.</td></tr>
<tr><td>GATES</td><td>The two oscillators as gates. In LO range the MOD gate is a clock; at audio rate it is a raw square to use as a sub or trigger source.</td></tr>
<tr><td><span class="sec" style="background:#E2603F"></span>FREQ / FINE / CV / FM</td><td>Principal oscillator, same ranges and inputs as the mod osc.</td></tr>
<tr><td>SINE, SQUARE, FINAL</td><td>The pure principal sine and square (after FM and lock), and FINAL: the shaped, modulated output.</td></tr>
<tr><td><span class="sec" style="background:#6CC08B"></span>TIMBRE</td><td>How much shaping. TIMB CV sets how much the TIMBRE jack moves it (10 V = full sweep).</td></tr>
<tr><td>SYMMETRY</td><td>With ORDER left: offsets the wave before folding, which brings in even harmonics (hollow becomes reedy). With ORDER right: sweeps the shape from sawtooth to an "M" pseudo-square.</td></tr>
<tr><td>ORDER</td><td>Crossfades the two shapers: left = wavefolder, right = the sine-to-saw/M shaper. Halfway mixes both.</td></tr>
</table>
<p class="small">All inputs are polyphonic (up to 16 voices). SYMMETRY and ORDER CV have no amount knob, as on the 259t: 5 V sweeps SYMMETRY end to end, 10 V sweeps ORDER.</p>

<h2 class="break">Patch ideas</h2>

<div class="recipe"><b class="t">1. Folded techno bass</b>
Principal FREQ around A1, sequencer into V/OCT, FINAL into a VCA with a short envelope. TIMBRE at 30%, SYMMETRY a touch right.
Send the same envelope (or Marbles' X) into TIMBRE with TIMB CV at about 50%: each note opens bright and closes back to a sine, like a
filter but rounder and more nasal.
<div class="v">Try: ORDER halfway for a squarer, Detroit-reese bite. Or put an accent CV into SYMMETRY so accented steps go reedy.</div></div>

<div class="recipe"><b class="t">2. Harmonic FM bells (tuned)</b>
Patch the same V/OCT into both oscillators. Tune the mod osc to a fixed interval above the principal (hover the knobs to read the Hz):
whole-number ratios (2&times;, 3&times;) give organs and brass, 3.5&times; or 1.41&times; gives bells and gongs. PITCH on, WAVE triangle,
MOD INDEX 20&ndash;50%. Envelope into INDEX so the attack is metallic and the tail is pure.
<div class="v">Try: TIMBRE route on as well, at low index, for bells with a shimmering fold.</div></div>

<div class="recipe"><b class="t">3. Autechre-style self-mutating texture</b>
Mod osc in LO range, WAVE square, around 2&ndash;5 Hz. Turn on TIMBRE and AMPL. Patch the MOD GATE into Marbles' clock and a Marbles
output into the mod osc CV: the rhythm and the depth of the folding now drift together. Put a slow random (Random8, or Marbles Y)
into ORDER and SYMMETRY. Nothing repeats exactly, but it stays tied to one pulse.
<div class="v">Try: patch the principal's SQUARE into the mod osc FM jack for feedback-like chaos at FM AMT 20%+.</div></div>

<div class="recipe"><b class="t">4. Phase-locked drone that stays in tune</b>
Detune the principal a few cents to a few semitones off a harmonic of the mod osc, LOCK on, STRENGTH around 60%. Now slowly sweep
the principal FREQ: instead of beating, it clicks from one harmonic to the next, like a harmonic series being stepped through.
Great under chord pads.
<div class="v">Try: put Just Friends (shape mode, cycling) into LOCK IN so the 259t locks to JF's chord instead of its own mod osc.</div></div>

<div class="recipe"><b class="t">5. Polyphonic west-coast chords</b>
A poly V/OCT from Hapax (via your interface) or a VCV chord module into the principal V/OCT. Each voice gets its own pair of oscillators
and shaper. Keep TIMBRE low (10&ndash;25%) and use a poly envelope into TIMBRE for plucked, glassy chords. Run FINAL through Three Sisters
or a VCV lowpass for warmth.</div>

<div class="recipe"><b class="t">6. Percussion</b>
Principal around A2, PITCH on, mod osc 1.5&ndash;3 octaves up, high index, short decaying envelope into INDEX and a short VCA:
toms and metallic snaps. With AMPL on and an audio-rate mod osc, you get ring-mod style clangs for hats.</div>

<div style="page-break-inside: avoid"><h2>How it works (briefly)</h2>
<p>Both oscillators run four times faster than your sample rate and are filtered back down, so heavy folding and deep FM stay
clean up into the treble. The wavefolder is a gentle triangle fold with its corners rounded off as TIMBRE rises; the
"M" shape is a sine with a third harmonic laid over it, which dips the top of the wave into an M.
The 259t's ART tuning system is not modelled (VCV is always in tune).</p>
<p class="small">Inspired by the Buchla Model 259 and the Buchla / Tiptop Audio 259t. This is an independent implementation with its own
code and panel; Buchla and Tiptop Audio are not involved and the names are used only to say what it is modelled after.</p></div>
</body></html>"""

tmp = out + ".html"
open(tmp, "w").write(html)
js = f"""const {{chromium}} = require('playwright');
(async () => {{ const b = await chromium.launch(); const p = await b.newPage();
await p.goto('file://{tmp}'); await p.waitForTimeout(300);
await p.pdf({{path: '{out}', format: 'A4', printBackground: true, preferCSSPageSize: true}}); await b.close(); }})();"""
open(out + ".js", "w").write(js)
subprocess.run(["node", out + ".js"], check=True, cwd="/tmp")
os.remove(out + ".js"); os.remove(tmp)
print("wrote", out)
