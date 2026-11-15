// Checks the Buchla 259t engine and renders a demo WAV.
//   make test            (checks only)
//   test/render demo.wav (checks + demo)
#include "../src/ComplexDSP.hpp"
#include <cstdio>
#include <string>
#include <vector>
#include <complex>
#include <cstdlib>

using namespace c259;
static const int SR = 48000;

static void writeWav(const std::string& path, const std::vector<float>& l, const std::vector<float>& r) {
	FILE* f = fopen(path.c_str(), "wb");
	if (!f) { printf("cannot write %s\n", path.c_str()); return; }
	int n = (int) l.size(), bytes = n * 4;
	auto u32 = [&](uint32_t v) { fwrite(&v, 4, 1, f); };
	auto u16 = [&](uint16_t v) { fwrite(&v, 2, 1, f); };
	fwrite("RIFF", 1, 4, f); u32(36 + bytes); fwrite("WAVEfmt ", 1, 8, f);
	u32(16); u16(1); u16(2); u32(SR); u32(SR * 4); u16(4); u16(16);
	fwrite("data", 1, 4, f); u32(bytes);
	for (int i = 0; i < n; i++) {
		int16_t a = (int16_t) (clampf(l[i], -1.f, 1.f) * 32767), b = (int16_t) (clampf(r[i], -1.f, 1.f) * 32767);
		fwrite(&a, 2, 1, f); fwrite(&b, 2, 1, f);
	}
	fclose(f);
}

static int fails = 0;
static void check(bool ok, const char* what) {
	printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
	if (!ok) fails++;
}

// level of frequency hz in a block (single-bin DFT, Hann window)
static float binLevel(const std::vector<float>& x, float hz) {
	std::complex<double> acc = 0;
	int n = (int) x.size();
	for (int i = 0; i < n; i++) {
		double w = 0.5 - 0.5 * std::cos(2 * M_PI * i / (n - 1));
		acc += x[i] * w * std::polar(1.0, -2 * M_PI * hz * i / SR);
	}
	return (float) (std::abs(acc) / (n * 0.25));
}

static std::vector<float> run(Controls c, float secs, float Outputs::*which) {
	Voice v; v.setSampleRate(SR);
	Outputs o;
	int n = (int) (secs * SR);
	std::vector<float> y(n);
	for (int i = 0; i < n; i++) { v.process(c, o); y[i] = o.*which; }
	return y;
}

static void checks() {
	Controls c;
	c.prinOct = std::log2(440.f / C4_HZ);       // A4
	auto sine = run(c, 0.5f, &Outputs::sine);
	std::vector<float> tail(sine.begin() + SR / 4, sine.end());
	float a440 = binLevel(tail, 440.f), a880 = binLevel(tail, 880.f);
	check(std::fabs(a440 - 5.f) < 0.3f, "SINE is 5 V at 440 Hz");
	check(a880 < 0.01f, "SINE has no 2nd harmonic");

	// FINAL with TIMBRE 0, ORDER left = clean sine
	auto fin = run(c, 0.5f, &Outputs::final);
	std::vector<float> ft(fin.begin() + SR / 4, fin.end());
	check(binLevel(ft, 1320.f) < 0.05f, "FINAL is a clean sine at TIMBRE 0");

	// folding adds odd harmonics; symmetry adds even ones
	c.timbre = 0.6f;
	fin = run(c, 0.5f, &Outputs::final);
	ft.assign(fin.begin() + SR / 4, fin.end());
	check(binLevel(ft, 1320.f) > 0.3f, "TIMBRE folds in odd harmonics");
	check(binLevel(ft, 880.f) < 0.05f, "symmetric folding: no even harmonics");
	c.symmetry = 0.5f;
	fin = run(c, 0.5f, &Outputs::final);
	ft.assign(fin.begin() + SR / 4, fin.end());
	check(binLevel(ft, 880.f) > 0.2f, "SYMMETRY brings in even harmonics");

	// aliasing: heavy folding of a high note should leave little energy
	// at non-harmonic frequencies
	Controls h; h.prinOct = std::log2(2111.f / C4_HZ); h.timbre = 1.f;
	fin = run(h, 0.5f, &Outputs::final);
	ft.assign(fin.begin() + SR / 4, fin.end());
	float worst = 0;
	for (float hz = 300; hz < 19000; hz += 137) {
		float d = std::fmod(hz, 2111.f);
		if (d < 150 || d > 1961) continue;
		worst = std::max(worst, binLevel(ft, hz));
	}
	printf("      worst alias at 2.1 kHz full fold: %.3f V\n", worst);
	check(worst < 0.15f, "folding aliasing stays low (< -30 dB)");

	// levels and NaNs under extreme modulation
	Controls x; x.prinOct = 3.f; x.modOct = 4.f; x.index = 1.f; x.pitchOn = x.ampOn = x.timbreOn = true;
	x.timbre = 1.f; x.symmetry = -1.f; x.order = 0.5f; x.lockOn = true; x.lockAmt = 1.f;
	fin = run(x, 1.f, &Outputs::final);
	float peak = 0; bool nan = false;
	for (float s : fin) { peak = std::max(peak, std::fabs(s)); if (!std::isfinite(s)) nan = true; }
	check(!nan && peak < 10.f, "no NaN, FINAL stays under 10 V at full modulation");

	// phase lock: principal a bit sharp of 2x mod freq should pull onto it
	Controls l; l.modOct = -1.f; l.prinOct = std::log2(2.f * 1.03f); l.prinOct += -1.f;
	l.lockOn = getenv("NOLOCK") == nullptr; l.lockAmt = 1.f; l.modWave = 2;
	auto sq = run(l, 1.f, &Outputs::sine);
	std::vector<float> lt(sq.begin() + SR / 2, sq.end());
	float target = 2.f * C4_HZ * 0.5f;
	check(binLevel(lt, target) > 3.f, "PHASE LOCK pulls the principal onto 2x the mod osc");
}

