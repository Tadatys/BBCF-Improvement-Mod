#pragma once
#include "Game/CharData.h"
#include <vector>
#include <string>

struct TimelineTick {
	int frame = 0;
	std::string log;
};

#define MAX_TICKS 10000 // should be enough for a 99s round

class Timeline {

public:
	int tick_count;
	TimelineTick ticks[MAX_TICKS];

	Timeline();

	void update();
	void rewind(int frame0);
	void clear();

	void write_tick(TimelineTick& t);
	std::string print_char_data(CharData* p);

	bool enabled = false;
};

extern Timeline g_timeline;



struct InputBufferButton {
	int8_t down;
	int8_t hit; // active on the first frame of 'down'
	int8_t release; // active on the first frame of 'up'
	int8_t hit_5f; // active for 5 frames, starting from 'hit'
	int8_t up;
	char pad_05[4];
};

struct InputBufferDirection {
	int8_t down, similar_down; // e.g. for dir=1, similar_down is active for directions 1,2,4
	char pad_02[6];
	int8_t up, similar_up;
	char pad_0A[3];
};

struct InputBuffer {
	InputBufferButton button[6]; // A,B,C,D, taunt and 1 more
	InputBufferDirection dir[9]; // numpad directions 1,2,3,4,5,6,7,8,9, if looking from p1 side
};


// at base + e19660
struct MatchInfo {
	int rounds_total, round, wins_p1, wins_p2;
	int time_total, time_in_frames, time;
	char pad_1C[16];
	int state; // 0x2C, enum MatchState_
	// ... ?
};
