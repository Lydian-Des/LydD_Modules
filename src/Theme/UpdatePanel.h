int curr = ((MODULE_NAME*)module)->currPanel;
int prev = ((MODULE_NAME*)module)->prevPanel;

if (curr != prev) {
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

	case 4: // Fruitiger
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/Fruitiger/" + panel)));
		break;

	case 5: // White
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/White/" + panel)));
		break;

	case 6: //Soulless
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AltPanels/Soulless/" + panel)));
		break;
	}

	((MODULE_NAME*)module)->prevPanel = curr;
}