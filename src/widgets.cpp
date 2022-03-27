#include "widgets.hpp"

namespace aetrion {

LargeKnob::LargeKnob() {
	setSvg(Svg::load(asset::plugin(pluginInstance,"res/large_knob_dial.svg")));
	bg->setSvg(Svg::load(asset::plugin(pluginInstance,"res/large_knob_bg.svg")));
	box.size = Vec(41.66, 41.66);
	shadow->blurRadius = 1.0;
	shadow->box.pos = Vec(0.0, 3.0);
}

struct RangeWidget : widget::TransparentWidget {
	float startAngle;
	float endAngle;
	float startAngle2;
	float endAngle2;

	bool drawSecondRange = false;

	RangeWidget(){

	}
	void draw(const DrawArgs& args) override {
		Vec center = box.getCenter();
		nvgStrokeColor(args.vg, SCHEME_WHITE_CUSTOM);
		nvgStrokeWidth(args.vg, 0.8f);

		nvgBeginPath(args.vg);
		nvgArc(args.vg, center.x, center.y, /*radius:*/ 19.2415f, startAngle - PI/2, endAngle - PI/2, NVG_CW);
		nvgStroke(args.vg);
		if(drawSecondRange){
			nvgBeginPath(args.vg);
			nvgArc(args.vg, center.x, center.y, /*radius:*/ 19.2415f, startAngle2 - PI/2, endAngle2 - PI/2, NVG_CW);
			nvgStroke(args.vg);
		}
	}
};

LargeKnobWithRange::LargeKnobWithRange() {
	rangeWidget = new RangeWidget();
	rangeWidget->startAngle = this->minAngle;
	rangeWidget->endAngle = this->maxAngle;
	rangeWidget->box = this->box;
	//this->addChild(rangeWidget);
	fb->addChildAbove(rangeWidget,bg);
}

void LargeKnobWithRange::updateRange(float min, float max){
	float rangeDelta = (maxAngle - minAngle);
	rangeWidget->startAngle = minAngle + min * rangeDelta;
	if(max <= 1){
		rangeWidget->drawSecondRange = false;
		rangeWidget->endAngle = minAngle + max * rangeDelta;
	}else{
		rangeWidget->drawSecondRange = true;
		rangeWidget->endAngle = maxAngle;
		rangeWidget->startAngle2 = minAngle;
		rangeWidget->endAngle2 = minAngle + (max-1) * rangeDelta;
	}
	fb->dirty = true;
}



Port::Port() {
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

}