#ifndef THREADPOLL_H
#define THREADPOLL_H

#include <pthread.h>
#include <list>
#include <unistd.h>
#include <sys/types.h>

typedef void *(*callback_t)(void *);//定义函数指针类型
void* work(void* arg);

struct task_t
{
	callback_t func;
	void* args;
};

class locker
{

	public:
	    locker()
	    {
	        if( pthread_mutex_init( &m_mutex, NULL ) != 0 )
	        {
	            perror("mutex init error");
	        }
	    }
	    ~locker()
	    {
	        pthread_mutex_destroy( &m_mutex );
	    }
	    bool lock()
	    {
	        return pthread_mutex_lock( &m_mutex ) == 0;
	    }
	    bool unlock()
	    {
	        return pthread_mutex_unlock( &m_mutex ) == 0;
	    }

	private:
	    pthread_mutex_t m_mutex;
};

class ThreadPoll
{
	friend void* work(void* arg);
	private:
		int max_thread_nums;
		int max_tasks;
		pthread_t tid[5];
		std::list<task_t> thread_queue;
		locker mylocker;
	public:
		ThreadPoll(int threadnums = 4,int tasknums = 10000):max_thread_nums(threadnums),max_tasks(tasknums)
		{
			for(int i=0;i<max_thread_nums;i++)
			{
				if(pthread_create(&tid[i],NULL,work,this)!=0){
					perror("pthread_create error");
					exit(1);
				}
				pthread_detach(tid[i]);
			}
		}
		~ThreadPoll(){}
		bool addTask(callback_t func,void* args)
		{
			//lock
			mylocker.lock();
			if(thread_queue.size()>max_tasks){
				//unlock
				mylocker.unlock();
				return false;
			}
			task_t task;
			task.func=func;
			task.args=args;
			thread_queue.push_back(task);
			//unlock;
			mylocker.unlock();
		}
};
void* work(void* arg){
	ThreadPoll* poll = (ThreadPoll*)arg;
	while(1){
		//lock
		poll->mylocker.lock();
		if(poll->thread_queue.empty()){
			//unlock
			poll->mylocker.unlock();
			continue;
		}
		task_t task = poll->thread_queue.front();
		poll->thread_queue.pop_front();
		//unlock
		poll->mylocker.unlock();
		task.func((void*)task.args);
	}
}

#endif