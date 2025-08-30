#pragma once

#include "ToolUI.h"
#include "../Renderers/Hardware/PerformanceStats.h"

class PerformanceStatsUI : public ToolUI
{

	PerformanceStats* performanceStats;

	const int maxFrameTimes = 1000;
	std::vector<float> frameTimes;
	std::vector<float> fpsTimes;

public:

	void SetPerformanceStats(PerformanceStats* stats)
	{
		performanceStats = stats;
	}

	void DoInterface() override
	{
		frameTimes.push_back(performanceStats->GetPerformanceMeasurement("Frame")->GetPerformanceMeasurementInMilliseconds());
		if (frameTimes.size() > maxFrameTimes)
			frameTimes.erase(frameTimes.begin());

		fpsTimes.push_back(performanceStats->GetFPS());
		if(fpsTimes.size() > maxFrameTimes)
			fpsTimes.erase(fpsTimes.begin());

		ImGui::Begin("Performance Stats", nullptr, ImGuiWindowFlags_None);

		ImGui::Text("Frame Time: %.2f ms", performanceStats->GetPerformanceMeasurement("Frame")->GetPerformanceMeasurementInMilliseconds());
		ImGui::Text("FPS: %.1f", performanceStats->GetFPS());
		ImGui::Text("AVG FPS: %.1f", performanceStats->GetAvgFPS());

		if (ImPlot::BeginPlot("Frame Times", ImVec2(-1, 150)))
		{
			ImPlot::SetupAxes("Frame", "Time (ms)");
			ImPlot::SetupAxisLimits(ImAxis_X1, 0, maxFrameTimes, ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 50, ImGuiCond_Always);

			// frameTimes is a std::vector<float> containing your frame times
			ImPlot::PlotLine("Frame Time", frameTimes.data(), frameTimes.size());

			ImPlot::EndPlot();
		}

		if(ImPlot::BeginPlot("FPS", ImVec2(-1, 150)))
		{
			ImPlot::SetupAxes("Frame", "FPS");
			ImPlot::SetupAxisLimits(ImAxis_X1, 0, maxFrameTimes, ImGuiCond_Always);
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 200, ImGuiCond_Always);
			// frameTimes is a std::vector<float> containing your frame times
			ImPlot::PlotLine("FPS", fpsTimes.data(), fpsTimes.size());
			ImPlot::EndPlot();
		}

		ImGui::End();
	}
};