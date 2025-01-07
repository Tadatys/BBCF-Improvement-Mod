#pragma once
#include "IWindow.h"
#include "Game/Timeline/Timeline.h"

class TimelineWindow : public IWindow
{
public:
	TimelineWindow(const std::string& windowTitle, bool windowClosable, ImGuiWindowFlags windowFlags = 0)
		: IWindow(windowTitle, windowClosable, windowFlags) {
	}
	~TimelineWindow() override = default;
protected:
	void BeforeDraw() override;
	void Draw() override;
private:
	ImGuiTextBuffer m_buffer;
	float           m_prevScrollMaxY = 0;
	int	            m_maxLines = 600;
};
