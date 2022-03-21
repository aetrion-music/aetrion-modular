#include "plugin.hpp"
#include "widgets.hpp"
#include "util.hpp"

using namespace aetrion;

#define VAULT_SIZE 16
#define VAULT_SIZE_MINUS_1 15
#define CHANNEL_COUNT 8
#define PlayMode_MAX 8

#define RndChordOption_MAX 16

enum PlayMode{
	//Normal Modes
	FORWARD,
	BACKWARD,
	RANDOM,
	CV,

	//Secret Modes
	SKIP,
	PING_PONG,
	SHUFFLE,
	GLIDE,
};

static std::string PLAY_MODE_NAMES [PlayMode_MAX] = {
	"Forward",
	"Backward",
	"Random",
	"CV Controled",
	"Skip",
	"Ping Pong",
	"Shuffle",
	"Glide",
};

#define A_NOTE -3/12.f
#define As_NOTE -2/12.f
#define B_NOTE -1/12.f
#define C_NOTE 0
#define Cs_NOTE 1/12.f
#define D_NOTE 2/12.f
#define Ds_NOTE 3/12.f
#define E_NOTE 4/12.f
#define F_NOTE 5/12.f
#define Fs_NOTE 6/12.f
#define G_NOTE 7/12.f
#define Gs_NOTE 8/12.f


float RndChordOption [RndChordOption_MAX][3] = {
	{A_NOTE,Cs_NOTE,E_NOTE}, //A Major
	{A_NOTE,C_NOTE,E_NOTE}, //A Minor
	{C_NOTE,E_NOTE,G_NOTE}, //C Major
	{C_NOTE,Ds_NOTE,G_NOTE}, //C Minor
	{D_NOTE,Fs_NOTE,A_NOTE}, //D Major
	{D_NOTE,F_NOTE,A_NOTE}, //D Minor
	{E_NOTE,Gs_NOTE,B_NOTE}, //E Major
	{E_NOTE,G_NOTE,B_NOTE}, //E Minor
	{F_NOTE,As_NOTE,C_NOTE}, //F Major
	{F_NOTE,Gs_NOTE,C_NOTE}, //F Minor
	{G_NOTE,B_NOTE,D_NOTE}, //G Major
	{G_NOTE,As_NOTE,D_NOTE}, //G Minor
};

struct ChordVault : Module {
	enum ParamId {
		STEP_KNOB_PARAM,
		RECORD_PLAY_BTN_PARAM,
		LENGTH_KNOB_PARAM,
		RESET_BTN_PARAM,
		PLAY_MODE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		STEP_CV_INPUT,
		RESET_INPUT,
		CLOCK_INPUT,
		GATE_IN_INPUT,
		CV_IN_INPUT,
		LENGTH_CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		GATE_OUT_OUTPUT,
		CV_OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		RECORD_LIGHT_LIGHT,
		PLAY_LIGHT_LIGHT,
		ENUMS(PLAY_FORWARD_LIGHT,2),
		ENUMS(PLAY_BACKWARD_LIGHT,2),
		ENUMS(PLAY_RANDOM_LIGHT,2),
		ENUMS(PLAY_CV_LIGHT,2),
		LIGHTS_LEN
	};

	struct SeqModeQuantity : ParamQuantity  {
		std::string getDisplayValueString() override {
			if(!module) return "";
			ChordVault* cvModule = dynamic_cast<ChordVault*>(module);
			return PLAY_MODE_NAMES[cvModule->playMode];
		}
	};

	//Not Persisted

	int seqStart;
	int seqLength;
	bool firstProcess;
	bool clockHigh;
	bool gatesHigh;
	bool recordPlayBtnDown;
	bool playModeBtnDown;
	int playModeBtnDown_counter;
	bool resetBtnDown;
	bool resetTrigHigh;
	bool partialPlayClock; //True for the first (partial) clock after going into play mode
	int stepSelect_prev;
	int stepSelect_previewGateTimer;
	bool pingPongDir;
	int activeChannels;

	//Persisted

	float vault_cv [VAULT_SIZE][CHANNEL_COUNT];
	bool vault_gate [VAULT_SIZE][CHANNEL_COUNT];
	int vault_pos;
	bool recording;
	int channels;
	bool dynamicChannels;
	bool startStepMode;
	bool skipPartialClock;

	int shuffle_index;
	int shuffle_arr [VAULT_SIZE];

	PlayMode playMode;

	ChordVault() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam<UnitPrefixQuanity>(STEP_KNOB_PARAM, 0.f, 15.f, 0.f, "Step Select", "Step ", 0, 1, 1);
		getParamQuantity(STEP_KNOB_PARAM)->randomizeEnabled = false;

