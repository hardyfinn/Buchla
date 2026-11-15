// Buchla 259t for VCV Rack: a complex oscillator laid out after the Buchla /
// Tiptop 259t. Left: Modulation oscillator. Middle: the routes from it into the
// Principal (Amplitude, Pitch, Timbre), the Mod Index, and Phase Lock. Right:
// the Principal oscillator and its Timbre / Symmetry / Order wave shaper.
// The sound engine is in ComplexDSP.hpp; this file is the panel and the wiring.
#include "plugin.hpp"
#include "ComplexDSP.hpp"

// ---- LAYOUT (same numbers in res/make_panel.py) ----
static const float COL[8] = {13.0f, 34.0f, 55.0f, 75.5f, 97.0f, 117.7f, 138.7f, 159.7f};
static const float CTR_X = 86.25f;          // Mod Index knob, between the centre columns
static const float RA = 32.0f, RB = 52.0f, RC = 73.5f;
static const float JD = 88.5f, JE = 102.5f, JF = 116.5f;

struct Buchla259t : Module {
	enum ParamIds {
		MOD_FREQ_PARAM, MOD_FINE_PARAM, MOD_RANGE_PARAM, MOD_WAVE_PARAM, MOD_CV_PARAM, MOD_FM_PARAM,
		INDEX_PARAM, AMP_PARAM, PITCH_PARAM, TIMBRE_ROUTE_PARAM, LOCK_PARAM, INDEX_CV_PARAM, LOCK_AMT_PARAM,
		PRIN_FREQ_PARAM, PRIN_FINE_PARAM, PRIN_CV_PARAM, PRIN_FM_PARAM, TIMBRE_CV_PARAM,
		TIMBRE_PARAM, SYMMETRY_PARAM, ORDER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		MOD_VOCT_INPUT, MOD_CV_INPUT, MOD_FM_INPUT,
		INDEX_INPUT, LOCK_INPUT,
		PRIN_VOCT_INPUT, PRIN_CV_INPUT, PRIN_FM_INPUT,
		TIMBRE_INPUT, SYMMETRY_INPUT, ORDER_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		TRI_OUTPUT, WAVE_OUTPUT, CV_OUTPUT, MOD_GATE_OUTPUT, PRIN_GATE_OUTPUT,
		SINE_OUTPUT, SQUARE_OUTPUT, FINAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		MOD_LIGHT, PRIN_LIGHT, LOCK_LIGHT,
		NUM_LIGHTS
	};

	c259::Voice voices[16];
	c259::Outputs outs[16];
	dsp::ClockDivider lightDivider;
	bool expFm = false;    // right-click: exponential FM on the PITCH route

	// Shows the mod osc frequency in Hz for whichever range is selected
	struct ModFreqQuantity : ParamQuantity {
		float getDisplayValue() override {
			Module* m = module;
			float hz = c259::C4_HZ * std::exp2(getValue());
			if (m && m->params[MOD_RANGE_PARAM].getValue() < 0.5f) hz /= 256.f;
			return hz;
		}
		void setDisplayValue(float v) override {
			Module* m = module;
			if (m && m->params[MOD_RANGE_PARAM].getValue() < 0.5f) v *= 256.f;
			setValue(std::log2(std::max(v, 1e-3f) / c259::C4_HZ));
		}
		std::string getUnit() override { return " Hz"; }
	};

	Buchla259t() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// A0 (27.5 Hz) .. A8 (7040 Hz), in octaves from C4
		const float lo = std::log2(27.5f / c259::C4_HZ), hi = std::log2(7040.f / c259::C4_HZ);
		configParam<ModFreqQuantity>(MOD_FREQ_PARAM, lo, hi, -1.f, "Modulation frequency");
		configParam(MOD_FINE_PARAM, -7.f, 7.f, 0.f, "Modulation fine tune", " semitones");
		configSwitch(MOD_RANGE_PARAM, 0.f, 1.f, 1.f, "Modulation range", {"LO (LFO, 0.1 to 27 Hz)", "HI (audio)"});
		configSwitch(MOD_WAVE_PARAM, 0.f, 2.f, 2.f, "Modulation waveform", {"Triangle", "Square", "Sawtooth"});
		configParam(MOD_CV_PARAM, -1.f, 1.f, 0.f, "Modulation frequency CV amount", "%", 0.f, 100.f);
		configParam(MOD_FM_PARAM, 0.f, 1.f, 0.f, "Modulation audio FM amount", "%", 0.f, 100.f);

