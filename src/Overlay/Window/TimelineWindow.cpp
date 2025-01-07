#include "TimelineWindow.h"
#include "Core/utils.h"


void TimelineWindow::BeforeDraw()
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
}

void TimelineWindow::Draw()
{
	// real timeline controls

	ImGui::InputInt("Lines", &m_maxLines);

	if (ImGui::Button("Clear"))
	{
		g_timeline.clear();
	}
	ImGui::SameLine();
	bool copyPressed = ImGui::Button("Copy to clipboard");

	//if (ImGui::Button("Update"))
	//if (g_timeline.enabled) g_timeline.update(); // running update here sometimes skips frames
	//ImGui::SameLine();


	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
	if (copyPressed)
	{
		ImGui::LogToClipboard();
	}

	int first_tick = max(0, (int)g_timeline.tick_count - min(m_maxLines, MAX_TICKS)), last_tick = g_timeline.tick_count;
	for (int i = first_tick; i < last_tick; i++)
	{
		ImGui::TextUnformatted(g_timeline.ticks[i%MAX_TICKS].log.c_str());
	}

	// Handle automatic scrolling
	if (m_prevScrollMaxY < ImGui::GetScrollMaxY())
	{
		// Scroll down automatically only if we didnt scroll up or we closed the window
		if (m_prevScrollMaxY - 5 <= ImGui::GetScrollY())
		{
			ImGui::SetScrollY(ImGui::GetScrollMaxY());
		}
	}
	m_prevScrollMaxY = ImGui::GetScrollMaxY();

	ImGui::EndChild();
}
