#pragma once
#include "Timeline.h"
#include "Core/interfaces.h"
#include "Core/utils.h"
#include "Game/gamestates.h"
#include "Game/ReplayFiles/ReplayFile.h"
#include <ctime>


Timeline::Timeline() {
	tick_count = 0;
}

void Timeline::write_tick(TimelineTick& t) {
	this->ticks[this->tick_count % MAX_TICKS] = t;
	this->tick_count += 1;
}

std::string Timeline::print_char_data(CharData* p) {
	std::string s;

	/*char* input = p->pad_1E79D; // 0xAB - debugging
	for (int i = 0; i < 6*9; i++) {
		if (i % 9 == 0) s += "|";
		s += std::to_string(int(input[i]));
	}
	for (int i = 6 * 9; i < 0xAB; i++) {
		if ((i - 6 * 9) % 13 == 0) s += " |";
		s += std::to_string(int(input[i]));
	}
	return s;*/

	const char* arrows = "1234 6789"; // u8"\u2B0B\u2B07\u2B0A\u2B05 \u2B95\u2B09\u2B06\u2B08" - TODO: unicode arrows need a font
	// e.g. ImGuiIO& io = ImGui::GetIO(); io.Fonts->AddFontFromFileTTF("font.ttf", size_pixels);

	InputBuffer* buffer = (InputBuffer*)p->pad_1E79D;
	int flip = p->facingLeft;
	int dir = 4; // neutral default
	for (int i = 0; i < 9; i++)
		if (buffer->dir[i].down > 0) dir = (flip ? i + 2 - (i % 3) * 2 : i); // flips 1<->3 and so on
	s += arrows[dir];

	const char buttons[7] = "ABCDEF";
	for (int i = 0; i < 6; i++)
		if (buffer->button[i].down > 0) s += buttons[i];
		else s += " ";


	s += " ";

	// abbreviate action
	int state_flags = *(int*)((char*)p + 0x254);
	if (strcmp(p->currentAction, "_NEUTRAL") == 0) {
		if (state_flags & 0x1) s += "JumpDown"; // sometimes _NEUTRAL happens in the air
		else s += "Stand"; // is there any difference from CmnActStand?
	}
	else if (memcmp(p->currentAction, "CmnAct", 6) == 0) s += p->currentAction + 6;
	else if (memcmp(p->currentAction, "NmlAtkAIR", 9) == 0) s = s + "j" + (p->currentAction + 9);
	else if (memcmp(p->currentAction, "NmlAtkAir", 9) == 0) s = s + "j" + (p->currentAction + 9);
	else if (memcmp(p->currentAction, "NmlAtk", 6) == 0) s += p->currentAction + 6;
	else if (memcmp(p->currentAction, "Atk", 3) == 0) s += p->currentAction + 3;
	else s += p->currentAction;



	// flags
	std::string f;

	scrState* st = NULL; // XXX: not using it any more
	//for (int i = 0; i < states.size(); i++)
	//	if (states[i]->name == p->currentAction) st = states[i];



	// attack properties

	if (p->hitboxCount > 0) {
		if (st != NULL) {
			f = " -" + f + (st->hit_low ? "low" : (st->hit_overhead ? "high" : "mid"));
		}
		
		char* guard_flags = ((char*)p + 0x04EA); // int8_t[3] - values in 0-99
		char* atk_flags = ((char*)p + 0x0589); // int8_t[5] - 0 or 1

		if (guard_flags[0] && guard_flags[1] && guard_flags[2]) f += " throw";
		else if (guard_flags[0]) f += " low";
		else if (guard_flags[1]) f += " high";
		else if (guard_flags[2]) f += " mid";
		else f += " all";

		if (atk_flags[0]) f += "H";
		if (atk_flags[1]) f += "B";
		if (atk_flags[2]) f += "F";
		if (atk_flags[3]) f += "P";
		if (atk_flags[4]) f += "T";
	}



	// count children with hitboxes

	int projectile_count = 0;
	// p->extra_child_entities + main_child_entity still don't have all child entities
	for (int i = 0; i < g_gameVals.entityCount; i++)
	{
		CharData* ent = (CharData*)g_gameVals.pEntityList[i];
		if (ent->ownerEntity != p || ent == p) continue;
		const bool isCharacter = i < 2;
		const bool isEntityActive = ent->unknownStatus1 == 1 && ent->pJonbEntryBegin;
		if (!(isCharacter || isEntityActive)) continue;

		if (ent->hitboxCount > 0)
			projectile_count += 1;
		//s = s + " " + ent->currentAction;  // + std::to_string(ent->hitboxCount);
	}
	if (projectile_count > 0) f += " p" + std::to_string(projectile_count);



	// invuln flags

	if (st != NULL && 1 < p->actionTime && p->actionTime - 1 < st->frame_invuln_status.size()) {
		//const char invuln_flags[] = "";
		//FrameInvuln invuln_masks[] = {};
		int fi = (int)st->frame_invuln_status[p->actionTime - 1];
		if ((int)fi != 0) {
			f += " -i"; // invuln or guard point
			if (fi & (int)FrameInvuln::Head) f += "H";
			if (fi & (int)FrameInvuln::Body) f += "B";
			if (fi & (int)FrameInvuln::Foot) f += "F";
			if (fi & (int)FrameInvuln::Throw) f += "T";

			if ((fi & 0xF) != fi) // any extra bits?
				f = f + std::to_string((int)fi);
		}
	}
	else if (st != NULL)
		f += " -?";

	{
		int inv_flags = *(int*)((char*)p + 0x998);
		int gp_flags = *(int*)((char*)p + 0x99C);

		if (inv_flags & 0x02) {
			if (gp_flags) f += " g";
			else f += " i";

			if (inv_flags & 0x08) f += "H";
			if (inv_flags & 0x10) f += "B";
			if (inv_flags & 0x20) f += "F";
			if (inv_flags & 0x80000) f += "P";
			if (inv_flags & 0x40) f += "T";
		}


		int state_flags = *(int*)((char*)p + 0x254);
		//        0x1 on during jumps, but not juggles?
		//        0x2 always on?
		//      0x100 on for every attack
		//      0x400 forward dash?
		// 0x20000000 turns on for non-ch recovery
		// 0x80000000 holding back?
		
		if ((state_flags & 0x100) && !(state_flags & 0x20000000)) f += " ch";  
		
		//char value[9] = ""; sprintf(value, "%x", state_flags); f = f + " " + value;
	}



	// hitstun
	if (p->hitstop > 0) f += " s" + std::to_string(p->hitstop);
	else {
		if (p->blockstun > 0) f += " b" + std::to_string(p->blockstun);
		if (p->hitstun > 0) f += " h" + std::to_string(p->hitstun);
	}



	// line formatting
	int output_length = 50;
	if (s.size() + f.size() > output_length) s = s.substr(0, output_length - (f.size()));
	s += f;
	if (s.size() < output_length) {
		//s.insert(s.size()-f.size(), (output_length - s.size()) / 2, ' '); // center the flags
		s.insert(s.size() - f.size(), min(output_length - s.size(), 4), ' ');
		s.insert(s.size(), output_length - s.size(), ' ');
	}

	return s;
}