		configParam(INDEX_PARAM, -1.f, 1.f, 0.35f, "Mod index (how hard the mod osc works the principal)", "%", 0.f, 100.f);
		configSwitch(AMP_PARAM, 0.f, 1.f, 0.f, "Amplitude modulation", {"Off", "On"});
		configSwitch(PITCH_PARAM, 0.f, 1.f, 0.f, "Pitch modulation (FM)", {"Off", "On"});
		configSwitch(TIMBRE_ROUTE_PARAM, 0.f, 1.f, 0.f, "Timbre modulation", {"Off", "On"});
		configSwitch(LOCK_PARAM, 0.f, 1.f, 0.f, "Phase lock", {"Off", "On"});
		configParam(INDEX_CV_PARAM, -1.f, 1.f, 1.f, "Mod index CV amount", "%", 0.f, 100.f);
		configParam(LOCK_AMT_PARAM, 0.f, 1.f, 0.6f, "Phase lock strength", "%", 0.f, 100.f);

		configParam(PRIN_FREQ_PARAM, lo, hi, -1.f, "Principal frequency", " Hz", 2.f, c259::C4_HZ);
		configParam(PRIN_FINE_PARAM, -7.f, 7.f, 0.f, "Principal fine tune", " semitones");
		configParam(PRIN_CV_PARAM, -1.f, 1.f, 0.f, "Principal frequency CV amount", "%", 0.f, 100.f);
		configParam(PRIN_FM_PARAM, 0.f, 1.f, 0.f, "Principal audio FM amount", "%", 0.f, 100.f);
		configParam(TIMBRE_CV_PARAM, -1.f, 1.f, 1.f, "Timbre CV amount", "%", 0.f, 100.f);
		configParam(TIMBRE_PARAM, 0.f, 1.f, 0.f, "Timbre (how much shaping)", "%", 0.f, 100.f);
		configParam(SYMMETRY_PARAM, -1.f, 1.f, 0.f, "Symmetry (folder: offset; shaper: sawtooth to M)", "%", 0.f, 100.f);
		configParam(ORDER_PARAM, 0.f, 1.f, 0.f, "Order (left: wavefolder, right: saw / M shaper)", "%", 0.f, 100.f);

		configInput(MOD_VOCT_INPUT, "Modulation 1 V/oct");
		configInput(MOD_CV_INPUT, "Modulation frequency CV (through CV AMT, 1 V/oct at full)");
		configInput(MOD_FM_INPUT, "Modulation audio FM (linear, through zero)");
		configInput(INDEX_INPUT, "Mod index CV (5 V = full)");
		configInput(LOCK_INPUT, "Phase lock source (replaces the mod osc)");
		configInput(PRIN_VOCT_INPUT, "Principal 1 V/oct");
		configInput(PRIN_CV_INPUT, "Principal frequency CV (through CV AMT, 1 V/oct at full)");
		configInput(PRIN_FM_INPUT, "Principal audio FM (linear, through zero)");
		configInput(TIMBRE_INPUT, "Timbre CV (through its amount knob, 10 V = full sweep)");
		configInput(SYMMETRY_INPUT, "Symmetry CV (5 V = full sweep)");
		configInput(ORDER_INPUT, "Order CV (10 V = full sweep)");

		configOutput(TRI_OUTPUT, "Modulation triangle");
		configOutput(WAVE_OUTPUT, "Modulation waveform (set by the WAVE switch)");
		configOutput(CV_OUTPUT, "Modulation CV (unipolar 0 to 10 V, follows the WAVE switch)");
		configOutput(MOD_GATE_OUTPUT, "Modulation gate (high for the first half of each cycle)");
		configOutput(PRIN_GATE_OUTPUT, "Principal gate (high for the first half of each cycle)");
		configOutput(SINE_OUTPUT, "Principal sine");
		configOutput(SQUARE_OUTPUT, "Principal square");
		configOutput(FINAL_OUTPUT, "Final (the shaped and modulated principal)");

