#include "plugin.hpp"
#include "widgets.hpp"

using namespace aetrion;

#define VALUT_SIZE 16
#define CHANNEL_COUNT 8

struct ChordVault_P1 : Module {
	enum ParamId {
		STEP_LENGTH_BTN_PARAM,
		STEP_SELECT_KNOB_PARAM,
		COPY_PASTE_BTN_PARAM,
		RECORD_PLAY_BTN_PARAM,
		LENGTH_KNOB_PARAM,
		RESET_BTN_PARAM,
		POLY_MODE_BTN_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		CV_PLAY_MODE_INPUT,
		RESET_INPUT,
		CLOCK_INPUT,
		GATE_IN_INPUT,
		CV_IN_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GATE_OUT_OUTPUT,
		CV_OUT_OUTPUT,
		BASE_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		STEP_MODE_LIGHT,
		LENGTH_MODE_LIGHT,
		COPY_PASTE_LIGHT_LIGHT,
		RECORD_LIGHT_LIGHT,
		PLAY_LIGHT_LIGHT,
		POLY_ALL_LIGHT_LIGHT,
		POLY_NO_BASE_LIGHT,
		LIGHTS_LEN
	};

	//Not Persisted
	int seqLength;
	bool firstProcess;
	bool clockHigh;
	bool recordPlayBtnDown;
	bool polyModeBtnDown;
	int stepSelect_prev;
	int stepSelect_previewGateTimer;

	//Persisted

	float vault_cv [VALUT_SIZE][CHANNEL_COUNT];
	bool vault_gate [VALUT_SIZE][CHANNEL_COUNT];
	int vault_pos;
	bool recording;
	bool excludeBase;


	ChordVault_P1() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configButton(STEP_LENGTH_BTN_PARAM, "");
		configParam(STEP_SELECT_KNOB_PARAM, 1.f, 16.f, 4.f, "Step Select");
		configButton(COPY_PASTE_BTN_PARAM, "");
		configButton(RECORD_PLAY_BTN_PARAM, "Record/Play Toggle");
		configParam(LENGTH_KNOB_PARAM, 1.f, 16.f, 4.f, "Step Length");
		configButton(RESET_BTN_PARAM, "");
		configButton(POLY_MODE_BTN_PARAM, "Poly Incldues Base?");

		configInput(CV_PLAY_MODE_INPUT, "");
		configInput(RESET_INPUT, "");
		configInput(CLOCK_INPUT, "");
		configInput(GATE_IN_INPUT, "");
		configInput(CV_IN_INPUT, "");
		configOutput(GATE_OUT_OUTPUT, "");
		configOutput(CV_OUT_OUTPUT, "");
		configOutput(BASE_OUT_OUTPUT, "");