void Timeline::update() {
	
	if (g_interfaces.player1.IsCharDataNullPtr() || g_interfaces.player2.IsCharDataNullPtr())
		return;

	TimelineTick t;
	t.frame = *g_gameVals.pFrameCount;

	int i_prev = max(0, tick_count - 1) % MAX_TICKS;
	if (t.frame == ticks[i_prev].frame) return; // do nothing if game is paused


	// print info about new round
	if (t.frame == 1) {
		write_tick(TimelineTick{ t.frame, "" });

		std::string p1_info = "?", p2_info = "?";

		if (g_interfaces.pRoomManager->IsRoomFunctional() && g_interfaces.pRoomManager->IsThisPlayerInMatch())
		{
			for (const IMPlayer& imPlayer : g_interfaces.pRoomManager->GetIMPlayersInCurrentMatch())
			{
				uint16_t matchPlayerIndex = g_interfaces.pRoomManager->GetPlayerMatchPlayerIndexByRoomMemberIndex(imPlayer.roomMemberIndex);

				if (matchPlayerIndex == 0) p1_info = imPlayer.steamName;
				if (matchPlayerIndex == 1) p2_info = imPlayer.steamName;
				// ignore spectators
			}
		}

		if (*g_gameVals.pGameMode == GameMode_ReplayTheater) 
		{
			char* base = GetBbcfBaseAdress();
			ReplayFile* rp = (ReplayFile*)(base + 0x115B470 + 8);
			
			p1_info = utf16_to_utf8(rp->p1_name);
			p2_info = utf16_to_utf8(rp->p2_name);
		}

		// TODO: in training mode, we could set p2_info = "cpu"|"dummy"|"controller"|...

		p1_info += " (" + getCharacterNameByIndexA(g_interfaces.player1.GetData()->charIndex) + ")";
		p2_info += " (" + getCharacterNameByIndexA(g_interfaces.player2.GetData()->charIndex) + ")";

		char date_str[50];
		time_t timestamp = time(NULL);
		tm datetime = *localtime(&timestamp);
		strftime(date_str, 50, "%Y-%m-%d %H:%M:%S", &datetime);

		MatchInfo* info = (MatchInfo*)g_gameVals.pMatchRounds; // bbcf_base + 0xe19660

		write_tick(TimelineTick{ t.frame, std::string() + "    ---- " + date_str + " ---- ROUND " + std::to_string(info->round) + " ---- " + p1_info + " vs " + p2_info + " ----"});
		write_tick(TimelineTick{ t.frame, "" });

	}
	
	// guess if a rewind happended and deal with it. XXX: it's hard to tell the difference between rewind to frame 0 and start of a new round
	if (t.frame > 1 && tick_count > 0 && ticks[(tick_count - 1) % MAX_TICKS].frame > t.frame)
		rewind(t.frame);

	
	// output sprites in json
	if (true) {
		std::string r = "[";

		char* base = GetBbcfBaseAdress();

		//r += "[\"G\"," // TODO: round metadata, current frame/clock. also character names?

		for (int i = 0; i < g_gameVals.entityCount; i++)
		{
			CharData* ent = (CharData*)g_gameVals.pEntityList[i];
			const bool isEntityActive = ent->unknownStatus1 == 1 && ent->pJonbEntryBegin;
			if (!isEntityActive) continue;

			// logic starting around BBCF.exe+1A00A1:
			char* ent_hip_info = (char*)ent + 0x09e0 + *(int*)((char*)ent + 0x1250) * 108; // p+0x1250 is an index into an array starting from p+0x09e0
			char* ptr = *(char**)(*(char**)(base + 0x623674) + *(int*)(ent_hip_info + 0 * 4 + 0x2C) * 4);
			char* hip_info = *(char**)(*(char**)(ptr + 4) + *(int*)(ent_hip_info + 0 * 4 + 0x4C) * 4);
			//int off_x = *(int*)(hip_info + 0x44), off_y = *(int*)(hip_info + 0x48); // sometimes causes pointer errors, I suspect that 0 * 4 should be 1 * 4 in those cases
			// TODO: transform position_x/y with view/proj matrices, as in HitboxOverlay

			if (r.size() > 1) r += ",";
			r = r + "[\"" + std::to_string((int)ent) + "\"," +
				std::to_string(ent->position_x) + "," + std::to_string(ent->position_y) + "," +
				//std::to_string(off_x) + "," + std::to_string(off_y) + "," +
				std::to_string(ent->facingLeft) + ",\"" + (char*)&ent->currentSprite + "\"]";
		}

		r += "],\n";

		FILE* f = fopen("theaterfile", "a");
		fputs(r.c_str(), f);
		fclose(f);
	}


	// padded frame number
	std::string s = std::to_string(t.frame);
	if (s.size() > 6) s = s.substr(s.size() - 6, 6);
	else s.insert(0, 6 - s.size(), ' ');


	CharData* p1 = g_interfaces.player1.GetData();
	s += " " + print_char_data(p1);

	CharData* p2 = g_interfaces.player2.GetData();
	s += " " + print_char_data(p2);

	t.log = s;
	//t.fullLog = std::string(s); // duplicate data


	std::string prevLog = ticks[i_prev].log;
	if (t.log == prevLog) return; // log nothing if nothing changed

	if (*g_gameVals.pMatchState != MatchState_Fight && prevLog.size() >= 7)
		if (t.log.substr(7) == prevLog.substr(7)) return;

	//if (t.log.substr(7) != prevLog.substr(7)) t.log[0] = '*'; // mark lines with changes -- with hitstun numbers, most lines are different 

	// replace repeating parts of the log with '.    ' -- not that readable
	/*int current_word = 0;
	bool different = false;
	char* a = prevFull.data();
	char* b = t.log.data();
	for (int i = 0; ; i++) {
		if (b[i] == ' ' || b[i] == '\t' || b[i] == 0) {
			if (i - current_word > 0 && !different) {
				b[current_word] = '.'; // modifying string.data() is legal since C++11
				memset(b + current_word + 1, ' ', i - current_word - 1);
			}
			current_word = i + 1;
			different = false;
		}
		if (a[i] == 0 || b[i] == 0) break;
		if (a[i] != b[i]) different = true;
	}*/

	write_tick(t);
}


void Timeline::rewind(int frame0) {
	// Ideally this would be called after loading a snapshot
	while (tick_count > 0 && ticks[(tick_count - 1) % MAX_TICKS].frame > frame0) {
		ticks[(tick_count - 1) % MAX_TICKS] = TimelineTick();
		tick_count -= 1;
	}
}


void Timeline::clear() {
	tick_count = 0;
}

Timeline g_timeline;