		configLight(MOD_LIGHT, "Modulation oscillator");
		configLight(PRIN_LIGHT, "Principal oscillator");
		configLight(LOCK_LIGHT, "Phase lock");
		lightDivider.setDivision(64);
	}

	void onSampleRateChange(const SampleRateChangeEvent& e) override {
		for (auto& v : voices) v.setSampleRate(e.sampleRate);
	}
	void onAdd(const AddEvent& e) override {
		for (auto& v : voices) v.setSampleRate(APP->engine->getSampleRate());
	}
	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		expFm = false;
		for (auto& v : voices) v.reset();
	}

	void process(const ProcessArgs& args) override {
		int ch = 1;
		for (int i = 0; i < NUM_INPUTS; i++) ch = std::max(ch, inputs[i].getChannels());

		c259::Controls c;
		const float modKnob = params[MOD_FREQ_PARAM].getValue() + params[MOD_FINE_PARAM].getValue() / 12.f;
		const float prinKnob = params[PRIN_FREQ_PARAM].getValue() + params[PRIN_FINE_PARAM].getValue() / 12.f;
		c.modLow = params[MOD_RANGE_PARAM].getValue() < 0.5f;
		int w = (int) std::round(params[MOD_WAVE_PARAM].getValue());   // 0 tri (down), 1 square, 2 saw (up)
		c.modWave = w == 2 ? 0 : (w == 1 ? 1 : 2);
		c.ampOn = params[AMP_PARAM].getValue() > 0.5f;
		c.pitchOn = params[PITCH_PARAM].getValue() > 0.5f;
		c.timbreOn = params[TIMBRE_ROUTE_PARAM].getValue() > 0.5f;
		c.lockOn = params[LOCK_PARAM].getValue() > 0.5f;
		c.lockExt = inputs[LOCK_INPUT].isConnected();
		c.expFm = expFm;
		const float lockAmt = params[LOCK_AMT_PARAM].getValue();

		for (int i = 0; i < ch; i++) {
			c.modOct = modKnob + inputs[MOD_VOCT_INPUT].getPolyVoltage(i)
			           + params[MOD_CV_PARAM].getValue() * inputs[MOD_CV_INPUT].getPolyVoltage(i);
			c.modFmV = params[MOD_FM_PARAM].getValue() * inputs[MOD_FM_INPUT].getPolyVoltage(i);
			c.prinOct = prinKnob + inputs[PRIN_VOCT_INPUT].getPolyVoltage(i)
			            + params[PRIN_CV_PARAM].getValue() * inputs[PRIN_CV_INPUT].getPolyVoltage(i);
			c.prinFmV = params[PRIN_FM_PARAM].getValue() * inputs[PRIN_FM_INPUT].getPolyVoltage(i);
			c.index = clamp(params[INDEX_PARAM].getValue()
			                + params[INDEX_CV_PARAM].getValue() * inputs[INDEX_INPUT].getPolyVoltage(i) * 0.2f, -1.f, 1.f);
			c.lockAmt = lockAmt;
			c.lockInV = inputs[LOCK_INPUT].getPolyVoltage(i);
			c.timbre = params[TIMBRE_PARAM].getValue()
			           + params[TIMBRE_CV_PARAM].getValue() * inputs[TIMBRE_INPUT].getPolyVoltage(i) * 0.1f;
			c.symmetry = clamp(params[SYMMETRY_PARAM].getValue() + inputs[SYMMETRY_INPUT].getPolyVoltage(i) * 0.4f, -1.f, 1.f);
			c.order = clamp(params[ORDER_PARAM].getValue() + inputs[ORDER_INPUT].getPolyVoltage(i) * 0.1f, 0.f, 1.f);
			voices[i].process(c, outs[i]);
		}

		struct { int id; float c259::Outputs::*f; } map[] = {
			{TRI_OUTPUT, &c259::Outputs::tri}, {WAVE_OUTPUT, &c259::Outputs::wave}, {CV_OUTPUT, &c259::Outputs::cv},
			{MOD_GATE_OUTPUT, &c259::Outputs::modGate}, {PRIN_GATE_OUTPUT, &c259::Outputs::prinGate},
			{SINE_OUTPUT, &c259::Outputs::sine}, {SQUARE_OUTPUT, &c259::Outputs::square}, {FINAL_OUTPUT, &c259::Outputs::final},
		};
		for (auto& m : map) {
			outputs[m.id].setChannels(ch);
			for (int i = 0; i < ch; i++) outputs[m.id].setVoltage(outs[i].*(m.f), i);
		}

		if (lightDivider.process()) {
			float dt = args.sampleTime * lightDivider.getDivision();
			// fast oscillators just glow; slow ones blink
			lights[MOD_LIGHT].setBrightnessSmooth(outs[0].modGate > 0.f ? 1.f : 0.f, dt, 20.f);
			lights[PRIN_LIGHT].setBrightnessSmooth(outs[0].prinGate > 0.f ? 1.f : 0.f, dt, 20.f);
			lights[LOCK_LIGHT].setBrightness(c.lockOn ? 0.4f + 0.6f * lockAmt : 0.f);
		}
	}

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "expFm", json_boolean(expFm));
		return root;
	}
	void dataFromJson(json_t* root) override {
		if (json_t* j = json_object_get(root, "expFm")) expFm = json_boolean_value(j);
	}
};

