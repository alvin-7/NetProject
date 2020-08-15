#ifndef _TASK_H_
#define _TASK_H_
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
	{}

	virtual void doTask() = 0;

};

class CServerTask
{
private:
	//任务数据
	std::list<CBaseTask*> tasks_;
	//任务数据缓冲区
	std::list<CBaseTask*> tasksBuf_;
	//改变数据缓冲区使用锁
	std::mutex lock_;
	//线程
	//std::thread* thread_;
public:
	CServerTask()
	{
	}
	
	~CServerTask()
	{
	}
	
	//添加任务
	void addTask(CBaseTask* task)
	{
		std::lock_guard<std::mutex> lock(lock_);
		tasksBuf_.push_back(task);
	}

	//启动服务
	void Start()
	{
		std::thread t(std::mem_fn(&CServerTask::OnRun), this);
		t.detach();
	}

protected:
	//工作函数
	void OnRun()
	{
		while(true)
		{
			if (!tasksBuf_.empty())
			{
				//从缓冲区取出数据给tasks_
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
			//处理任务
			for (auto pTask : tasks_)
			{
				pTask->doTask();
				delete pTask;
			}
			tasks_.clear();
		}
	}

};

#endif // !_TASK_H_
