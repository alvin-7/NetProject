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

	//��ȡ��ǰ��
	double GetElapsedSecond()
	{
		return GetElapsedTimeInMicroSec() * 0.000001;
	}

	//��ȡ����
	double GetElapsedTimeInMilliSec()
	{
		return GetElapsedTimeInMicroSec() * 0.001;
	}

	//��ȡ΢��
	long long GetElapsedTimeInMicroSec()
	{
		auto t = duration_cast<microseconds>(high_resolution_clock::now() - m_Begin).count();
		return t;
	}

private:
	time_point<high_resolution_clock> m_Begin;
};