struct Buchla259tWidget : ModuleWidget {
	Buchla259tWidget(Buchla259t* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Buchla259t.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		using M = Buchla259t;
		auto at = [](float x, float y) { return mm2px(Vec(x, y)); };

		// Modulation oscillator
		addParam(createParamCentered<RoundLargeBlackKnob>(at(COL[0], RA), module, M::MOD_FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(COL[1], RA), module, M::MOD_FINE_PARAM));
		addParam(createParamCentered<CKSS>(at(COL[2], RA), module, M::MOD_RANGE_PARAM));
		addParam(createParamCentered<CKSSThree>(at(COL[0] - 2.0f, RB), module, M::MOD_WAVE_PARAM));
		addParam(createParamCentered<Trimpot>(at(COL[1], RB), module, M::MOD_CV_PARAM));
		addParam(createParamCentered<Trimpot>(at(COL[2], RB), module, M::MOD_FM_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(at(COL[1] + 7.0f, RA - 6.5f), module, M::MOD_LIGHT));

		// Centre: modulation routes, index, phase lock
		addParam(createParamCentered<RoundLargeBlackKnob>(at(CTR_X, RA), module, M::INDEX_PARAM));
		addParam(createParamCentered<CKSS>(at(COL[3], RB), module, M::AMP_PARAM));
		addParam(createParamCentered<CKSS>(at(COL[4], RB), module, M::PITCH_PARAM));
		addParam(createParamCentered<CKSS>(at(COL[3], RC), module, M::TIMBRE_ROUTE_PARAM));
		addParam(createParamCentered<CKSS>(at(COL[4], RC), module, M::LOCK_PARAM));
		addChild(createLightCentered<SmallLight<YellowLight>>(at(COL[4] - 4.6f, RC), module, M::LOCK_LIGHT));
		addParam(createParamCentered<Trimpot>(at(COL[3], JE), module, M::INDEX_CV_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(at(COL[4], JE), module, M::LOCK_AMT_PARAM));

		// Principal oscillator + timbre section
		addParam(createParamCentered<RoundLargeBlackKnob>(at(COL[5], RA), module, M::PRIN_FREQ_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(at(COL[6], RA), module, M::PRIN_FINE_PARAM));
		addChild(createLightCentered<MediumLight<GreenLight>>(at(COL[6] + 7.0f, RA - 6.5f), module, M::PRIN_LIGHT));
		addParam(createParamCentered<Trimpot>(at(COL[5], RB), module, M::PRIN_CV_PARAM));
		addParam(createParamCentered<Trimpot>(at(COL[6], RB), module, M::PRIN_FM_PARAM));
		addParam(createParamCentered<Trimpot>(at(COL[7], RB), module, M::TIMBRE_CV_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(at(COL[5], RC), module, M::TIMBRE_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(at(COL[6], RC), module, M::SYMMETRY_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(at(COL[7], RC), module, M::ORDER_PARAM));

		// Jacks
		auto in = [&](float x, float y, int id) { addInput(createInputCentered<PJ301MPort>(at(x, y), module, id)); };
		auto out = [&](float x, float y, int id) { addOutput(createOutputCentered<PJ301MPort>(at(x, y), module, id)); };
		in(COL[0], JD, M::MOD_VOCT_INPUT); in(COL[1], JD, M::MOD_CV_INPUT); in(COL[2], JD, M::MOD_FM_INPUT);
		out(COL[0], JE, M::TRI_OUTPUT); out(COL[1], JE, M::WAVE_OUTPUT); out(COL[2], JE, M::CV_OUTPUT);
		in(COL[3], JD, M::INDEX_INPUT); in(COL[4], JD, M::LOCK_INPUT);
		out(COL[3], JF, M::MOD_GATE_OUTPUT); out(COL[4], JF, M::PRIN_GATE_OUTPUT);
		in(COL[5], JD, M::PRIN_VOCT_INPUT); in(COL[6], JD, M::PRIN_CV_INPUT); in(COL[7], JD, M::PRIN_FM_INPUT);
		in(COL[5], JE, M::TIMBRE_INPUT); in(COL[6], JE, M::SYMMETRY_INPUT); in(COL[7], JE, M::ORDER_INPUT);
		out(COL[5], JF, M::SINE_OUTPUT); out(COL[6], JF, M::SQUARE_OUTPUT); out(COL[7], JF, M::FINAL_OUTPUT);
	}

	void appendContextMenu(Menu* menu) override {
		Buchla259t* module = dynamic_cast<Buchla259t*>(this->module);
		if (!module) return;
		menu->addChild(new MenuSeparator);
		menu->addChild(createBoolPtrMenuItem("Exponential FM on the PITCH route (default: linear, through zero)", "", &module->expFm));
	}
};

Model* modelBuchla259t = createModel<Buchla259t, Buchla259tWidget>("Buchla259t");