		configButton(RECORD_PLAY_BTN_PARAM, "Play/Record");
		configParam(LENGTH_KNOB_PARAM, 1.f, 16.f, 4.f, "Step Length", " steps");
		configButton(RESET_BTN_PARAM, "Reset");
		configButton<SeqModeQuantity>(PLAY_MODE_PARAM, "Sequence Mode");

		configInput(STEP_CV_INPUT, "Step CV");
		configInput(RESET_INPUT, "Reset");
		configInput(CLOCK_INPUT, "Clock");
		configInput(GATE_IN_INPUT, "Gates");
		configInput(CV_IN_INPUT, "V/octs");
		configInput(LENGTH_CV_INPUT, "Length cv");

		configOutput(GATE_OUT_OUTPUT, "Gates");
		configOutput(CV_OUT_OUTPUT, "V/octs");



		initalize();
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		initalize();

		//Clear output volgates on reset
		for(int ci = 0; ci < CHANNEL_COUNT; ci++) outputs[CV_OUT_OUTPUT].setVoltage(0,ci);
	}

	void onRandomize (const RandomizeEvent& e) override {
		Module::onRandomize(e);
		for(int si = 0; si < VAULT_SIZE; si ++){
			int chordIndex = floor(rack::random::uniform() * RndChordOption_MAX);
			for(int ci = 0; ci < 3; ci++){
				vault_gate[si][ci] = true;
				vault_cv[si][ci] = RndChordOption[chordIndex][ci];
			}
		}
	}

	void initalize(){
		
		seqStart = 0;
		seqLength = 4;
		firstProcess = true;		
		clockHigh = false;
		gatesHigh = false;
		recordPlayBtnDown = false;
		playModeBtnDown = false;
		playModeBtnDown_counter = 0;		
		resetBtnDown = false;
		resetTrigHigh = false;
		stepSelect_prev = 0;
		stepSelect_previewGateTimer = 0;
		pingPongDir = false;

		memset(vault_cv, 0, sizeof vault_cv);
		memset(vault_gate, 0, sizeof vault_gate);
		vault_pos = 0;
		recording = true;	
		channels = 5;
		dynamicChannels = false;
		startStepMode = false;
		skipPartialClock = false;
		playMode = (PlayMode)0;	
		shuffle_index = 0;
		for(int i = 0; i < VAULT_SIZE; i++) shuffle_arr[i] = i;


		activeChannels = channels;

	}

	json_t *dataToJson() override{
		json_t *jobj = json_object();
		json_object_set_new(jobj, "vault_pos", json_integer(vault_pos));
		json_object_set_new(jobj, "playMode", json_integer(playMode));
		json_object_set_new(jobj, "channels", json_integer(channels));		
		json_object_set_new(jobj, "shuffle_index", json_integer(shuffle_index));
		json_object_set_new(jobj, "recording", json_bool(recording));
		json_object_set_new(jobj, "dynamicChannels", json_bool(dynamicChannels));
		json_object_set_new(jobj, "startStepMode", json_bool(startStepMode));
		json_object_set_new(jobj, "skipPartialClock", json_bool(skipPartialClock));
		

		json_t *vaultJ = json_array();
		json_t *shuffle_arrJ = json_array();
		for(int vi = 0; vi < VAULT_SIZE; vi++){
			json_t *vaultRowJ = json_object();
			json_object_set_new(vaultRowJ, "cv", json_floatArray(vault_cv[vi],CHANNEL_COUNT));
			json_object_set_new(vaultRowJ, "gate", json_boolArray(vault_gate[vi],CHANNEL_COUNT));
			json_array_insert_new(vaultJ, vi, vaultRowJ);

			json_array_insert_new(shuffle_arrJ, vi, json_integer(shuffle_arr[vi]));
		}

		json_object_set_new(jobj, "vault", vaultJ);
		json_object_set_new(jobj, "shuffle_arr", shuffle_arrJ);

		return jobj;
	}

	void dataFromJson(json_t *jobj) override {
		setVaultPos(json_integer_value(json_object_get(jobj, "vault_pos")));
		playMode = (PlayMode)json_integer_value(json_object_get(jobj, "playMode"));
		channels = json_integer_value(json_object_get(jobj, "channels"));
		shuffle_index = json_integer_value(json_object_get(jobj, "shuffle_index"));
		recording = json_is_true(json_object_get(jobj, "recording"));
		dynamicChannels = json_is_true(json_object_get(jobj, "dynamicChannels"));
		startStepMode = json_is_true(json_object_get(jobj, "startStepMode"));
		skipPartialClock = json_is_true(json_object_get(jobj, "skipPartialClock"));
		

		json_t *vaultJ = json_object_get(jobj,"vault");
		json_t *shuffle_arrJ = json_object_get(jobj,"shuffle_arr");
		for(int vi = 0; vi < VAULT_SIZE; vi++){
			json_t *vaultRowJ = json_array_get(vaultJ,vi);
			json_floatArray_value(json_object_get(vaultRowJ,"cv"),vault_cv[vi],CHANNEL_COUNT);
			json_boolArray_value(json_object_get(vaultRowJ,"gate"),vault_gate[vi],CHANNEL_COUNT);
			shuffle_arr[vi] = json_integer_value(json_array_get(shuffle_arrJ,vi));
		}
	}

	void process(const ProcessArgs& args) override {

		outputs[CV_OUT_OUTPUT].setChannels(activeChannels);
		outputs[GATE_OUT_OUTPUT].setChannels(activeChannels);

		if(firstProcess){
			firstProcess = false;			

			lights[RECORD_LIGHT_LIGHT].setBrightness(recording ? 1.f : 0.f);
			lights[PLAY_LIGHT_LIGHT].setBrightness(recording ? 0.f : 1.f);

			stepSelect_prev = (int)params[STEP_KNOB_PARAM].getValue();
			updatePlayModeLights();
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
					//When changing play -> record do NOT change the position
				}else{
					//When changing record -> play mode reset play position
					setVaultPos(seqStart);
					partialPlayClock = skipPartialClock;

					//If done recording sort the current CVs
					sortAndClearCurrentCVs();
				}
			}
		}

		//Seq Mode Button
		{
			float btnValue = params[PLAY_MODE_PARAM].getValue();
			if(playModeBtnDown && btnValue <= 0){
				playModeBtnDown = false;
				if(playModeBtnDown_counter > 0){
					playModeBtnDown_counter = 0;

					//Only advance mode on release and if long press didn't trigger
					int pmi = playMode;
					pmi++;
					if(pmi == 4) pmi = 0;
					if(pmi == 8) pmi = 4;
					playMode = (PlayMode) pmi;

					updatePlayModeLights();	
				}
			}else if(!playModeBtnDown && btnValue > 0){
				playModeBtnDown = true;
				playModeBtnDown_counter = args.sampleRate;							
			}

			if(playModeBtnDown && playModeBtnDown_counter > 0){
				playModeBtnDown_counter--;
				if(playModeBtnDown_counter == 0){
					playMode = (PlayMode)((playMode + 4) % PlayMode_MAX);
					updatePlayModeLights();
				}
			}
		}

		if(inputs[LENGTH_CV_INPUT].isConnected()){
			if(!recording){
				//Playback Mode
				seqLength = (int)(inputs[LENGTH_CV_INPUT].getVoltage() / 5.01f * (VAULT_SIZE));
				while(seqLength < 0) seqLength += VAULT_SIZE;
				while(seqLength >= VAULT_SIZE) seqLength -= VAULT_SIZE;
				seqLength+=1;			
				params[LENGTH_KNOB_PARAM].setValue(seqLength);
			}else{
				//Don't animate Length knob when in record mode
				//Also don't update the length display based on CV input when in record mode
			}
		}else{
			seqLength = (int)params[LENGTH_KNOB_PARAM].getValue();
		}

		if(startStepMode && !recording){
			seqStart = getSeqStartPos();
		}else{
			int stepSelect = (int)params[STEP_KNOB_PARAM].getValue();
			
			if(stepSelect_prev != stepSelect){
				stepSelect_prev = stepSelect;
				vault_pos = stepSelect;
				
				//When cycling through the steps trigger a fake/preview gate for a quarter second
				stepSelect_previewGateTimer = args.sampleRate / 4;

				//Clear this to allow for previewing of steps
				partialPlayClock = false;
			}		
		}

		//Clock Detection
		{
			float clockValue = inputs[CLOCK_INPUT].getVoltage(); 
			if(clockHigh && clockValue <= 0.1f){
				clockHigh = false;
			}else if(!clockHigh && clockValue >= 2.0f){
				clockHigh = true;

				//If not recording, the advance the step here (on clock high)
				if(!recording){
					if(partialPlayClock){
						//Absorb the partical clock and don't advance the sequence
						partialPlayClock = false;
						setStartingVaultPosition();
					}else{
						nextVaultPosition();

						//Stop previewing when when moving to next step
						stepSelect_previewGateTimer = 0;
					}
				}
			}
		}

		if(!recording){
			bool resetEvent = false;
			//Reset Button
			{
				float btnValue = params[RESET_BTN_PARAM].getValue();
				if(resetBtnDown && btnValue <= 0){
					resetBtnDown = false;
				}else if(!resetBtnDown && btnValue > 0){
					resetBtnDown = true;
					resetEvent = true;
				}
			}

			//Reset Trigger
			{
				float triggerValue = inputs[RESET_INPUT].getVoltage();
				if(resetTrigHigh && triggerValue <= 0.1f){
					resetTrigHigh = false;
				}else if(!resetTrigHigh && triggerValue >= 2.0f){
					resetTrigHigh = true;
					resetEvent = true;
				}
			}

			if(resetEvent){
				setVaultPos(seqStart);
				partialPlayClock = skipPartialClock; //set this to true to cause the next gate to play step 1

				//Stop previewing on reset
				stepSelect_previewGateTimer = 0;				
			}
		}

		//Gate Detection
		{
			//When recording, use the max of all input gate values
			//This means any gate down will cause a record and all gates must go low for the next record to happen
			float maxGateValue = 0;
			for(int ci = 0; ci < channels; ci++){
				maxGateValue = std::max(maxGateValue,inputs[GATE_IN_INPUT].getVoltage(ci));
			}
			if(gatesHigh && maxGateValue <= 0.1f){
				gatesHigh = false;

				//In record mode, advance step on gates going low
				if(recording){
					//Sort previous CVs lowest to highest
					//We have to do extra work here to only sort CVs with high gates	
					sortAndClearCurrentCVs();

					setVaultPos((vault_pos + 1) % VAULT_SIZE);

					//Stop previewing when when moving to next step
					stepSelect_previewGateTimer = 0;
				}
			}else if(!gatesHigh && maxGateValue >= 2.0f){
				gatesHigh = true;

				if(recording){
					//On Gate high on this step, first clear all the gate values
					//Clear next gates
					for(int ci = 0; ci < CHANNEL_COUNT; ci++){
						vault_gate[getVaultPos()][ci] = false;
					}
				}
			}
		}

		bool outGateHigh = clockHigh;
		bool previewGateHigh = false;

		if(stepSelect_previewGateTimer > 0){
			stepSelect_previewGateTimer --;
			outGateHigh = true;
			previewGateHigh = true;
		}

		if(!recording && playMode == GLIDE){
			setVaultPos(getCV_vault_pos());
		}

		//Input/Output
		{
			for(int ci = 0; ci < channels; ci++){
				bool outputVaultValues = false;
				if(recording){
					float inCV = inputs[CV_IN_INPUT].getVoltage(ci);
					float inGate = inputs[GATE_IN_INPUT].getVoltage(ci);
					if(inGate >= 2.0f){
						vault_cv[getVaultPos()][ci] = inCV;
						vault_gate[getVaultPos()][ci] = true;
					}
					//Since we don't record to the vault until the gates are up we need this extra case here
					//If the gates are down (before the gates go up) we want to output the values that are form the inputs
					if(gatesHigh){
						outputs[CV_OUT_OUTPUT].setVoltage(inCV,ci);
						outputs[GATE_OUT_OUTPUT].setVoltage(inGate,ci);
					}else if(previewGateHigh){
						//Otherwise if we are previewing a note from the knob, we want to play it out
						outputVaultValues = true;
					}else{
						//Otherwise reset the gate, but let the CV retain its previous value
						outputs[GATE_OUT_OUTPUT].setVoltage(0,ci);
					}	
				}else if(partialPlayClock){
					//Don't play anything on the first partial clock 
					
					//But do clear the gates
					outputs[GATE_OUT_OUTPUT].setVoltage(0,ci);
				}else{
					outputVaultValues = true;
				}

				if(outputVaultValues){
					//Output Gate Value
					bool gateValue = vault_gate[getVaultPos()][ci];
					outputs[GATE_OUT_OUTPUT].setVoltage((outGateHigh && gateValue) ? 10.f : 0.f,ci);

					//Output CV Value
					//This check makes it so steps without a gate don't change CV and instead hold their previous value
					if(gateValue){
						outputs[CV_OUT_OUTPUT].setVoltage(vault_cv[getVaultPos()][ci],ci);
					}					
				}			
			}
		}
	}

	void setVaultPos(int new_pos){
		if(vault_pos == new_pos) return;
		vault_pos = new_pos;
		if(startStepMode && !recording){
			//Do Nothing
		}else{
			params[STEP_KNOB_PARAM].setValue(vault_pos);
			stepSelect_prev = new_pos;
		}

		updateActiveChannels();
	}

	void updateActiveChannels(){
		if(dynamicChannels && !recording){
			activeChannels = 0;
			for(int ci = 0; ci < channels; ci++){
				if(vault_gate[getVaultPos()][ci]) activeChannels++;
			}
		}else{
			activeChannels = channels;
		}
	}

	inline int getVaultPos(){
		return vault_pos % VAULT_SIZE;
	}

	void setStartingVaultPosition(){
		switch(playMode){
			case SKIP:
			case PING_PONG:
			case FORWARD:{
				setVaultPos(seqStart);
				}break;
		
			case BACKWARD:{
				setVaultPos(seqStart+seqLength-1);
				}break;

			case RANDOM:{
				nextVaultPosition();
				}break;

			case GLIDE:
			case CV:{
				setVaultPos(getCV_vault_pos());
				}break;

			case SHUFFLE:{
				//Set shuffle_index to 0 so the call to nextVaultPosition() creates a new sequence
				shuffle_index = 0;
				nextVaultPosition();
				}break;
		}
	}

	void nextVaultPosition(){
		switch(playMode){
			//Normal Modes
			case FORWARD:{
				setVaultPos(vault_pos+1);
				if(vault_pos >= seqLength+seqStart) setVaultPos(seqStart);
				}break;
		
			case BACKWARD:{
				setVaultPos(vault_pos-1);
				if(vault_pos < seqStart) setVaultPos(seqLength+seqStart-1);
				}break;

			case RANDOM:{
				if(seqLength == 1){
					setVaultPos(seqStart);
				}else{
					//Select a new position that isn't the current one
					int newPos = seqStart + (int)std::floor(rack::random::uniform() * (seqLength - 1));
					if(newPos >= vault_pos) newPos++;
					setVaultPos(newPos);
				}
				}break;

			case GLIDE:
			case CV:{
				setVaultPos(getCV_vault_pos());
				}break;

			//Secret Modes
			case SKIP:{
				//Chance of skipping a Chord, 20% by default but can be set by the CV_IN_INPUT
				if(rack::random::uniform() < inputs[STEP_CV_INPUT].getNormalVoltage(2.f) / 10.f){
					setVaultPos(vault_pos+2);
				}else{
					setVaultPos(vault_pos+1);
				}
				while(vault_pos >= seqLength+seqStart) vault_pos -= (seqLength);
				}break;

			case PING_PONG:{
				if(seqLength == 1){
					setVaultPos(seqStart);
				}else{
					if(pingPongDir){
						setVaultPos(vault_pos+1);
						if(vault_pos >= seqLength+seqStart){
							setVaultPos(seqLength-2+seqStart);
							pingPongDir = false;
						}
					}else{
						setVaultPos(vault_pos-1);
						if(vault_pos < seqStart){
							setVaultPos(seqStart+1);
							pingPongDir = true;
						}
					}
				}
				}break;

			case SHUFFLE:{
				if(shuffle_index == 0){
					for(int i = 0; i < VAULT_SIZE; i++){
						shuffle_arr[i] = i;
					}
					for(int i = 0; i < seqLength; i++){
						int d = (int)std::floor(rack::random::uniform() * i);
						//Swap shuffle_arr[i] & shuffle_arr[d]
						int v = shuffle_arr[i];
						shuffle_arr[i] = shuffle_arr[d];
						shuffle_arr[d] = v;
					}
				}
				shuffle_index++;
				if(shuffle_index >= seqLength) shuffle_index = 0;
				setVaultPos(seqStart+shuffle_arr[shuffle_index]);
				}break;
		}		
	}

	int getCV_vault_pos(){
		int newPos = inputs[STEP_CV_INPUT].getVoltage() / 5.01f * seqLength;
		while(newPos < 0) newPos += seqLength;
		while(newPos >= seqLength) newPos -= seqLength; 
		return newPos;
	}

	int getSeqStartPos(){
		int newPos = inputs[STEP_CV_INPUT].getVoltage() / 5.01f * VAULT_SIZE + params[STEP_KNOB_PARAM].getValue();
		while(newPos < 0) newPos += VAULT_SIZE;
		while(newPos >= VAULT_SIZE) newPos -= VAULT_SIZE; 
		return newPos;
	}

	void updatePlayModeLights(){
		lights[PLAY_FORWARD_LIGHT + 0].setBrightness(playMode == FORWARD ? 1 : 0);
		lights[PLAY_FORWARD_LIGHT + 1].setBrightness(playMode == SKIP ? 1 : 0);

		lights[PLAY_BACKWARD_LIGHT + 0].setBrightness(playMode == BACKWARD ? 1 : 0);
		lights[PLAY_BACKWARD_LIGHT + 1].setBrightness(playMode == PING_PONG ? 1 : 0);

		lights[PLAY_RANDOM_LIGHT + 0].setBrightness(playMode == RANDOM ? 1 : 0);
		lights[PLAY_RANDOM_LIGHT + 1].setBrightness(playMode == SHUFFLE ? 1 : 0);

		lights[PLAY_CV_LIGHT + 0].setBrightness(startStepMode || playMode == CV ? 1 : 0);
		lights[PLAY_CV_LIGHT + 1].setBrightness(playMode == GLIDE ? 1 : 0);
	}

	void sortAndClearCurrentCVs(){
		float activeCVs [CHANNEL_COUNT];
		int activeCV_count = 0;
		auto cvs = vault_cv[getVaultPos()];
		auto gates = vault_gate[getVaultPos()];
		for(int ci = 0; ci < channels; ci++){
			if(gates[ci]){
				activeCVs[activeCV_count] = cvs[ci];
				activeCV_count++;
			}
		}
		std::sort(activeCVs, activeCVs + activeCV_count, std::less<float>());
		activeCV_count = 0;
		for(int ci = 0; ci < channels; ci++){
			if(gates[ci]){
				cvs[ci] = activeCVs[activeCV_count];
				activeCV_count++;
			}else{
				//Clear any CVs on gates that are low
				//Not strictly nessecary because those values won't get used
				//But it just keeps the JSON clean
				cvs[ci] = 0;
			}
		}
	}

	void shiftNotes(int semitones){
		float voct = semitones / 12.f;
		for(int si = 0; si < VAULT_SIZE; si ++){
			for(int ci = 0; ci < CHANNEL_COUNT; ci++){
				if(vault_gate[si][ci]){
					vault_cv[si][ci] += voct;
				}
			}
		}
	}
};

