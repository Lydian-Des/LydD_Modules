int curr = module ? module->currPanel : getDefPanel(false);

	switch (curr) {
	case 0:	// Dark
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/Dark/" + panel)));
		break;

	case 1: // Blue
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/Blue/" + panel)));
		break;

	case 2: // Purple
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/Purple/" + panel)));
		break;

	case 3: // Red
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/Red/" + panel)));
		break;

	case 4: // White
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/White/" + panel)));
		break;

	case 5: //Soulless
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/Soulless/" + panel)));
		break;
	}
