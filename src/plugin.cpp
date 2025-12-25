#include "plugin.hpp"

Plugin* pluginInstance;

void init(rack::Plugin* p) {
	pluginInstance = p;

	// Here you must add all the models that your plugin implements.
	// There must be at least one. Often there are many more.
	p->addModel(modelPoppy);
	p->addModel(modelDadMom);
	p->addModel(modelSimone);
	p->addModel(modelTorus);
	p->addModel(modelClock);
	p->addModel(modelDobbs);
	p->addModel(modelLedger);
	p->addModel(modelOnceler);
	p->addModel(modelShear);
}
