#include "plugin.hpp"
#include "barkComponents.hpp"

using namespace barkComponents;

struct Clamp : Module {
	enum ParamIds {
		MAX_PARAM,
		CEILING_PARAM,
		MIN_PARAM,
		LINKMINMAX_PARAM,
		_2BY_PARAM,
		GAIN_PARAM,
		ENUMS(ATTENUVERT_BUTTONS, 4),
		NUM_PARAMS
	};
	enum InputIds {
		INL_INPUT,
		INR_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OUTL_OUTPUT,
		OUTR_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {	NUM_LIGHTS };

	float volt1, volt2, prevMax = 0.f, prevMin = 0.f;

	Clamp() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MAX_PARAM, -10.f, 10.f, 10.f, "Max", "v");
		configParam<tpCeiling>(CEILING_PARAM, 0.f, 1.f, 0.f, "Celing");
		configParam(MIN_PARAM, -10.f, 10.f, -10.f, "Min", "v");
		configParam<tpOnOff>(LINKMINMAX_PARAM, 0.f, 1.f, 1.f, "Link");
		configParam(_2BY_PARAM, -2.f, 2.f, 1.f, "Multiplier");
		configParam(GAIN_PARAM, 0.f, 4.f, 1.f, "Input Gain", "dB", -10, 40);
		for (int i = 0; i < 4; i++) {
			configParam<tpOnOffBtn>(ATTENUVERT_BUTTONS + i, 0.f, 1.f, 0.f, "Snap to");
		}
	}//config
	
	void process(const ProcessArgs &args) override {
		float prevVal1 = 0.f, prevVal2 = 0.f;
		prevVal1 = params[MAX_PARAM].getValue();
		prevVal2 = params[MIN_PARAM].getValue();
		//display
		volt1 = params[MAX_PARAM].getValue();
		volt2 = params[MIN_PARAM].getValue();

		//------- one knob controls other, 
		if (params[LINKMINMAX_PARAM].getValue() < 1.f && prevVal1 != prevMax) {
			params[MIN_PARAM].setValue(-prevVal1);
		} if (params[LINKMINMAX_PARAM].getValue() < 1.f && prevVal2 != prevMin) {
			params[MAX_PARAM].setValue(-prevVal2);
			prevMax = prevVal1;
			prevMin = prevVal2;
		}

		//pad 0.1dB
		if (params[CEILING_PARAM].getValue() > 0.f) {
			volt1 = 9.94f;
			volt2 = -9.94f;
		} else {
			volt1 = params[MAX_PARAM].getValue();
			volt2 = params[MIN_PARAM].getValue();
		}

		params[MAX_PARAM].setValue(volt1);
		params[MIN_PARAM].setValue(volt2);

		//--------------------
		float gain = params[GAIN_PARAM].getValue(), attenuvert = params[_2BY_PARAM].getValue(), inL, inR;

		//snap atten value to knob
		params[ATTENUVERT_BUTTONS + 0].getValue() == 1.f ? params[_2BY_PARAM].setValue(-1.f) : params[_2BY_PARAM].setValue(params[_2BY_PARAM].getValue());
		params[ATTENUVERT_BUTTONS + 1].getValue() == 1.f ? params[_2BY_PARAM].setValue(1.f)  : params[_2BY_PARAM].setValue(params[_2BY_PARAM].getValue());
		params[ATTENUVERT_BUTTONS + 2].getValue() == 1.f ? params[_2BY_PARAM].setValue(-2.f) : params[_2BY_PARAM].setValue(params[_2BY_PARAM].getValue());
		params[ATTENUVERT_BUTTONS + 3].getValue() == 1.f ? params[_2BY_PARAM].setValue(2.f)  : params[_2BY_PARAM].setValue(params[_2BY_PARAM].getValue());
		
		//apply pre gain and attenuvert
		inL = clamp(inputs[INL_INPUT].getVoltage() * gain * attenuvert, fmin(params[MIN_PARAM].getValue(), params[MAX_PARAM].getValue()),
			fmax(params[MAX_PARAM].getValue(), params[MIN_PARAM].getValue()));
		inR = clamp(inputs[INR_INPUT].getVoltage() * gain * attenuvert, fmin(params[MIN_PARAM].getValue(), params[MAX_PARAM].getValue()),
				fmax(params[MAX_PARAM].getValue(), params[MIN_PARAM].getValue()));
		
		///output---
		//if not connected sets outputs to offsets
		inputs[INL_INPUT].isConnected() == true ? 
			outputs[OUTL_OUTPUT].setVoltage(inL) : outputs[OUTL_OUTPUT].setVoltage(params[MAX_PARAM].getValue());
		inputs[INL_INPUT].isConnected() == true ? 
			outputs[OUTR_OUTPUT].setVoltage(inR) : outputs[OUTR_OUTPUT].setVoltage(params[MIN_PARAM].getValue());
				
	}//process
};

//-----------------------------------  Volt Display  -----------------------------------
struct voltDisplayWidget : TransparentWidget {
	Clamp *module;
	float *value;
	std::shared_ptr<Font> font;

	voltDisplayWidget() {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/segoescb.ttf"));
	}

