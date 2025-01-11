#include "GymWindow.h"
#include "Overlay/Widget/FileWidget.h"

#include "Core/utils.h"
#include "Core/interfaces.h"
#include "Game/Playbacks/PlaybackSlot.h"
#include "Game/Playbacks/PlaybackManager.h"
#include "Game/gamestates.h"




void memcpy_range(void* dst, void* src, int from, int to) {
	memcpy((char*)dst + from, (char*)src + from, to - from);
}

// memcpy_CharData
void copy_CharData(CharData* dst, CharData* src) {
	// copy the parts of CharData that don't cause a crash. skipping all pointers and most pads
	memcpy_range(dst, src, offsetof(CharData, SLOT_31), offsetof(CharData, pad_01B0));
	memcpy_range(dst, src, offsetof(CharData, overdriveTimeleft), offsetof(CharData, pad_01E4));
	memcpy_range(dst, src, offsetof(CharData, pad_0250), offsetof(CharData, pad_0338));
	
	// bad
	//memcpy_range(dst, src, offsetof(CharData, hurtboxCount), offsetof(CharData, ownerEntity));
	//memcpy_range(dst, src, offsetof(CharData, pad_0250), offsetof(CharData, pad_0338));

	memcpy_range(dst, src, offsetof(CharData, previousHP), offsetof(CharData, pad_09DC));
	memcpy_range(dst, src, offsetof(CharData, frameCounterCurrentSprite), offsetof(CharData, pad_1340));
	memcpy_range(dst, src, offsetof(CharData, lastAction), offsetof(CharData, pad_2090));
	memcpy_range(dst, src, offsetof(CharData, hitCount), offsetof(CharData, pad_57A4));
	memcpy_range(dst, src, offsetof(CharData, heatMeter), offsetof(CharData, pad_5B08));
	memcpy_range(dst, src, offsetof(CharData, L_input_buffer_flag_base), offsetof(CharData, pad_1E9B0));
	// TODO: if dst is airborne but src is grounded, we get stuck in a falling animation loop until some action is taken
}



std::string GymPlayback::mk_summary() {
	// returns string that rougly describes the inputs in playback, all in one line
	const char* direction[] = { "1", "2", "3", "4", " ", "6", "7", "8", "9" };
	const char* button[] = { "A", "B", "C", "D" };
	const char button_mask[] = { 0x10, 0x20, 0x40, 0x80 };

	std::string s = "";
	char prev = 5;
	for (char c : buf) {
		if (facing == 1)  // flip direction
			c = c + 2 - (((c & 0xF) - 1) % 3) * 2;

		if ((c & 0xF) != (prev & 0xF) && 1 <= (c & 0xF) && (c & 0xF) <= 9) {
			s += direction[(c & 0xF) - 1];
		}

		for (int j = 0; j < 4; j++) {
			if ((c & button_mask[j]) && (c & button_mask[j]) != (prev & button_mask[j])) {
				s += button[j];
			}
		}

		prev = c;
	}

	return s.substr(0, 100);
}

std::string GymPlayback::to_text() {
	// returns string that exactly describes the inputs in playback, one line for every frame
	const char* direction[] = { "1", "2", "3", "4", "", "6", "7", "8", "9" };
	const char* button[] = { "A", "B", "C", "D" };
	const char button_mask[] = { 0x10, 0x20, 0x40, 0x80 };

	std::string s = "";
	for (char c : buf) {
		if (facing == 1)  // flip direction
			c = c + 2 - (((c & 0xF) - 1) % 3) * 2;

		if (1 <= (c & 0xF) && (c & 0xF) <= 9) {
			s += direction[(c & 0xF) - 1];
		}

		for (int j = 0; j < 4; j++) {
			if ((c & button_mask[j])) {
				s += button[j];
			}
		}

		s += '\n';
	}

	return s;
}

