#pragma once

#include <chrono>

using namespace std::chrono;

class CTimestamp
{
public:
	CTimestamp()
	{
		Update();
	}
	~CTimestamp()
	{

	}

	void Update()
	{
		m_Begin = high_resolution_clock::now();
	}

	//获取当前秒
	double GetElapsedSecond()
	{
		return GetElapsedTimeInMicroSec() * 0.000001;
	}

	//获取毫秒
	double GetElapsedTimeInMilliSec()
	{
		return GetElapsedTimeInMicroSec() * 0.001;
	}

	//获取微秒
	long long GetElapsedTimeInMicroSec()
	{
		auto t = duration_cast<microseconds>(high_resolution_clock::now() - m_Begin).count();
		return t;
	}

private:
	time_point<high_resolution_clock> m_Begin;
};