	void draw(const DrawArgs &voltDisp) override {
		NVGcolor backgroundColor = nvgRGB(26, 26, 36);
		NVGcolor borderColor = nvgRGB(0, 0, 0);
		NVGcolor gradStartCol = nvgRGBA(255, 255, 244, 17);
		NVGcolor gradEndCol = nvgRGBA(0, 0, 0, 15);
		NVGcolor textColor = nvgRGB(63, 154, 0);
		nvgBeginPath(voltDisp.vg);
		nvgRoundedRect(voltDisp.vg, 0.0, 0.0, box.size.x, box.size.y, 0.75);
		nvgFillColor(voltDisp.vg, backgroundColor);
		nvgFill(voltDisp.vg);
		nvgStrokeWidth(voltDisp.vg, 0.75);
		nvgStrokeColor(voltDisp.vg, borderColor);
		nvgStroke(voltDisp.vg);
		nvgTextAlign(voltDisp.vg, 1 << 1);
		nvgFontSize(voltDisp.vg, 18);
		nvgFontFaceId(voltDisp.vg, font->handle);
		nvgTextLetterSpacing(voltDisp.vg, 0.75);
		char display_string[10];
		sprintf(display_string, "%5.2f", *value);
		Vec textPos = Vec(25.0f, 10.55f);		//		box.size = Vec(50.728f, 13.152f);
		nvgFillColor(voltDisp.vg, nvgTransRGBA(nvgRGB(0xdf, 0xd2, 0x2c), 16));
		nvgText(voltDisp.vg, textPos.x, textPos.y, "$$$$", NULL);
		nvgFillColor(voltDisp.vg, nvgTransRGBA(nvgRGB(0xda, 0xe9, 0x29), 11));
		nvgText(voltDisp.vg, textPos.x, textPos.y, "##.##", NULL);
		nvgFillColor(voltDisp.vg, textColor);
		nvgText(voltDisp.vg, textPos.x, textPos.y, display_string, NULL);
		//---------Gradient Screen
		nvgRoundedRect(voltDisp.vg, 0.f, 0.f, box.size.x, box.size.y, .75f);
		//(startX,startY)-(endX,endY) should be the reverse of inkscape coordinates 
		float gradHeight = 12.728f;
		nvgFillPaint(voltDisp.vg, nvgLinearGradient(voltDisp.vg, 71.5f, gradHeight - 4.98f, 70.61f, 
							    gradHeight - 10.11f, gradStartCol, gradEndCol));
		nvgFill(voltDisp.vg);
	}
};
//-----------------------------------  Volt Display  -----------------------------------

struct ClampWidget : ModuleWidget {
	ClampWidget(Clamp *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BarkClamp.svg")));

		//constexpr int rackY = 380;
		constexpr float portLX = 4.11f, portRX = 31.67f, inY = 187.78f, outY = 60.18f,
				att1xPos[2] = { 4.77f, 44.48f }, att2xPos[2] = { 15.34f, 34.15f };
		///Ports---
		//Out---
		addOutput(createOutput<BarkOutPort350>(Vec(portLX, rackY - outY), module, Clamp::OUTL_OUTPUT));
		addOutput(createOutput<BarkOutPort350>(Vec(portRX, rackY - outY), module, Clamp::OUTR_OUTPUT));
		//In---
		addInput(createInput<BarkInPort350>(Vec(portLX, rackY - inY), module, Clamp::INL_INPUT));
		addInput(createInput<BarkInPort350>(Vec(portRX, rackY - inY), module, Clamp::INR_INPUT));
		//Knobs---
		addParam(createParam<BarkKnob_40>(Vec(14.677f, rackY - 333.8f), module, Clamp::MAX_PARAM));
		addParam(createParam<BarkKnob_40>(Vec(14.677f, rackY - 259.8f), module, Clamp::MIN_PARAM));
		addParam(createParam<BarkKnob_20>(Vec(20.f, rackY - 155.16f), module, Clamp::_2BY_PARAM));
		addParam(createParam<BarkKnob_40>(Vec(10.f, rackY - 119.14f), module, Clamp::GAIN_PARAM));
		//Switch---
		addParam(createParam<BarkSwitchSmallSide>(Vec(26.68f, rackY - 283.7f), module, Clamp::LINKMINMAX_PARAM));
		//button---
		addParam(createParam<BarkPushButton1>(Vec(2.42f, rackY - 334.44f), module, Clamp::CEILING_PARAM));
		addParam(createParam<BarkPushButton1>(Vec(att1xPos[0], rackY - 146.75f), module, Clamp::ATTENUVERT_BUTTONS + 0));
		addParam(createParam<BarkPushButton1>(Vec(att1xPos[1], rackY - 146.75f), module, Clamp::ATTENUVERT_BUTTONS + 1));
		addParam(createParam<BarkPushButton1>(Vec(att2xPos[0], rackY - 132.07f), module, Clamp::ATTENUVERT_BUTTONS + 2));
		addParam(createParam<BarkPushButton1>(Vec(att2xPos[1], rackY - 132.07f), module, Clamp::ATTENUVERT_BUTTONS + 3));
		//screw---
		addChild(createWidget<BarkScrew2>(Vec(box.size.x - 13, 3)));				//pos2
		addChild(createWidget<BarkScrew3>(Vec(2, 367.2f)));						//pos3
		//Display---
		if (module != NULL) {
			voltDisplayWidget *maxVolt = createWidget<voltDisplayWidget>(Vec(4.61f, rackY - 355.65f));
			maxVolt->box.size = Vec(50.728f, 13.152f);
			maxVolt->value = &module->volt1;
			addChild(maxVolt);
			voltDisplayWidget *minVolt = createWidget<voltDisplayWidget>(Vec(4.61f, rackY - 210.51f));
			minVolt->box.size = Vec(50.728f, 13.152f);
			minVolt->value = &module->volt2;
			addChild(minVolt);
		}
	}
};

Model *modelClamp = createModel<Clamp, ClampWidget>("Clamp");