struct ChordVaultWidget : ModuleWidget {

	struct CurStepKnob : LargeKnob {

		CurStepKnob() {
			
		}
		void drawLayer(const DrawArgs& args, int layer) override
		{
			engine::ParamQuantity* pq = getParamQuantity();
			if (pq==nullptr)
			{
				Widget::drawLayer(args,layer);
				return;
			}
			ChordVault* module = dynamic_cast<ChordVault*>(this->getParamQuantity()->module);
			if (module && layer == 1)
			{
				//float value = pq->getSmoothValue();

				Vec center = args.clipBox.getCenter();
				float angleRange = maxAngle - minAngle;
				float angleDelta = angleRange / VAULT_SIZE_MINUS_1;
				int start_index = module->seqStart;
				int end_index = start_index + module->seqLength;
				int cur_index = module->getVaultPos();
				for(int i = start_index; i < end_index; i++){
					int si = (i % VAULT_SIZE);
					float angle = minAngle + si * angleDelta;

					//No dot for current position
					bool currentStep = si == cur_index;
					nvgBeginPath(args.vg);	
					Vec pos = center + Vec(0,-15.6f).rotate(angle);
					nvgCircle(args.vg, pos.x, pos.y, 1.5f);
					nvgFillColor(args.vg, currentStep ? SCHEME_RED_CUSTOM : SCHEME_WHITE_CUSTOM);
					nvgFill(args.vg);
				}
			}
			Widget::drawLayer(args,layer);
		}
		void draw(const DrawArgs& args) override
	    {
	        LargeKnob::draw(args);
	    }
	};

