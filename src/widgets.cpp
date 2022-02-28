#include "widgets.hpp"

using namespace aetrion;

LargeKnob::LargeKnob() {
	setSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/large_knob.svg")));
	box.size = Vec(41.66, 41.66);
	shadow->blurRadius = 1.0;
	shadow->box.pos = Vec(0.0, 3.0);
}


aetrion::Port::Port() {
	setSvg(Svg::load(asset::plugin(pluginInstance,"res/port.svg")));
	box.size = Vec(26.23, 26.23);
	shadow->blurRadius = 1.0;
	shadow->box.pos = Vec(0.0, 1.5);
}

SmallButton::SmallButton() {
	momentary = true;
	addFrame(Svg::load(asset::plugin(pluginInstance,"res/small_button_0.svg")));
	addFrame(Svg::load(asset::plugin(pluginInstance,"res/small_button_1.svg")));
	shadow->blurRadius = 1.0;
	shadow->box.pos = Vec(0.0, 1.5);
}