void GymPlayback::from_text(const char* text) {
	// parses the output of to_text()
	std::vector<char> new_buf;
	char b = 5; // next byte to be pushed to buf
	for (int i = 0; text[i] != 0; i++) {
		char c = text[i];
		if ('1' <= c && c <= '9') // interpret any digit as direction
			b = (b & 0xF0) + c - '0';
		if ('A' <= c && c <= 'D') // interpret any letter A-D as button
			b = b | (1 << c - 'A' + 4);
		if ('a' <= c && c <= 'd') // interpret any letter a-d as button
			b = b | (1 << c - 'A' + 4);
			
		if (c == '\n') { // push on newline
			new_buf.push_back(b);
			b = 5;
		}
		// ignore everything else
	}
	if (b != 5) new_buf.push_back(b);

	this->facing = 0;
	this->buf = new_buf;
	this->summary = this->mk_summary();
}


void GymWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
}


void GymWindow::PlaySlot(int i, int start_time) {
	next_slot = i;
	next_slot_start_time = start_time - slots[i].time_buffer;
}

bool GymWindow::PlayRandomSlot(int type, int start_time) { // TODO: cache type->[slot] ?
	int r = 0, ri = -1;
	for (int i = 0; i < slots.size(); i++) {
		int r1 = rand();
		if (slots[i].type == type && (ri == -1 || r1 > r)) {
			r = r1;
			ri = i;
		}
	}
	if (ri == -1) return false;
	PlaySlot(ri, start_time);
	return true;
}

