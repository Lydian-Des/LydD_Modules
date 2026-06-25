
//individual panel menu
struct ListItem : MenuItem {
	MODULE_NAME* module;
	//each panel type sets this on creation, may as well default to 0
	int Panel = 0;

	void onAction(const event::Action& e) override {
		module->currPanel = Panel;
	}
};

// default theme menu
struct DefaultListItem : MenuItem {
	MODULE_NAME* module;
	//same name here for ease of duplication only
	int Panel = 0;

	void onAction(const event::Action& e) override {
		// write panel to global value, gets put in setting.json next check
		setDefPanel(Panel, false);

		// set current panel as well
		module->currPanel = Panel;
	}
};

struct PanelMenu : MenuItem {
	MODULE_NAME* module;

	Menu* createChildMenu() override {
		Menu* menu = new Menu;

		ListItem* OriginalMenuItem = createMenuItem<ListItem>("Dark", CHECKMARK(module->currPanel == 0));
		OriginalMenuItem->module = module;
		OriginalMenuItem->Panel = 0;
		menu->addChild(OriginalMenuItem);


		ListItem* BlueMenuItem = createMenuItem<ListItem>("Blue Night", CHECKMARK(module->currPanel == 1));
		BlueMenuItem->module = module;
		BlueMenuItem->Panel = 1;
		menu->addChild(BlueMenuItem);


		ListItem* PurpleMenuItem = createMenuItem<ListItem>("Purple Jewel", CHECKMARK(module->currPanel == 2));
		PurpleMenuItem->module = module;
		PurpleMenuItem->Panel = 2;
		menu->addChild(PurpleMenuItem);

		ListItem* RedMenuItem = createMenuItem<ListItem>("Red Ruby", CHECKMARK(module->currPanel == 3));
		RedMenuItem->module = module;
		RedMenuItem->Panel = 3;
		menu->addChild(RedMenuItem);

		ListItem* WhiteMenuItem = createMenuItem<ListItem>("White", CHECKMARK(module->currPanel == 4));
		WhiteMenuItem->module = module;
		WhiteMenuItem->Panel = 4;
		menu->addChild(WhiteMenuItem);

		ListItem* SoulMenuItem = createMenuItem<ListItem>("Soulless", CHECKMARK(module->currPanel == 5));
		SoulMenuItem->module = module;
		SoulMenuItem->Panel = 5;
		menu->addChild(SoulMenuItem);

		return menu;
	}
};
struct DefaultPanelMenu : MenuItem {
	MODULE_NAME* module;

	Menu* createChildMenu() override {
		Menu* menu = new Menu;


		int currDefault = getDefPanel(false);

		DefaultListItem* OriginalMenuItem = createMenuItem<DefaultListItem>("Dark", CHECKMARK(currDefault == 0));
		OriginalMenuItem->module = module;
		OriginalMenuItem->Panel = 0;
		menu->addChild(OriginalMenuItem);


		DefaultListItem* BlueMenuItem = createMenuItem<DefaultListItem>("Blue Night", CHECKMARK(currDefault == 1));
		BlueMenuItem->module = module;
		BlueMenuItem->Panel = 1;
		menu->addChild(BlueMenuItem);


		DefaultListItem* PurpleMenuItem = createMenuItem<DefaultListItem>("Purple Jewel", CHECKMARK(currDefault == 2));
		PurpleMenuItem->module = module;
		PurpleMenuItem->Panel = 2;
		menu->addChild(PurpleMenuItem);

		DefaultListItem* RedMenuItem = createMenuItem<DefaultListItem>("Red Ruby", CHECKMARK(currDefault == 3));
		RedMenuItem->module = module;
		RedMenuItem->Panel = 3;
		menu->addChild(RedMenuItem);

		DefaultListItem* WhiteMenuItem = createMenuItem<DefaultListItem>("White", CHECKMARK(currDefault == 4));
		WhiteMenuItem->module = module;
		WhiteMenuItem->Panel = 4;
		menu->addChild(WhiteMenuItem);

		DefaultListItem* SoulMenuItem = createMenuItem<DefaultListItem>("Soulless", CHECKMARK(currDefault == 5));
		SoulMenuItem->module = module;
		SoulMenuItem->Panel = 5;
		menu->addChild(SoulMenuItem);

		return menu;
	}
};