#pragma once

#include "ToolUI.h"
#include "../Renderers/Hardware/PerformanceStats.h"

class PerformanceStatsUI : public ToolUI
{

	PerformanceStats* performanceStats;

public:

	void SetPerformanceStats(PerformanceStats* stats)
	{
		performanceStats = stats;
	}

	void DoInterface() override
	{
		ImGui::Begin("Performance Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("Frame Time: %.2f ms", performanceStats->GetPerformanceMeasurement("Frame")->GetPerformanceMeasurementInMilliseconds());
		ImGui::Text("FPS: %.1f", performanceStats->GetFPS());
		ImGui::Text("AVG FPS: %.1f", performanceStats->GetAvgFPS());

		ImGui::End();
	}
};