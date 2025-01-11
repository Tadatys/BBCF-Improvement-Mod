#include "MainMenu.h"

#include <imgui.h>

#include <set>
#include "Core/utils.h"
#include "Core/interfaces.h"



void run_ranked_menu_action(int item_index) {
	// only works when ranked menu is already open?
	char* base = GetBbcfBaseAdress();
	char* ranked_menu_base = base + 0xDAAF70; // different index location from main menu, even though ranked menu items are in the main menu struct
	char* run_ranked_menu_action = base + 0x142536;
	char* edx_value = base + 0x49B13C; // no idea what it means

	_asm {
		pusha

		mov esi, ranked_menu_base
		mov eax, item_index
		mov[esi + 0x28], eax
		mov eax, run_ranked_menu_action
		mov ebx, 0
		mov ecx, esi
		mov edx, edx_value
		call dword ptr[eax + 04]

		popa
	}
}

void run_menu_action(int sub_menu_index, int item_index) {
	char* base = GetBbcfBaseAdress();
	MainMenu* menu = (MainMenu*)(base + 0xe8c044); // start of menu data
	char* run_menu_action = base + 0x31A5A0;

	menu->sub_menu_index = sub_menu_index;
	menu->sub_menus[menu->sub_menu_index].item_index = item_index;

	_asm
	{
		pushad

		mov ecx, menu
		mov esi, ecx
		mov eax, 0
		mov ebx, 0
		push eax

		call run_menu_action

		popad
	}
}

void write_room_defaults() {
	char* base = GetBbcfBaseAdress();

	// room data is weirdly spread out. starts at 0xDA7C40
	//*(base + 0xDA7C40 + 4) = 0; // set to 1 when menu is open? 
	*(base + 0xDA7E0C) = 1; // room type
	*(base + 0xDA7EC4) = 6; // room capacity
	*(base + 0xDA7F7C) = 0; // room invitations
	*(base + 0xDA8034) = 2; // room connectivity
	*(base + 0xDA80EC) = 0; // room color
	*(base + 0xDA81A4) = 0; // room match limit
	*(base + 0xDA825C) = 1; // room rematch
	*(base + 0xDA8314) = 1; // room auto pass
	*(base + 0xDA83CC) = 0; // room skip time
	*(base + 0xDA8484) = 0; // room chat
	*(base + 0xDA853C) = 3; // room rotation
	*(base + 0xDA85F4) = 1; // room to win
	*(base + 0xDA86AC) = 4; // room time
	memcpy(base + 0xDA93A0, "E\0U\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 32); // room name, utf16
	// TODO: move the values to settings.def
}

void write_controller_select_defaults() {
	// controller select screen, seen after selecting practice mode in main menu

	char* base = GetBbcfBaseAdress();
	*(int32_t*)(base + 0xE92C44) = 0;
	*(int32_t*)(base + 0xE92C48) = -1;
}

void write_character_select_defaults() {
	// character select screen

	char* base = GetBbcfBaseAdress();
	// x = column * 81, 0 is center; y = col_start + row * 88. 
	*(float_t*)(base + 0xE3F8D0) = 81; // p1 selection cursor x
	*(float_t*)(base + 0xE3F8D4) = 304; // y
	// TODO: default colors, maybe stylish, maybe mark all as selected. would that be a problem online?

	//*(float_t*)(base + 0xE3F8D8) = -243; // p2 selection cursor x
	//*(float_t*)(base + 0xE3F8Dc) = 118; // y
	
	// TODO: these don't get reset, they could be written on mod start
	// these are same as g_gameVals.stageSelect_X, _Y, musicSelect_X, _Y
	*(int32_t*)(base + 0xe41150) = 1; // stage select x -- 1 is random
	*(int32_t*)(base + 0xe41154) = 0; // y

	*(int32_t*)(base + 0xE413C0) = 1; // music select x -- 1 is random
	*(int32_t*)(base + 0xE413C4) = 0; // y
}


void DrawMenuStateMachine() {
	if (ImGui::Button("Open Practice")) {

		run_menu_action(0, 1); // select training mode
	}

	ImGui::SameLine();

	if (ImGui::Button("Open Net")) {

		run_menu_action(4, 0); // log in
		run_menu_action(3, 1); // open player menu
		// running this twice breaks something?
	}

	ImGui::SameLine();

	if (ImGui::Button("Toggle Ranked Entry")) {
		// only worls when ranked menu is already open?
		run_ranked_menu_action(1); // entry
	}

	ImGui::SameLine();

	if (ImGui::Button("Write room defaults")) {
		write_room_defaults();
	}


	static int timeout = 0;
	const char* do_nothing = "<none>";
	static const char* current_state = "practice - select";
	static std::set<const char*> all_states;
	static bool playing = false;

	ImGui::PushItemWidth(100);
	if (ImGui::BeginCombo("state", current_state)) {

		for (auto st : all_states) {
			if (ImGui::Selectable(st, st == current_state)) {
				current_state = st;
			}
		}

		ImGui::EndCombo();
	}

	ImGui::SameLine();

	ImGui::InputInt("timeout", &timeout);
	ImGui::PopItemWidth();

	ImGui::SameLine();

	if (current_state == do_nothing) playing = false;
	
	if (!playing && ImGui::Button("start")) playing = true;
	if (playing && ImGui::Button("stop")) playing = false;

	ImGui::SameLine();

	bool step = ImGui::Button("step") || playing && timeout == 0;
	
	if (playing && timeout > 0) timeout -= 1;


	// logic for each state
	const char* st = NULL;
	const char* next_state = current_state;


	st = do_nothing;
	all_states.insert(st);
	// does nothing

	st = "ranked entry - login";
	all_states.insert(st);
	if (current_state == st && step) {
		run_menu_action(4, 0); // log in
		next_state = "ranked entry - open ranked menu";
	}

	st = "ranked entry - open ranked menu";
	all_states.insert(st);
	if (current_state == st && step) {
		run_menu_action(3, 0); // open ranked menu
		next_state = "ranked entry - select entry";
	}

	st = "ranked entry - select entry";
	all_states.insert(st);
	if (current_state == st && step) {
		// this crashes sometimes
		run_ranked_menu_action(1); // entry
		next_state = do_nothing;
	}


	st = "replay theater - open list";
	all_states.insert(st);
	if (current_state == st && step) {
		run_menu_action(7, 0); // open replay theater
		next_state = do_nothing;
	}


	st = "practice - select";
	all_states.insert(st);
	if (current_state == st && step) {
		run_menu_action(0, 1); // select training mode
		next_state = "practice - controller";
		// needs no timeout
	}

	st = "practice - controller";
	all_states.insert(st);
	if (current_state == st && step) {
		write_controller_select_defaults();
		next_state = "practice - controller wait";
	}

	st = "practice - controller wait";
	all_states.insert(st);
	if (current_state == st && step) {
		char* base = GetBbcfBaseAdress();
		//MainMenu1* menu = (MainMenu1*)(base + 0xe8c044); // start of menu data

		// base + 0x8904bc
		if (*g_gameVals.pGameState == 6) {
			next_state = "practice - character";
			timeout = 60; // needs a timeout, who knows how much
		}

		// menu->menu_level
		else if (*(int*)(base + 0xe8c044 + 0x64) != 2 && *(int*)(base + 0xe8c044 + 0x64) != 19)
			next_state = do_nothing; // log error?

		else timeout = 100;
	}

	st = "practice - character";
	all_states.insert(st);
	if (current_state == st && step) {
		write_character_select_defaults();
		next_state = do_nothing;
	}

	current_state = next_state;
}