void GymWindow::Draw() {

	const char* slot_type[] = { "Neutral", "OnGap", "OnWakeup", "AA" }; // TODO: OnTech, OnBlock, OnHit, Far, Disabled

	static int slot_edit = 0;
	static char slot_edit_buf[10000] = "";
	static int slot_edit_time_buffer = 0;
	static bool slot_edit_dirty = false;
	


	for (int i = 0; i < slots.size(); i++) {
		GymPlayback& g = slots[i];
		ImGui::TextUnformatted((std::to_string(i + 1) + ". ").c_str());
		ImGui::SameLine();
		ImGui::PushItemWidth(70);
		ImGui::Combo(("##slot_type_" + std::to_string(i)).c_str(), &g.type, slot_type, IM_ARRAYSIZE(slot_type));
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::TextUnformatted((g.summary).c_str());
		ImGui::SameLine();

		if (ImGui::Button(("e##slot_edit_" + std::to_string(i)).c_str())) { // open slot in editor
			slot_edit = i;
			strcpy(slot_edit_buf, g.to_text().c_str()); // TODO: check max length
			slot_edit_time_buffer = g.time_buffer;
			slot_edit_dirty = false;
		}
		ImGui::SameLine();

		if (ImGui::Button(("p##slot_edit_" + std::to_string(i)).c_str())) PlaySlot(i, 0);
	}
	// TODO: how to reorder? need up/down buttons?

	bool sync = ImGui::Button("Sync 1-4##gym_sync");
	ImGui::SameLine();
	bool push = ImGui::Button("Push active slot##gym_push");
	ImGui::SameLine();
	ImGui::TextUnformatted("press these after recording a slot");
	if (sync || slots.size() == 0) { // load slots from training menu
		if (slots.size() < 4) slots.resize(4);

		for (int i = 0; i < 4; i++) {
			PlaybackSlot slot(1 + i);
			std::vector<char> buf = slot.get_slot_buffer(); // TODO: use slot.frame_len_slot_p, etc ?
			int8_t facing = slot.get_facing_direction();

			GymPlayback& g = slots[i];
			g.facing = facing;
			g.buf = buf;
			g.summary = g.mk_summary();

			if (i == slot_edit) { // update editor if it was open
				strcpy(slot_edit_buf, g.to_text().c_str()); // TODO: check max length
				slot_edit_time_buffer = g.time_buffer;
				slot_edit_dirty = false;
			}
		}
	}
	if (push) { // load active slot from training menu into a new row
		int i0 = *PlaybackManager().active_slot_p;
		PlaybackSlot slot(1 + i0);
		std::vector<char> buf = slot.get_slot_buffer(); // TODO: use slot.frame_len_slot_p, etc ?
		int8_t facing = slot.get_facing_direction();

		GymPlayback g;
		g.type = 0;
		g.facing = facing;
		g.buf = buf;
		g.summary = g.mk_summary();

		if (slots.size() == slot_edit) { // update editor if it was open
			strcpy(slot_edit_buf, g.to_text().c_str()); // TODO: check max length
			slot_edit_time_buffer = g.time_buffer;
			slot_edit_dirty = false;
		}

		slots.push_back(g);
	}


	ImGui::Separator();



	// draw editor
	if (0 <= slot_edit && slot_edit < slots.size()) {
		ImGui::Text("Edit slot %d %s", slot_edit + 1, slot_edit_dirty ? "*" : "");

		ImGui::PushItemWidth(ImGui::GetWindowWidth());
		if (ImGui::InputTextMultiline("##slot_edit", slot_edit_buf, 10000, ImVec2(0, 0)))
			slot_edit_dirty = true;

		ImGui::PopItemWidth();


		ImGui::PushItemWidth(100);
		if (ImGui::InputInt("buffer##slot_edit", &slot_edit_time_buffer)) {
			slot_edit_time_buffer = max(-120, min(slot_edit_time_buffer, 120)); // clamp to +-2 seconds
			slot_edit_dirty = true;
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();


		if (ImGui::Button("Save##slot_edit")) {
			GymPlayback& g = slots[slot_edit];
			g.from_text(slot_edit_buf);
			g.time_buffer = slot_edit_time_buffer;
			slot_edit_dirty = false;


			if (slot_edit < 4) { // write slots 1-4 to training menu
				PlaybackSlot slot(1 + slot_edit);
				int facing_direction = g.facing;
				memcpy(&facing_direction, slot.facing_direction_p, 4);
				slot.load_into_slot(g.buf);
			}
		}

		ImGui::SameLine();


		if (ImGui::Button("Close##slot_edit")) {
			slot_edit = -1;
		}

		ImGui::Separator();

		// TODO: Button("Push") that creates a new slot with given playback
	}



	// store a bunch of traing room data in a type-length-value format (4 chars, 4 byte int, N bytes of data)
	static FileWidget gymFile{ "slots/" };

	if (gymFile.DrawLoad("##gym_load", "Load Gym##gym")) {
		FILE* f = fopen(gymFile.FullPath().c_str(), "rb");
		char* base = GetBbcfBaseAdress();

		int slot_i = 0;
		slots.clear();

		while (true) {
			char chunk_header[8];
			int n_read = fread(chunk_header, 8, 1, f);
			if (n_read == 0) break; // EOF
			int chunk_size = *(unsigned long*)(chunk_header + 4);

			if (memcmp(chunk_header, "SLOT", 4) == 0) {
				int8_t facing, type, time_buffer;
				fread(&facing, 1, 1, f);
				fread(&type, 1, 1, f);
				fread(&time_buffer, 1, 1, f);

				std::vector<char> buf(chunk_size - 3);
				fread(buf.data(), buf.size(), 1, f);

				GymPlayback g;
				g.facing = facing;
				g.type = type;
				g.time_buffer = time_buffer;
				g.buf = buf;
				g.summary = g.mk_summary();
				slots.push_back(g);

				if (slot_i == slot_edit) {
					strcpy(slot_edit_buf, g.to_text().c_str()); // TODO: check max length
					slot_edit_time_buffer = g.time_buffer;
					slot_edit_dirty = false;
				}

				if (slot_i < 4) {
					PlaybackSlot slot(1 + slot_i);
					int facing_direction = g.facing;
					memcpy(&facing_direction, slot.facing_direction_p, 4);
					slot.load_into_slot(g.buf);
				}

				slot_i += 1;
			}

			else if (memcmp(chunk_header, "MENU", 4) == 0) {
				// training mode menu state
				int menu_start = 0x902B3C; // 'Player Character' setting
				// = 0x902B7C // 'Hit Ponts' setting
				// = 0x902C3C // 'Active Slot' setting
				// = 0x902C6C // 'Silpheed' setting
				int menu_end = 0x902D4C + 4; // after 'Immovable Object: Lotus' setting
				fread(base + menu_start, chunk_size, 1, f);
			}

			else if (memcmp(chunk_header, "MEN1", 4) == 0) {
				int extra_start = 0x904198; // settings for valk and platinum are elsewhere
				int extra_end = 0x9041A8;
				fread(base + extra_start, chunk_size, 1, f);
			}

			else if (memcmp(chunk_header, "VIEW", 4) == 0) {
				// this sets new camera position. otherwise loading player positions will move them no further than edge of the screen.
				int view_start = 0xE3A9E8;
				int view_end = 0xE3AAF8 + 4; // last fields: float camera_x (-441 to 441), camera_y (100+), camera_scale (1 to 1.2)
				fread(base + view_start, chunk_size, 1, f);
			}

			else if (memcmp(chunk_header, "VMAT", 4) == 0) {
				// this moves the background, otherwise it can end up off screen, after reading VIEW
				char* bg_matrix_ptr = **(char***)(base + 0x6128A4 + 4) + 4;
				int bg_matrix_len = 304;
				fread(bg_matrix_ptr, chunk_size, 1, f);
			}

			else if (memcmp(chunk_header, "P1  ", 4) == 0 && !g_interfaces.player1.IsCharDataNullPtr()) {
				// assert chunk_size == sizeof(CharData)
				CharData* p = g_interfaces.player1.GetData();
				CharData tmp; // TODO: copy p into tmp, so that we don't break anything if chunk_size is too small?
				fread(&tmp, chunk_size, 1, f);
				copy_CharData(p, &tmp);
			}

			else if (memcmp(chunk_header, "P2  ", 4) == 0 && !g_interfaces.player2.IsCharDataNullPtr()) {
				// assert chunk_size == sizeof(CharData)
				CharData* p = g_interfaces.player2.GetData();
				CharData tmp; // TODO: copy p into tmp, so that we don't break anything if chunk_size is too small?
				fread(&tmp, chunk_size, 1, f);
				copy_CharData(p, &tmp);
			}

			else {
				fseek(f, chunk_size, SEEK_CUR);
			}
		}

		fclose(f);
	}

	if (gymFile.DrawSave("##gym_save", "Save Gym##gym", "Overwrite##gym")) {
		FILE* f = fopen(gymFile.FullPath().c_str(), "wb");
		char* base = GetBbcfBaseAdress();

		/*for (int i = 0; i < 4; i++) { // each SLOT chunk data is 1 byte facing, (len-1) bytes of inputs
			PlaybackSlot slot(1+i);
			std::vector<char> buf = slot.get_slot_buffer(); // TODO: use slot.frame_len_slot_p, etc ?
			int8_t facing = slot.get_facing_direction();
			*/
		for (auto& g : slots) { // each SLOT chunk data is 1 byte facing, 1 byte type, (len-2) bytes of inputs
			fwrite("SLOT", 4, 1, f);
			int32_t chunk_size = g.buf.size() + 3;
			fwrite(&chunk_size, 4, 1, f);

			int8_t facing = g.facing, type = g.type, time_buffer = g.time_buffer;
			fwrite(&facing, 1, 1, f);
			fwrite(&type, 1, 1, f);
			fwrite(&time_buffer, 1, 1, f);
			fwrite(g.buf.data(), g.buf.size(), 1, f);
		}

		{
			fwrite("MENU", 4, 1, f);
			int menu_start = 0x902B3C; // 'Player Character' setting
			// = 0x902B7C // 'Hit Ponts' setting
			// = 0x902C3C // 'Active Slot' setting
			// = 0x902C6C // 'Silpheed' setting
			int menu_end = 0x902D4C + 4; // after 'Immovable Object: Lotus' setting
			int32_t chunk_size = menu_end - menu_start;
			fwrite(&chunk_size, 4, 1, f);
			fwrite(base + menu_start, menu_end - menu_start, 1, f);
		}
		{
			fwrite("MEN1", 4, 1, f);
			int extra_start = 0x904198; // settings for valk and platinum are elsewhere
			int extra_end = 0x9041A8;
			int32_t chunk_size = extra_end - extra_start;
			fwrite(&chunk_size, 4, 1, f);
			fwrite(base + extra_start, extra_end - extra_start, 1, f);
		}

		{
			fwrite("VIEW", 4, 1, f);
			int view_start = 0xE3A9E8;
			int view_end = 0xE3AAF8 + 4; // last fields: float camera_x (-441 to 441), camera_y (100+), camera_scale (1 to 1.2)

			int32_t chunk_size = view_end - view_start;
			fwrite(&chunk_size, 4, 1, f);
			fwrite(base + view_start, view_end - view_start, 1, f);
		}
		{
			fwrite("VMAT", 4, 1, f);
			char* bg_matrix_ptr = **(char***)(base + 0x6128A4 + 4) + 4;
			int bg_matrix_len = 304;

			int32_t chunk_size = bg_matrix_len;
			fwrite(&chunk_size, 4, 1, f);
			fwrite(bg_matrix_ptr, bg_matrix_len, 1, f);
		}

		if (!g_interfaces.player1.IsCharDataNullPtr()) {
			fwrite("P1  ", 4, 1, f);
			int32_t chunk_size = sizeof(CharData);
			fwrite(&chunk_size, 4, 1, f);
			fwrite(g_interfaces.player1.GetData(), sizeof(CharData), 1, f);
		}

		if (!g_interfaces.player2.IsCharDataNullPtr()) {
			fwrite("P2  ", 4, 1, f);
			int32_t chunk_size = sizeof(CharData);
			fwrite(&chunk_size, 4, 1, f);
			fwrite(g_interfaces.player2.GetData(), sizeof(CharData), 1, f);
		}

		fclose(f);
	}

	ImGui::Separator();

	



	// count hits
	static int prev_hitbox_2 = 0;
	static int prev_hitstun_1 = 0;
	static int prev_blockstun_1 = 0;
	static int atk_count_2 = 0;
	static int hit_count_1 = 0;
	static int block_count_1 = 0;

	if (!g_interfaces.player1.IsCharDataNullPtr() && !g_interfaces.player2.IsCharDataNullPtr()) {
		CharData* p1 = g_interfaces.player1.GetData();
		CharData* p2 = g_interfaces.player2.GetData();

		if (p2->hitboxCount > 0 && prev_hitbox_2 == 0) atk_count_2 += 1;
		prev_hitbox_2 = p2->hitboxCount > 0;

		if (p1->hitstun > 0 && prev_hitstun_1 == 0) hit_count_1 += 1;
		prev_hitstun_1 = p1->hitstun > 0;

		if (p1->blockstun > prev_blockstun_1) block_count_1 += 1;
		prev_blockstun_1 = p1->blockstun;

		ImGui::Text("Defence stats: %d attacks, %d blocks, %d times hit. score %f",
			atk_count_2, block_count_1, hit_count_1,
			block_count_1 + hit_count_1 == 0 ? 0 : block_count_1 * 100.0f / (block_count_1 + hit_count_1));
		if (ImGui::Button("Clear##hit_stats")) {
			prev_hitbox_2 = 0;
			prev_hitstun_1 = 0;
			prev_blockstun_1 = 0;
			atk_count_2 = 0;
			hit_count_1 = 0;
			block_count_1 = 0;
		}

		// TODO: count playback loops, and max 1 hit per loop, maybe
	}





	if (g_interfaces.player1.IsCharDataNullPtr() || g_interfaces.player2.IsCharDataNullPtr()) return;

	CharData* p1 = g_interfaces.player1.GetData();
	CharData* p2 = g_interfaces.player2.GetData();




	// manage playback state

	int time = *g_gameVals.pFrameCount;

	ImGui::Checkbox("loop playback", (bool*)&loop_playback);

	// wait for current playback to finish (estimated time)
	if (time > current_slot_end_time) {
		current_slot = -1;
	}

	// run conditional playbacks
	if (current_slot == -1) { // don't interrupt a running playback
		bool is_new_action = p2->actionTime == 1;

		// loop: current_action == "CmnActStand" || "CmnActCrouch" || "_NEUTRAL"
		if (loop_playback && (strcmp(p2->currentAction, "CmnActStand") == 0 || strcmp(p2->currentAction, "CmnActCrouch") == 0 || strcmp(p2->currentAction, "_NEUTRAL") == 0)) {
			if (!PlayRandomSlot(0, time + 0)) { // TODO: use scrState->frames, to avoid 1 idle frame between loops?
				loop_playback = false; // if there are no playbacks, can't loop
			}
		}

		// gap: g_interfaces.player2.GetData()->blockstun == 1 && current_action.find("Guard") != std::string::npos
		if (is_new_action && p2->blockstun > 0 && strstr(p2->currentAction, "Guard") != NULL)
			PlayRandomSlot(1, time + p2->blockstun);

		// wakeup: when entering one of {"ActUkemiLandN",30 } , {"ActUkemiLandF",30 }, {"ActUkemiLandB",30 }, {"ActFDown2Stand", 14}, {"ActBDown2Stand", 14}, wait for that many frames before playing
		if (is_new_action && (strcmp(p2->currentAction, "CmnActUkemiLandN") == 0 || strcmp(p2->currentAction, "CmnActUkemiLandF") == 0 || strcmp(p2->currentAction, "CmnActUkemiLandB") == 0))
			PlayRandomSlot(2, time + 30);
		
		if (is_new_action && (strcmp(p2->currentAction, "CmnActFDown2Stand") == 0 || strcmp(p2->currentAction, "CmnActBDown2Stand") == 0))
			PlayRandomSlot(2, time + 14);

		// AA: find p1 CmnActJumpPre
		if (p1->actionTime == 1 && strcmp(p1->currentAction, "CmnActJumpPre") == 0)
			PlayRandomSlot(3, time + 35); // rough estimate of jump duration

		// TODO:
		// onblock: current_action.find("GuardLoop") != std::string::npos
		// onhit: g_interfaces.player2.GetData()->hitstun > 0 && find any of { "CmnActHit", "CmnActBDown", "CmnActFDown", "CmnActVDown", "CmnActStaggerLoop", "CmnActSlideAir" , "CmnActSkeleton", "CmnActBlowoff"}
		// auto tech: g_interfaces.player2.GetData()->timeAfterTechIsPerformed == 29 && current_action.find("LockReject") != std::string::npos
	}

	// wait for next_slot_delay frames, then start playback
	if (time > next_slot_start_time && 0 <= next_slot && next_slot < slots.size()) {
		GymPlayback& g = slots[next_slot];

		PlaybackManager playback_manager;
		playback_manager.load_into_slot(g.buf, g.facing != p2->facingLeft, next_slot < 4 ? next_slot+1 : 1); // use existing slots if possible
		playback_manager.set_active_slot(next_slot < 4 ? next_slot + 1 : 1); // but otherwise overwrite slot 1
		playback_manager.set_playback_type(0); //forces playback type to be "normal" instead of "random"
		playback_manager.set_playback_position(0); //makes sure the playback is in frame zero
		playback_manager.set_playback_control(3); //activates the playback

		current_slot = next_slot;
		current_slot_end_time = time + g.buf.size();
		next_slot = -1;
	}

}