		initalize();
	}

	void onAdd(const AddEvent& e) override {
		Module::onAdd(e);
		initalize();
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		initalize();
	}

	void onRandomize (const RandomizeEvent& e) override {
		Module::onRandomize(e);
	}

	void initalize(){
		
		seqLength = 4;
		firstProcess = true;		
		clockHigh = false;
		recordPlayBtnDown = false;
		polyModeBtnDown = false;
		stepSelect_prev = 0;
		stepSelect_previewGateTimer = 0;

		memset(vault_cv, 0, sizeof vault_cv);
		memset(vault_gate, 0, sizeof vault_gate);
		vault_pos = 0;
		recording = true;
		excludeBase = false;		

	}

	// json_t *dataToJson() override{
	// }

	// void dataFromJson(json_t *jobj) override {					
	// }

	void process(const ProcessArgs& args) override {

		if(firstProcess){
			firstProcess = false;			

			lights[RECORD_LIGHT_LIGHT].setBrightness(recording ? 1.f : 0.f);
			lights[PLAY_LIGHT_LIGHT].setBrightness(recording ? 0.f : 1.f);
			lights[POLY_NO_BASE_LIGHT].setBrightness(excludeBase ? 1.f : 0.f);
			lights[POLY_ALL_LIGHT_LIGHT].setBrightness(excludeBase ? 0.f : 1.f);

			outputs[CV_OUT_OUTPUT].setChannels(CHANNEL_COUNT);
			outputs[GATE_OUT_OUTPUT].setChannels(CHANNEL_COUNT);

			stepSelect_prev = (int)params[STEP_SELECT_KNOB_PARAM].getValue();
		}

		//Toggle Recrod mode if Button is down
		{
			float btnValue = params[RECORD_PLAY_BTN_PARAM].getValue();
			if(recordPlayBtnDown && btnValue <= 0){
				recordPlayBtnDown = false;
			}else if(!recordPlayBtnDown && btnValue > 0){
				recordPlayBtnDown = true;
				recording = !recording;
				lights[RECORD_LIGHT_LIGHT].setBrightness(recording ? 1.f : 0.f);
				lights[PLAY_LIGHT_LIGHT].setBrightness(recording ? 0.f : 1.f);
				if(recording){
					//When entering record mode reset play position
					vault_pos = 0;
				}else{
					//If done recording sort the current CVs
					sortCurrentCVs();
				}
			}
		}

		//Toggle Poly Out mode if Button is down
		{
			float btnValue = params[POLY_MODE_BTN_PARAM].getValue();
			if(polyModeBtnDown && btnValue <= 0){
				polyModeBtnDown = false;
			}else if(!polyModeBtnDown && btnValue > 0){
				polyModeBtnDown = true;
				excludeBase = !excludeBase;
				lights[POLY_NO_BASE_LIGHT].setBrightness(excludeBase ? 1.f : 0.f);
				lights[POLY_ALL_LIGHT_LIGHT].setBrightness(excludeBase ? 0.f : 1.f);
			}
		}

		seqLength = (int)params[LENGTH_KNOB_PARAM].getValue();

		int stepSelect = (int)params[STEP_SELECT_KNOB_PARAM].getValue();
		//Wrap back around to correct for 0 indexing vs 1 indexing
		//Do this before the comparison so turnning the knob above the seqLimit doesn't keep triggering things
		if(stepSelect >= seqLength) stepSelect = 0; 
		
		if(stepSelect_prev != stepSelect){
			stepSelect_prev = stepSelect;
			vault_pos = stepSelect;
			
			//When cycling through the steps trigger a fake/preview gate for a quarter second
			stepSelect_previewGateTimer = args.sampleRate / 4;
		}		

		//Clock Detection
		{
			float usedClock = 0;
			if(recording){
				//When recording, use the max of all input gate values
				//This means any gate down will cause a record and all gates must go low for the next record to happen
				for(int ci = 0; ci < CHANNEL_COUNT; ci++){
					usedClock = std::max(usedClock,inputs[GATE_IN_INPUT].getVoltage(ci));
				}
			}else{
				usedClock = inputs[CLOCK_INPUT].getVoltage();
			}

			if(clockHigh && usedClock <= 0.1f){
				clockHigh = false;
			}else if(!clockHigh && usedClock >= 2.0f){
				clockHigh = true;

				//Sort previous CVs lowest to highest
				//We have to do extra work here to only sort CVs with high gates
				if(recording){
					sortCurrentCVs();
				}

				vault_pos++;
				if(vault_pos >= seqLength) vault_pos = 0;
				
				//Clear next gates if recording started
				if(recording){
					for(int ci = 0; ci < CHANNEL_COUNT; ci++){
						vault_gate[vault_pos][ci] = false;
					}
				}
			}
		}

		bool outGateHigh = clockHigh;

		if(stepSelect_previewGateTimer > 0){
			stepSelect_previewGateTimer --;
			outGateHigh = true;
		}

		//Input/Output
		{
			for(int ci = 0; ci < CHANNEL_COUNT; ci++){
				if(recording && inputs[GATE_IN_INPUT].getVoltage(ci) >= 2.0f){
					vault_cv[vault_pos][ci] = inputs[CV_IN_INPUT].getVoltage(ci);
					vault_gate[vault_pos][ci] = true;
				}

				int channel = ci;
				if(excludeBase){
					if(channel == 0) continue;
					channel--;
				}

				outputs[CV_OUT_OUTPUT].setVoltage(vault_cv[vault_pos][ci],channel);
				outputs[GATE_OUT_OUTPUT].setVoltage((outGateHigh && vault_gate[vault_pos][ci]) ? 10.f : 0.f,channel);
			}
			//Since CVs are sorted we know the first one is the lowest one
			outputs[BASE_OUT_OUTPUT].setVoltage(vault_cv[vault_pos][0]);

		}
	}

	void sortCurrentCVs(){
		float activeCVs [CHANNEL_COUNT];
		int activeCV_count = 0;
		auto cvs = vault_cv[vault_pos];
		auto gates = vault_gate[vault_pos];
		for(int ci = 0; ci < CHANNEL_COUNT; ci++){
			if(gates[ci]){
				activeCVs[activeCV_count] = cvs[ci];
				activeCV_count++;
			}
		}
		std::sort(activeCVs, activeCVs + activeCV_count, std::greater<float>());
		activeCV_count = 0;
		for(int ci = 0; ci < CHANNEL_COUNT; ci++){
			if(gates[ci]){
				cvs[ci] = activeCVs[activeCV_count];
				activeCV_count++;
			}
		}
	}
};

