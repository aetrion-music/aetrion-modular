#pragma once

#include "rack.hpp"
#include <functional>
#include <string>

using namespace rack;

extern Plugin *pluginInstance;

namespace  aetrion {

static const NVGcolor SCHEME_RED_CUSTOM = nvgRGB(0xf6, 0x8f, 0xb2);

//LargeKnob
struct LargeKnob : RoundKnob {
	LargeKnob();
};

//Port
struct Port : SvgPort {
	Port();
};

struct SmallButton : app::SvgSwitch {
	SmallButton();
};

template <typename TBase>
struct RotarySwitch : TBase {
	RotarySwitch() {
		this->snap = true;
		this->smooth = false;
	}
	
	// handle the manually entered values
	void onChange(const event::Change &e) override {		
		SvgKnob::onChange(e);		
		this->getParamQuantity()->setValue(roundf(this->getParamQuantity()->getValue()));
	}
};

struct DigitalDisplay : Widget {
	std::string fontPath;
	std::string bgText;
	std::string text;
	float fontSize;

	NVGcolor fgColor = SCHEME_WHITE;
	Vec textPos;

	void prepareFont(const DrawArgs& args) {
		// Get font
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		if (!font)
			return;
		nvgFontFaceId(args.vg, font->handle);
		nvgFontSize(args.vg, fontSize);
		nvgTextLetterSpacing(args.vg, 0.0);
		nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
	}

	void draw(const DrawArgs& args) override {
		//Background
		nvgBeginPath(args.vg);
		nvgRect(args.vg, -1, -1, box.size.x+2, box.size.y+2);
		nvgFillColor(args.vg, nvgRGB(0x06, 0x0e, 0x2c)); //Very dark blue
		nvgFill(args.vg);

		prepareFont(args);

		// Background text
		nvgFillColor(args.vg, nvgRGB(0x0f, 0x26, 0x74));//Slightly less dark dark blue
		nvgText(args.vg, textPos.x, textPos.y, bgText.c_str(), NULL);
	}

	void drawLayer(const DrawArgs& args, int layer) override {
		if (layer == 1) {
			prepareFont(args);

			// Foreground text
			nvgFillColor(args.vg, fgColor);
			nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
		}
		Widget::drawLayer(args, layer);
	}
};


}