	struct CurStepDisplay : DigitalDisplay {
		CurStepDisplay() {
			fontPath = asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf");
			bgText = "18";
			fontSize = 8;
		}
		ChordVault* module;
		int steps_prev = -1;
		void step() override {
			if (module) {
				int steps;
				if(!module->recording && module->partialPlayClock) steps = -1;
				else steps = module->getVaultPos();

				if (steps_prev != steps){
					steps_prev = steps;
					if(steps == -1){
						text = "1";
					}else{
						text = string::f("%d", steps + 1);	
					}
					int effStep = steps - steps;
					this->fgColor = effStep >= 0 && effStep < module->seqLength ? SCHEME_WHITE : SCHEME_RED_CUSTOM;
				}
			}else{
				text = string::f("1");	
			}		
		}
	};

	struct CurLengthDisplay : DigitalDisplay {
		CurLengthDisplay() {
			fontPath = asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf");
			bgText = "18";
			fontSize = 8;
		}
		ChordVault* module;
		int length_prev = -1;
		void step() override {
			if (module) {
				int length = module->seqLength;
				if (length_prev != length){
					length_prev = length;
					text = string::f("%d", length);
				}
			}else{
				text = string::f("4");
			}		
		}
	};

	ChordVaultWidget(ChordVault* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/ChordVault.svg")));

		addChild(createWidget<ScrewSilver>(Vec(0, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RotarySwitch<CurStepKnob>>(mm2px(Vec(18.101, 34.556)), module, ChordVault::STEP_KNOB_PARAM));
		addParam(createParamCentered<RotarySwitch<LargeKnob>>(mm2px(Vec(18.101, 70.424)), module, ChordVault::LENGTH_KNOB_PARAM));
		addParam(createParamCentered<SmallButton>(mm2px(Vec(17.403, 16.274)), module, ChordVault::RECORD_PLAY_BTN_PARAM));		
		addParam(createParamCentered<SmallButton>(mm2px(Vec(5.566, 85.815)), module, ChordVault::RESET_BTN_PARAM));
		addParam(createParamCentered<SmallButton>(mm2px(Vec(5.566, 50.141)), module, ChordVault::PLAY_MODE_PARAM));

		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(29.98, 34.111)), module, ChordVault::STEP_CV_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(5.648, 93.131)), module, ChordVault::RESET_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(5.648, 110.558)), module, ChordVault::CLOCK_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(18.143, 93.131)), module, ChordVault::GATE_IN_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(29.98, 93.131)), module, ChordVault::CV_IN_INPUT));
		addInput(createInputCentered<aetrion::Port>(mm2px(Vec(29.98, 70.691)), module, ChordVault::LENGTH_CV_INPUT));

		addOutput(createOutputCentered<aetrion::Port>(mm2px(Vec(18.061, 110.503)), module, ChordVault::GATE_OUT_OUTPUT));
		addOutput(createOutputCentered<aetrion::Port>(mm2px(Vec(29.937, 110.503)), module, ChordVault::CV_OUT_OUTPUT));

		// addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(10.991, 16.274)), module, ChordVault::RECORD_LIGHT_LIGHT));
		// addChild(createLightCentered<TinyLight<BlueLight>>(mm2px(Vec(23.815, 16.274)), module, ChordVault::PLAY_LIGHT_LIGHT));
		// addChild(createLightCentered<TinyLight<BlueRedLight>>(mm2px(Vec(10.334, 49.172)), module, ChordVault::PLAY_FORWARD_LIGHT));
		// addChild(createLightCentered<TinyLight<BlueRedLight>>(mm2px(Vec(15.89, 49.171)), module, ChordVault::PLAY_BACKWARD_LIGHT));
		// addChild(createLightCentered<TinyLight<BlueRedLight>>(mm2px(Vec(21.48, 49.172)), module, ChordVault::PLAY_RANDOM_LIGHT));
		// addChild(createLightCentered<TinyLight<BlueRedLight>>(mm2px(Vec(27.036, 49.172)), module, ChordVault::PLAY_CV_LIGHT));

		addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(11.991, 16.274)), module, ChordVault::RECORD_LIGHT_LIGHT));
		addChild(createLightCentered<SmallLight<BlueLight>>(mm2px(Vec(22.815, 16.274)), module, ChordVault::PLAY_LIGHT_LIGHT));
		addChild(createLightCentered<SmallLight<BlueRedLight>>(mm2px(Vec(10.334, 49.172)), module, ChordVault::PLAY_FORWARD_LIGHT));
		addChild(createLightCentered<SmallLight<BlueRedLight>>(mm2px(Vec(15.89, 49.171)), module, ChordVault::PLAY_BACKWARD_LIGHT));
		addChild(createLightCentered<SmallLight<BlueRedLight>>(mm2px(Vec(21.48, 49.172)), module, ChordVault::PLAY_RANDOM_LIGHT));
		addChild(createLightCentered<SmallLight<BlueRedLight>>(mm2px(Vec(27.036, 49.172)), module, ChordVault::PLAY_CV_LIGHT));

		{
			CurStepDisplay* display = createWidget<CurStepDisplay>(mm2px(Vec(5.235, 32.974)));
			display->box.size = mm2px(Vec(5.235, 32.974));
			display->textPos = mm2px(Vec(5.225+2, 2.976+3).div(2.f)); 
			display->module = module;
			addChild(display);
		}

		{
			CurLengthDisplay* display = createWidget<CurLengthDisplay>(mm2px(Vec(5.259, 68.849)));
			display->box.size = mm2px(Vec(5.226, 2.977));
			display->textPos = mm2px(Vec(5.225+2, 2.976+3).div(2.f)); 
			display->module = module;
			addChild(display);
		}
	}

	void appendContextMenu(Menu* menu) override {
		ChordVault* module = dynamic_cast<ChordVault*>(this->module);

		menu->addChild(new MenuEntry); //Blank Row
		menu->addChild(createMenuLabel("Chord Vault"));
		
		menu->addChild(createSubmenuItem("Squence Mode", PLAY_MODE_NAMES[module->playMode],
			[=](Menu* menu) {
				for(int i = 0; i < PlayMode_MAX; i++){
					menu->addChild(createMenuItem(PLAY_MODE_NAMES[i], CHECKMARK(module->playMode == i), [module,i]() { 
						module->playMode = (PlayMode)i;
						module->updatePlayModeLights();
					}));
				}
			}
		));

		menu->addChild(createSubmenuItem("Max Polyphony channels", std::to_string(module->channels),
			[=](Menu* menu) {
				for(int i = 3; i <= CHANNEL_COUNT; i++){
					menu->addChild(createMenuItem(std::to_string(i), CHECKMARK(module->channels == i), [module,i]() { 
						module->channels = i;
						module->updateActiveChannels();
					}));
				}
			}
		));

		menu->addChild(createSubmenuItem("Dynamic Channels", module->dynamicChannels ? "Yes" : "No",
			[=](Menu* menu) {
				menu->addChild(createMenuLabel("Change the number of output channels to match each chord?"));
				menu->addChild(createMenuItem("No", CHECKMARK(module->dynamicChannels == false), [module]() { 
					module->dynamicChannels = false;
				}));
				menu->addChild(createMenuItem("Yes", CHECKMARK(module->dynamicChannels == true), [module]() { 
					module->dynamicChannels = true;
				}));
			}
		));

		menu->addChild(createSubmenuItem("Start Step", module->startStepMode ? "Yes" : "No",
			[=](Menu* menu) {
				menu->addChild(createMenuLabel("Step Knob/CV adjust sequence start in Play Mode?"));
				menu->addChild(createMenuItem("No", CHECKMARK(module->startStepMode == false), [module]() { 
					module->startStepMode = false;
				}));
				menu->addChild(createMenuItem("Yes", CHECKMARK(module->startStepMode == true), [module]() { 
					module->startStepMode = true;
				}));
			}
		));

		menu->addChild(createSubmenuItem("Skip Partial Clock", module->skipPartialClock ? "Yes" : "No",
			[=](Menu* menu) {
				menu->addChild(createMenuLabel("Skip the first partial clock after reset/entering play mode?"));
				menu->addChild(createMenuItem("No", CHECKMARK(module->skipPartialClock == false), [module]() { 
					module->skipPartialClock = false;
				}));
				menu->addChild(createMenuItem("Yes", CHECKMARK(module->skipPartialClock == true), [module]() { 
					module->skipPartialClock = true;
				}));
			}
		));

		menu->addChild(createSubmenuItem("Shift Notes", "",
			[=](Menu* menu) {
				menu->addChild(createMenuLabel("Shift all notes by X Semitones"));
				menu->addChild(createMenuItem("-12 (Perfect Octave)", "", [module]() { 
					module->shiftNotes(-12);
				}));
				menu->addChild(createMenuItem("-11 (Major 7th)", "", [module]() { 
					module->shiftNotes(-11);
				}));
				menu->addChild(createMenuItem("-10 (Minor 7th)", "", [module]() { 
					module->shiftNotes(-10);
				}));
				menu->addChild(createMenuItem("-9 (Major 6th)", "", [module]() { 
					module->shiftNotes(-9);
				}));
				menu->addChild(createMenuItem("-8 (Minor 6th)", "", [module]() { 
					module->shiftNotes(-8);
				}));
				menu->addChild(createMenuItem("-7 (Perfect 5th)", "", [module]() { 
					module->shiftNotes(-7);
				}));
				menu->addChild(createMenuItem("-6 (Diminished 5th)", "", [module]() { 
					module->shiftNotes(-6);
				}));
				menu->addChild(createMenuItem("-5 (Perfect 4th)", "", [module]() { 
					module->shiftNotes(-5);
				}));
				menu->addChild(createMenuItem("-4 (Major 3rd)", "", [module]() { 
					module->shiftNotes(-4);
				}));
				menu->addChild(createMenuItem("-3 (Minor 3rd)", "", [module]() { 
					module->shiftNotes(-3);
				}));
				menu->addChild(createMenuItem("-2 (Major 2nd)", "", [module]() { 
					module->shiftNotes(-2);
				}));
				menu->addChild(createMenuItem("-1 (Minor 2nd)", "", [module]() { 
					module->shiftNotes(-1);
				}));
				menu->addChild(createMenuLabel(""));
				menu->addChild(createMenuItem("+1 (Minor 2nd)", "", [module]() { 
					module->shiftNotes(+1);
				}));
				menu->addChild(createMenuItem("+2 (Major 2nd)", "", [module]() { 
					module->shiftNotes(+2);
				}));
				menu->addChild(createMenuItem("+3 (Minor 3rd)", "", [module]() { 
					module->shiftNotes(+3);
				}));
				menu->addChild(createMenuItem("+4 (Major 3rd)", "", [module]() { 
					module->shiftNotes(+4);
				}));
				menu->addChild(createMenuItem("+5 (Perfect 4th)", "", [module]() { 
					module->shiftNotes(+5);
				}));
				menu->addChild(createMenuItem("+6 (Diminished 5th)", "", [module]() { 
					module->shiftNotes(+6);
				}));
				menu->addChild(createMenuItem("+7 (Perfect 5th)", "", [module]() { 
					module->shiftNotes(+7);
				}));
				menu->addChild(createMenuItem("+8 (Augmented 5th)", "", [module]() { 
					module->shiftNotes(+8);
				}));
				menu->addChild(createMenuItem("+9 (Major 6th)", "", [module]() { 
					module->shiftNotes(+9);
				}));
				menu->addChild(createMenuItem("+10 (Minor 7th)", "", [module]() { 
					module->shiftNotes(+10);
				}));
				menu->addChild(createMenuItem("+11 (Major 7th)", "", [module]() { 
					module->shiftNotes(+11);
				}));
				menu->addChild(createMenuItem("+12 (Perfect Octave)", "", [module]() { 
					module->shiftNotes(+12);
				}));
			}
		));
	}
};


Model* modelChordVault = createModel<ChordVault, ChordVaultWidget>("ChordVault");