struct CurStepDisplay : DigitalDisplay {
	CurStepDisplay() {
		fontPath = asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf");
		bgText = "18";
		fontSize = 8;
	}
	ChordVault_P1* module;
	int steps_prev = -1;
	void step() override {
		if (module) {
			//Note here we are not adding 1, so we are effectively showing the seq number from the last step, which is the step current playing
			int steps = module->vault_pos;
			if(steps == 0) steps = module->seqLength;
			if (steps_prev != steps){
				steps_prev = steps;
				text = string::f("%d", steps);	
			}
		}else{
			text = string::f("4");	
		}		
	}
};


struct ChordVault_P1Widget : ModuleWidget {
	ChordVault_P1Widget(ChordVault_P1* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ChordVault_P1.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<SmallButton>(mm2px(Vec(17.649, 16.613)), module, ChordVault_P1::STEP_LENGTH_BTN_PARAM));
		addParam(createParamCentered<RotarySwitch<LargeKnob>>(mm2px(Vec(18.017, 31.152)), module, ChordVault_P1::STEP_SELECT_KNOB_PARAM));
		addParam(createParamCentered<SmallButton>(mm2px(Vec(29.586, 30.925)), module, ChordVault_P1::COPY_PASTE_BTN_PARAM));
		addParam(createParamCentered<SmallButton>(mm2px(Vec(4.488, 46.388)), module, ChordVault_P1::RECORD_PLAY_BTN_PARAM));
		addParam(createParamCentered<RotarySwitch<LargeKnob>>(mm2px(Vec(17.939, 63.679)), module, ChordVault_P1::LENGTH_KNOB_PARAM));
		addParam(createParamCentered<SmallButton>(mm2px(Vec(5.475, 75.012)), module, ChordVault_P1::RESET_BTN_PARAM));
		addParam(createParamCentered<SmallButton>(mm2px(Vec(4.488, 115.48)), module, ChordVault_P1::POLY_MODE_BTN_PARAM));

		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(30.069, 63.579)), module, ChordVault_P1::CV_PLAY_MODE_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(5.557, 82.497)), module, ChordVault_P1::RESET_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(5.557, 98.289)), module, ChordVault_P1::CLOCK_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(17.895, 82.497)), module, ChordVault_P1::GATE_IN_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(17.895, 98.289)), module, ChordVault_P1::CV_IN_INPUT));

		addOutput(createOutputCentered<aetrion::Port>(mm2px(Vec(30.026, 82.552)), module, ChordVault_P1::GATE_OUT_OUTPUT));
		addOutput(createOutputCentered<aetrion::Port>(mm2px(Vec(30.026, 98.235)), module, ChordVault_P1::CV_OUT_OUTPUT));
		addOutput(createOutputCentered<aetrion::Port>(mm2px(Vec(30.019, 112.96)), module, ChordVault_P1::BASE_OUT_OUTPUT));

		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(10.904, 16.613)), module, ChordVault_P1::STEP_MODE_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(24.393, 16.613)), module, ChordVault_P1::LENGTH_MODE_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(32.958, 31.089)), module, ChordVault_P1::COPY_PASTE_LIGHT_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(7.778, 45.073)), module, ChordVault_P1::RECORD_LIGHT_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(7.778, 47.539)), module, ChordVault_P1::PLAY_LIGHT_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(7.778, 114.163)), module, ChordVault_P1::POLY_ALL_LIGHT_LIGHT));
		addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(7.778, 116.632)), module, ChordVault_P1::POLY_NO_BASE_LIGHT));

		CurStepDisplay* display = createWidget<CurStepDisplay>(mm2px(Vec(5.126, 29.633)));
		display->box.size = mm2px(Vec(5.225, 2.976));
		display->textPos = mm2px(Vec(5.225+2, 2.976+3).div(2.f)); 
		display->module = module;
		addChild(display);
	}
};


Model* modelChordVault_P1 = createModel<ChordVault_P1, ChordVault_P1Widget>("ChordVault_P1");