// ---- demo: a short phrase showing each part of the module
struct Env { float v = 0; void step(bool g, float a, float d) { v += g ? (1 - v) * a : -v * d; } };

int main(int argc, char** argv) {
	checks();
	printf(fails ? "\n%d check(s) failed\n" : "\nall checks passed\n", fails);
	if (argc < 2) return fails ? 1 : 0;

	const float bpm = 128.f, step = 60.f / bpm / 4.f;
	const int stepsPerPart = 64;
	const int parts = 5;
	int n = (int) (parts * stepsPerPart * step * SR) + SR * 2;
	std::vector<float> L(n), R(n);
	Voice v; v.setSampleRate(SR);
	Voice v2; v2.setSampleRate(SR);
	Outputs o, o2;
	Env env;
	// F minor bass/lead pattern (semitones from F1/F2), -99 = rest
	const int pat[16] = {0, -99, 0, 12, -99, 3, 0, -99, 7, -99, 0, 10, -99, 3, 15, 12};
	for (int i = 0; i < n; i++) {
		float t = (float) i / SR;
		int s = (int) (t / step), part = std::min(s / stepsPerPart, parts - 1);
		float inStep = t - s * step;
		float partPos = (float) (s % stepsPerPart) / stepsPerPart + inStep / (step * stepsPerPart);
		int note = pat[s % 16];
		bool gate = note != -99 && inStep < step * 0.55f && s < parts * stepsPerPart;
		if (note == -99) note = 0;
		Controls c;
		float base = std::log2(43.65f / C4_HZ) + note / 12.f;   // F1
		c.prinOct = base;
		float decay = 0.0009f, mix = 1.f, pan = 0.f;
		switch (part) {
		case 0:   // plain folding: TIMBRE swept by a slow LFO
			c.timbre = 0.5f - 0.5f * std::cos(2 * M_PI * partPos * 2);
			c.symmetry = 0.f;
			break;
		case 1:   // SYMMETRY and ORDER moving
			c.prinOct = base + 1.f;
			c.timbre = 0.55f;
			c.symmetry = std::sin(2 * M_PI * partPos * 3);
			c.order = 0.5f - 0.5f * std::cos(2 * M_PI * partPos);
			decay = 0.0004f;
			break;
		case 2:   // FM bells: mod osc at 3.5x, PITCH route on, index swept
			c.prinOct = base + 2.f;
			c.modOct = c.prinOct + std::log2(3.5f);
			c.pitchOn = true; c.index = 0.15f + 0.6f * partPos;
			c.timbre = 0.15f;
			decay = 0.00025f; pan = 0.4f;
			break;
		case 3:   // AM + TIMBRE mod from an audio-rate mod osc a fifth up
			c.prinOct = base + 1.f;
			c.modOct = c.prinOct + std::log2(1.5f) + 0.01f;
			c.ampOn = c.timbreOn = true; c.index = 0.8f;
			c.timbre = 0.3f + 0.3f * partPos; c.modWave = 2;
			decay = 0.0005f;
			break;
		default:  // PHASE LOCK: principal detuned, locked to the mod osc
			c.prinOct = base + 1.f + 0.03f * std::sin(2 * M_PI * partPos * 2);
			c.modOct = base + 1.f; c.modWave = 2;
			c.lockOn = partPos > 0.25f; c.lockAmt = 0.8f;
			c.timbre = 0.45f; c.symmetry = 0.3f;
			decay = 0.0004f;
			break;
		}
		env.step(gate, 0.02f, decay);
		v.process(c, o);
		// a second, quiet voice an octave up for width in the FM part
		Controls c2 = c; c2.prinOct += 0.004f; c2.modOct += 0.004f;
		v2.process(c2, o2);
		float a = o.final * env.v * 0.16f * mix, b = o2.final * env.v * 0.16f * mix;
		L[i] = a * (1 - pan * 0.5f) + b * pan * 0.5f;
		R[i] = a * (1 - pan * 0.5f) * (pan > 0 ? 0.7f : 1.f) + b * pan * 0.8f;
	}
	// normalise to -1 dBFS
	float pk = 1e-6f;
	for (int i = 0; i < n; i++) pk = std::max(pk, std::max(std::fabs(L[i]), std::fabs(R[i])));
	float g = 0.89f / pk;
	for (int i = 0; i < n; i++) { L[i] *= g; R[i] *= g; }
	writeWav(argv[1], L, R);
	printf("wrote %s (%.1f s)\n", argv[1], (float) n / SR);
	return fails ? 1 : 0;
}
