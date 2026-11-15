// Buchla 259t for VCV Rack: the sound engine, with no Rack code in it so it
// can be tested on its own (test/render.cpp).
//
// One Voice = one polyphony channel: a Modulation oscillator, a Principal
// oscillator, the three modulation routes between them (Amplitude, Pitch,
// Timbre), Phase Lock, and the Timbre / Symmetry / Order wave shaper.
// Everything runs at 4x the sample rate and is filtered back down, so the
// wavefolder and through-zero FM stay clean up high.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace c259 {

static const int OS = 4;                 // oversampling factor
static const float C4_HZ = 261.6256f;
static const float TWO_PI = 6.283185307f;

inline float clampf(float x, float lo, float hi) { return std::min(std::max(x, lo), hi); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// sin(2*pi*p) for any p
inline float sin2pi(float p) {
	p -= std::floor(p);
	return std::sin(TWO_PI * p);
}

// PolyBLEP correction for a step at phase 0 (t = phase in [0,1), dt = |phase step|)
inline float polyBlep(float t, float dt) {
	if (dt <= 0.f || dt >= 0.5f) return 0.f;
	if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
	if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
	return 0.f;
}

// Triangle fold: the identity between -1 and +1, then folds back and forth.
inline float triFold(float x) {
	float u = (x + 1.f) * 0.25f;
	u -= std::floor(u);
	return 1.f - 4.f * std::fabs(u - 0.5f);
}

// Biquad (transposed direct form II)
struct Biquad {
	float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
	void lowpass(float fcNorm, float q) {   // fcNorm = fc / fs
		float w = TWO_PI * fcNorm, c = std::cos(w), s = std::sin(w), al = s / (2.f * q);
		float a0 = 1.f + al;
		b0 = (1.f - c) * 0.5f / a0; b1 = (1.f - c) / a0; b2 = b0;
		a1 = -2.f * c / a0; a2 = (1.f - al) / a0;
	}
	float process(float x) {
		float y = b0 * x + z1;
		z1 = b1 * x - a1 * y + z2;
		z2 = b2 * x - a2 * y;
		return y;
	}
	void reset() { z1 = z2 = 0.f; }
};

// 8th-order Butterworth lowpass at 0.43 x the base sample rate, run at OS x.
struct Decimator {
	Biquad s[4];
	Decimator() {
		const float qs[4] = {0.5098f, 0.6013f, 0.9000f, 2.5629f};
		for (int i = 0; i < 4; i++) s[i].lowpass(0.43f / OS, qs[i]);
	}
	float process(float x) {
		for (int i = 0; i < 4; i++) x = s[i].process(x);
		return x;
	}
};

// Everything the panel and CV set, per channel. Pitches are in octaves above
// or below C4 (0 = 261.6 Hz).
struct Controls {
	// Modulation oscillator
	float modOct = -1.f;
	bool modLow = false;       // LO range = LFO (frequency / 256)
	int modWave = 0;           // 0 saw, 1 square, 2 triangle
	float modFmV = 0.f;        // external audio FM into the mod osc, volts (already attenuated)
	// Principal oscillator
	float prinOct = -1.f;
	float prinFmV = 0.f;       // external audio FM into the principal, volts (already attenuated)
	// Modulation routes
	float index = 0.5f;        // MOD INDEX, -1..+1
	bool ampOn = false, pitchOn = false, timbreOn = false;
	bool expFm = false;        // right-click: exponential instead of linear FM
	// Phase Lock
	bool lockOn = false;
	float lockAmt = 0.6f;      // 0..1
	bool lockExt = false;      // a cable in LOCK IN replaces the mod osc as the lock source
	float lockInV = 0.f;
	// Timbre section
	float timbre = 0.f;        // 0..1
	float symmetry = 0.f;      // -1..+1
	float order = 0.f;         // 0 folder .. 1 saw/M shaper
};

struct Outputs {
	float tri = 0, wave = 0, cv = 0, modGate = 0;     // mod osc (volts)
	float sine = 0, square = 0, final = 0, prinGate = 0;
};

struct Voice {
	float sr = 48000.f;
	double pm = 0.0, pp = 0.0;          // phases
	float lockCorr = 0.f;               // phase-lock frequency correction (octaves)
	bool lockHigh = false;
	float dcX = 0.f, dcY = 0.f, dcR = 0.999f;
	Decimator dTri, dWave, dSine, dSquare, dFinal;

	void setSampleRate(float s) {
		sr = s;
		dcR = 1.f - TWO_PI * 4.f / s;   // ~4 Hz DC blocker on FINAL
	}

	void reset() { pm = 0.0; pp = 0.0; lockCorr = 0.f; }

	void process(const Controls& c, Outputs& o) {
		const float fsOs = sr * OS;
		const float fsLimit = 0.45f * fsOs;

		float modHz = C4_HZ * std::exp2(c.modOct);
		if (c.modLow) modHz *= (1.f / 256.f);
		float prinBase = C4_HZ * std::exp2(c.prinOct - lockCorr);

		float sTri = 0, sWave = 0, sSine = 0, sSquare = 0, sFinal = 0;
		float m = 0.f;
		for (int k = 0; k < OS; k++) {
			// ---- Modulation oscillator (through-zero linear FM from its FM input)
			float fm = modHz * (1.f + 0.2f * c.modFmV);
			fm = clampf(fm, -fsLimit, fsLimit);
			float dm = fm / fsOs;
			pm += dm;
			if (pm >= 1.0) pm -= 1.0;
			else if (pm < 0.0) pm += 1.0;
			float tm = (float) pm, adm = std::fabs(dm);
			float tri = 1.f - 4.f * std::fabs(tm - 0.5f);           // starts at -1
			float saw = 2.f * tm - 1.f - polyBlep(tm, adm);
			float t2 = tm + 0.5f; if (t2 >= 1.f) t2 -= 1.f;
			float sq = (tm < 0.5f ? 1.f : -1.f) + polyBlep(tm, adm) - polyBlep(t2, adm);
			m = c.modWave == 0 ? saw : (c.modWave == 1 ? sq : tri);

			// ---- Principal frequency: pitch modulation, external FM, lock correction
			float f = prinBase;
			if (c.pitchOn) {
				if (c.expFm) f *= std::exp2(2.f * c.index * m);
				else f += 4.f * c.index * m * prinBase;               // through zero
			}
			f += 0.4f * c.prinFmV * prinBase;                          // 5 V = 2x deviation
			f = clampf(f, -fsLimit, fsLimit);
			float dp = f / fsOs;
			pp += dp;
			pp -= std::floor(pp);

			// ---- Phase Lock: at each rising edge of the lock source, pull the
			// principal toward the nearest whole cycle, and nudge its tuning.
			if (c.lockOn) {
				float src = c.lockExt ? c.lockInV * 0.2f : m;
				if (!lockHigh && src > 0.1f) {
					lockHigh = true;
					float e = (float) (pp - std::floor(pp + 0.5));   // -0.5..0.5 cycles
					pp -= c.lockAmt * 0.7f * e;
					pp -= std::floor(pp);
					lockCorr = clampf(lockCorr + c.lockAmt * 0.08f * e, -0.6f, 0.6f);
				}
				else if (lockHigh && src < -0.1f) lockHigh = false;
			}

			float tp = (float) pp, adp = std::fabs(dp);
			float s = sin2pi(tp);
			float tp2 = tp + 0.5f; if (tp2 >= 1.f) tp2 -= 1.f;
			float psq = (tp < 0.5f ? 1.f : -1.f) + polyBlep(tp, adp) - polyBlep(tp2, adp);
			float psaw = 2.f * tp - 1.f - polyBlep(tp, adp);

			// ---- Timbre section
			float t = c.timbre;
			if (c.timbreOn) t += 0.5f * c.index * m;
			t = clampf(t, 0.f, 1.f);
			// folder (ORDER left): gain up to 10x, SYMMETRY adds DC before folding
			float x = s * (1.f + 9.f * t) + 1.2f * c.symmetry;
			float fold = triFold(x);
			float round = std::min(t * 4.f, 1.f) * 0.6f;               // soften the corners as it folds
			fold = lerpf(fold, fold * (1.5f - 0.5f * fold * fold), round);
			// shaper (ORDER right): TIMBRE crossfades sine to a shape that
			// SYMMETRY sweeps from sawtooth to the "M" (pseudo-square)
			float mShape = s + 0.5f * sin2pi(3.f * tp);
			float shape = lerpf(psaw, mShape, 0.5f * (c.symmetry + 1.f));
			float shaped = lerpf(s, shape, t);
			float out = lerpf(fold, shaped, clampf(c.order, 0.f, 1.f));

			// ---- Amplitude modulation
			if (c.ampOn) {
				float a = std::fabs(c.index);
				out *= (1.f - 0.5f * a) + 0.5f * c.index * m;
			}

			sTri = dTri.process(tri);
			sWave = dWave.process(m);
			sSine = dSine.process(s);
			sSquare = dSquare.process(psq);
			sFinal = dFinal.process(out);
		}

		// DC blocker on FINAL (SYMMETRY adds DC)
		float y = sFinal - dcX + dcR * dcY;
		dcX = sFinal; dcY = y;
		// lock correction slowly relaxes when not being pulled
		lockCorr *= c.lockOn ? 0.99998f : 0.999f;

		o.tri = 5.f * sTri;
		o.wave = 5.f * sWave;
		o.cv = 5.f * (m + 1.f);                    // unipolar 0..10 V, not filtered (it's a CV)
		o.modGate = pm < 0.5 ? 10.f : 0.f;
		o.sine = 5.f * sSine;
		o.square = 5.f * sSquare;
		o.final = 5.f * y;
		o.prinGate = pp < 0.5 ? 10.f : 0.f;
	}
};

} // namespace c259
