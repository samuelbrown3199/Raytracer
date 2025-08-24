#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>

#include "../../Useful/Useful.h"

struct PerformanceMeasurement
{
	std::chrono::time_point<std::chrono::high_resolution_clock> m_measurementStart;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_measurementEnd;
	std::chrono::duration<double, std::micro> m_measurementTime;

	void StartMeasurement()
	{
		m_measurementStart = std::chrono::high_resolution_clock::now();
	}

	void EndMeasurement()
	{
		m_measurementEnd = std::chrono::high_resolution_clock::now();
		m_measurementTime = std::chrono::duration_cast<std::chrono::microseconds>(m_measurementEnd - m_measurementStart);
	}

	double GetPerformanceMeasurementInMicroSeconds() const
	{
		double returnVal = m_measurementTime.count();
		return returnVal;
	}

	double GetPerformanceMeasurementInMilliseconds() const
	{
		double returnVal = m_measurementTime.count() / 1000.0;
		return returnVal;
	}
};

struct AveragePerformanceMeasurement
{
	double m_dTotalTime = 0;
	int m_iCount = 0;

	AveragePerformanceMeasurement() {}

	void AddMeasurement(double microseconds)
	{
		double time = microseconds / 1000.0; // Convert to milliseconds

		m_dTotalTime += time;
		m_iCount++;
	}
	double GetAverageTime() const
	{
		if (m_iCount == 0) return 0.0;
		return m_dTotalTime / m_iCount;
	}
};

/**
*Stores engine performance stats.
*/
struct PerformanceStats
{
	friend class EngineManager;

private:

	const int m_iAvgFrameRateCount = 250;
	const int m_iSecondsPerLog = 10;
	std::vector<double> m_vFramerateList;
	int m_iCurrentFrameCount = 0;
	int m_iLastLogTime = 0;

	int m_iTotalFrames = 0;

	std::mutex m_performanceMutex;

	std::map<std::string, PerformanceMeasurement> m_mPerformanceMeasurements;
	std::map<std::string, AveragePerformanceMeasurement> m_mAveragePerformanceMeasurements;

	double m_dDeltaT = 0;
	double m_dFPS = 0;
	double m_dAvgFPS = 0;
	double m_fEngineUpTime = 0;

public:

	PerformanceStats();

	/**
	* Adds a performance measurement to the performance stats.
	*/
	void AddPerformanceMeasurement(std::string name);

	/**
	* Starts a performance measurement.
	*/
	void StartPerformanceMeasurement(std::string name);

	/**
	* Ends a performance measurement. This will calculate the time taken for the measurement.
	*/
	void EndPerformanceMeasurement(std::string name);

	/**
	* Gets a performance measurement.
	*/
	PerformanceMeasurement* GetPerformanceMeasurement(std::string name);
	AveragePerformanceMeasurement* GetAveragePerformanceMeasurement(std::string name);

	/**
	* Gets the total number of frames since starting the application.
	*/
	int GetFrameCount() const { return m_iTotalFrames; }

	/**
	* Gets all performance measurements.
	*/
	std::map<std::string, PerformanceMeasurement> GetPerformanceMeasurements() const { return m_mPerformanceMeasurements; }

	std::map<std::string, AveragePerformanceMeasurement> GetAveragePerformanceMeasurements() const { return m_mAveragePerformanceMeasurements; }

	/**
	*Updates the performance stats. Done every frame by the engine.
	*/
	void UpdatePerformanceStats();

	double GetDeltaT() const { return m_dDeltaT; }
	double GetFPS() const { return m_dFPS; }
	double GetAvgFPS() const { return m_dAvgFPS; }
	double GetEngineUpTime() const { return m_fEngineUpTime; }
};