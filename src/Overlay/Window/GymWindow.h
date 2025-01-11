#pragma once
#include "IWindow.h"
#include <vector>

struct GymPlayback {
	int type = 0;
	int facing = 0;
	int time_buffer = 0;
	std::vector<char> buf;

	std::string summary;


	std::string mk_summary();
	std::string to_text();
	void from_text(const char* text);
};

class GymWindow : public IWindow
{
public:
	GymWindow(const std::string& windowTitle, bool windowClosable, ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags) {
	}
	~GymWindow() override = default;
protected:
	void BeforeDraw() override;
	void Draw() override;
private:
	std::vector<GymPlayback> slots;

	int loop_playback = 0;
	int next_slot = -1;
	int next_slot_start_time = 0;
	int current_slot = -1;
	int current_slot_end_time = 0;

	void PlaySlot(int i, int start_time);
	bool PlayRandomSlot(int type, int start_time);
};
