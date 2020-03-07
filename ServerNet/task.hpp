#ifndef _TASK_H_
#include <thread>
#include <mutex>
#include <list>
#include <functional>

class CBaseTask
{
public:
	CBaseTask()
	{}
	
	virtual ~CBaseTask() 
	{
	}

	virtual void doTask()
	{
	}

};

class CServerTask
{
private:
	//��������
	std::list<CBaseTask*> tasks_;
	//�������ݻ�����
	std::list<CBaseTask*> tasksBuf_;
	//�ı����ݻ�����ʹ����
	std::mutex lock_;
	//�߳�
	//std::thread* thread_;
public:
	CServerTask()
	{
	}
	
	~CServerTask()
	{
	}
	
	//�������
	void addTask(CBaseTask* task)
	{
		std::lock_guard<std::mutex> lock(lock_);
		tasksBuf_.push_back(task);
	}

	//��������
	void Start()
	{
		std::thread t(std::mem_fn(&CServerTask::OnRun), this);
		t.detach();
	}

	//��������
	void OnRun()
	{
		while(true)
		{
			if (!tasksBuf_.empty())
			{
				//�ӻ�����ȡ�����ݸ�tasks_
				std::lock_guard<std::mutex> lock(lock_);
				for (auto pTask : tasksBuf_)
				{
					tasks_.push_back(pTask);
				}
				tasksBuf_.clear();
			}
			if (tasks_.empty())
			{
				std::chrono::milliseconds t(1);
				std::this_thread::sleep_for(t);
				continue;
			}
			//��������
			for (auto pTask : tasks_)
			{
				pTask->doTask();
				delete pTask;
			}

		}
	}

};

#endif // !_TASK_H_
