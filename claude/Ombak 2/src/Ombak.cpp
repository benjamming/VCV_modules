#include "plugin.hpp"

// -----------------------------------------------------------------------
// Ombak
//
// Takes a 1V/octave pitch signal and produces two outputs:
//   - THRU:     the input pitch, unchanged.
//   - SHIMMER:  the input pitch shifted by a fixed frequency offset
//               (delta_frequency), set by the DELTA knob.
//
// Derivation (from the module author):
//   input frequency:   f_in  = F0 * 2^v            (v = input volts)
//   desired output:    f_out = f_in + delta_frequency = F0 * 2^w
//   =>  w = log2( 2^v + delta_frequency / F0 )
//
// F0 is the reference ("base") frequency at 0V. It's fixed to C4
// (261.6256 Hz, Rack's dsp::FREQ_C4) for now, matching Rack's own
// 1V/octave convention. A future version could expose F0 as a knob
// or CV input; the math above already generalizes to that case.
// -----------------------------------------------------------------------

struct Ombak : Module {
	enum ParamId {
		DELTA_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		THRU_OUTPUT,
		SHIMMER_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	// F0: reference frequency at 0V. Matches Rack's 1V/oct convention
	// (0V = C4). Change this (or turn it into a param/input) if you
	// later want a user-adjustable base frequency.
	// Declared here, defined (with its actual value) just below the
	// struct. A float static member can't take dsp::FREQ_C4 as an
	// in-class initializer unless it's constexpr -- and dsp::FREQ_C4
	// itself is only declared `const` in Rack's headers, not
	// `constexpr` -- so the value has to be assigned out-of-class
	// instead. See Ombak.cpp, just after this struct's closing brace.
	static const float BASE_FREQ;
	// old code: static const float BASE_FREQ = dsp::FREQ_C4; // 261.6256 Hz

	// DELTA knob range, in Hz.
	// "Ombak" beating rates for a shimmering, single-tone effect are
	// typically ~5-12 Hz. Above roughly 15-20 Hz most listeners stop
	// hearing a shimmering single tone and start hearing two distinct
	// pitches (a dyad/chord). +/-15 Hz covers the classic shimmer range
	// with headroom on both sides, right up to where it starts to
	// split into two tones. Adjust these three constants any time to
	// change the knob's range or default.
	static constexpr float DELTA_MIN = -25.f;
	static constexpr float DELTA_MAX = 25.f;
	static constexpr float DELTA_DEFAULT = 7.f;

	Ombak() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

		configParam(DELTA_PARAM, DELTA_MIN, DELTA_MAX, DELTA_DEFAULT, "Frequency offset", " Hz");
		configInput(VOCT_INPUT, "1V/octave pitch");
		configOutput(THRU_OUTPUT, "Pitch pass-through");
		configOutput(SHIMMER_OUTPUT, "Shifted pitch (ombak)");

		// THRU is a direct, unmodified copy of the input, so it's a
		// sensible bypass route. SHIMMER creates a different pitch
		// entirely, so it's intentionally left silent when bypassed.
		configBypass(VOCT_INPUT, THRU_OUTPUT);
		configBypass(VOCT_INPUT, SHIMMER_OUTPUT);
	}

	void process(const ProcessArgs& args) override {
		// Support polyphony: process however many channels are present
		// on the input (at least 1, so we still produce output when
		// nothing is patched).
		int channels = std::max(1, inputs[VOCT_INPUT].getChannels());

		float deltaFreq = params[DELTA_PARAM].getValue();

		for (int c = 0; c < channels; c++) {
			float v = inputs[VOCT_INPUT].getPolyVoltage(c);

			// Pass-through: unchanged input pitch.
			outputs[THRU_OUTPUT].setVoltage(v, c);

			// w = log2( 2^v + delta_frequency / F0 )
			float arg = std::pow(2.f, v) + deltaFreq / BASE_FREQ;

			// Guard against log2 of a non-positive number. This can
			// happen if delta_frequency is negative enough to push the
			// resulting frequency to/below 0 Hz. Clamp to a tiny
			// positive epsilon instead of producing -inf or NaN.
			arg = std::max(arg, 1e-6f);
			float w = std::log2(arg);

			outputs[SHIMMER_OUTPUT].setVoltage(w, c);
		}

		outputs[THRU_OUTPUT].setChannels(channels);
		outputs[SHIMMER_OUTPUT].setChannels(channels);
	}
};

// Out-of-class definition of BASE_FREQ (see the declaration inside the
// struct above for why this can't just be an in-class initializer).
const float Ombak::BASE_FREQ = dsp::FREQ_C4;


struct OmbakWidget : ModuleWidget {
	OmbakWidget(Ombak* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Ombak.svg")));

		// Corner screws
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// DELTA knob
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(20.32, 32.0)), module, Ombak::DELTA_PARAM));

		// V/OCT input
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(20.32, 58.0)), module, Ombak::VOCT_INPUT));

		// THRU and SHIMMER outputs
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.32, 88.0)), module, Ombak::THRU_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.32, 110.0)), module, Ombak::SHIMMER_OUTPUT));
	}
};


Model* modelOmbak = createModel<Ombak, OmbakWidget>("Ombak");
