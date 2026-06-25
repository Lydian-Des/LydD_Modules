#include "plugin.hpp"

Plugin* pluginInstance;

int defaultPanel = 0;
int prevdefaultPanel = -1;

void setDefPanel(int pan, bool prev) {
	if (prev)
		prevdefaultPanel = pan;
	else
		defaultPanel = pan;
}
int getDefPanel(bool prev) {
	return prev ? prevdefaultPanel : defaultPanel;
}
//vcv suggestion for saving plugin level setting
json_t* settingsToJson() {
	json_t* rootJ = json_object();
	json_object_set_new(rootJ, "PanelDefault", json_integer(defaultPanel));
	return rootJ;
}
void settingsFromJson(json_t* rootJ) {
	json_t* PanelJ = json_object_get(rootJ, "PanelDefault");
	if (PanelJ)
		defaultPanel = json_integer_value(PanelJ);
}

void init(rack::Plugin* p) {
	pluginInstance = p;
	LydD::Components::setPlugin(p);



	p->addModel(modelPoppy);
	p->addModel(modelDadMom);
	p->addModel(modelSimone);
	p->addModel(modelTorus);
	p->addModel(modelClock);
	p->addModel(modelDobbs);
	p->addModel(modelLedger);
	p->addModel(modelOnceler);
	p->addModel(modelShear);
	p->addModel(modelSwitch);
	p->addModel(modelQuant);
	p->addModel(modelReflect);
	p->addModel(modelCanyon);
	p->addModel(modelSeethe);

}
