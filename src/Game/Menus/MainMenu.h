#pragma once

#include <cstdint>

struct MenuItem {
	int32_t pad_00;
	char id[32];
	char title[32];
	// size 0x44
};

struct SubMenu {
	int32_t pad_00, pad_04;
	char id[32]; // 0x08
	char title[32]; // 0x28
	int item_count; // 0x48
	int item_index; // 0x4C, values 0-N
	MenuItem items[24]; // 0x50
	// size 0x6B0
};

struct MainMenu {
	// starts at base+e8c044
	char pad_00[0x54];
	int menu_level; // 0x54, 0 for main menu, 1 for sumbenu, more for other menus
	char pad_0058[0x18];
	//char pad_00[0x64];
	//int menu_level; // 0x64, 0 for main menu, 1 for sumbenu, more for other menus
	//char pad_0068[0x08];
	SubMenu sub_menus[16]; // 0x70 to 0x6B70. practice, story, network (when online), network (when offline), network (player), network (ranked), collection, options
	int pad_6B70;
	int sub_menu_index; // 0x6B74, values 0,1,2,3/4,7,8
};


void run_menu_action(int sub_menu_index, int item_index);

void run_ranked_menu_action(int item_index);


void DrawMenuStateMachine();