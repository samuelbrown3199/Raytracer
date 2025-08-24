#include "PerformanceStats.h"

void PerformanceStats::UpdatePerformanceStats()
{
	std::lock_guard<std::mutex> lock(m_performanceMutex);
		
	double frameMilliseconds = m_mPerformanceMeasurements["Frame"].GetPerformanceMeasurementInMicroSeconds() / 1000;
	m_dFPS = 1000.0f / frameMilliseconds;
	m_dDeltaT = 1.0f / m_dFPS;
	m_vFramerateList.push_back(m_dFPS);
	m_iCurrentFrameCount++;
	m_iTotalFrames++;
	m_fEngineUpTime += m_dDeltaT;

	if(m_dAvgFPS == 0)
		m_dAvgFPS = m_dFPS;

	if (m_iAvgFrameRateCount == m_iCurrentFrameCount)
	{
		m_dAvgFPS = 0;
		for (int i = 0; i < m_vFramerateList.size(); i++)
		{
			m_dAvgFPS += m_vFramerateList.at(i);
		}
		m_vFramerateList.clear();
		m_dAvgFPS /= m_iAvgFrameRateCount;
		m_iCurrentFrameCount = 0;
	}
}

PerformanceStats::PerformanceStats()
{
	AddPerformanceMeasurement("Frame");
	m_iCurrentFrameCount = 0;
}

void PerformanceStats::AddPerformanceMeasurement(std::string name)
{
	std::lock_guard<std::mutex> lock(m_performanceMutex);

	if (m_mPerformanceMeasurements.count(name) != 0)
		return;

	m_mPerformanceMeasurements[name] = PerformanceMeasurement();
}

void PerformanceStats::StartPerformanceMeasurement(std::string name)
{
	std::lock_guard<std::mutex> lock(m_performanceMutex);
	m_mPerformanceMeasurements[name].StartMeasurement();
}

void PerformanceStats::EndPerformanceMeasurement(std::string name)
{
	std::lock_guard<std::mutex> lock(m_performanceMutex);
	m_mPerformanceMeasurements[name].EndMeasurement();
}

PerformanceMeasurement* PerformanceStats::GetPerformanceMeasurement(std::string name)
{
	std::lock_guard<std::mutex> lock(m_performanceMutex);
	return &m_mPerformanceMeasurements[name];
}

AveragePerformanceMeasurement* PerformanceStats::GetAveragePerformanceMeasurement(std::string name)
{
	if (m_mAveragePerformanceMeasurements.find(name) == m_mAveragePerformanceMeasurements.end())
	{
		m_mAveragePerformanceMeasurements[name] = AveragePerformanceMeasurement();
	}
	return &m_mAveragePerformanceMeasurements[name];
}