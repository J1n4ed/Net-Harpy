// Multithread pool processor by https://github.com/Dedulya07
// Source URL: https://github.com/Dedulya07/ThreadPool/tree/main

module;

#include <vector>
#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>

// work with threads
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

// Timer
#include <chrono>
#include <iomanip>
#include <string>
#include <sstream>

export module threadq;

namespace harpy 
{
	class Timer
	{
	public:

		Timer();
		std::string make_checkpoint(const std::string& message = "<none>");

	private:

		class Duration
		{
		public:
			Duration(const std::chrono::duration<double>& d);
			void print(std::basic_iostream<char>& stream) const;
		private:
			std::chrono::hours hours;
			std::chrono::minutes minutes;
			std::chrono::seconds seconds;
			std::chrono::milliseconds milliseconds;
		};

		std::chrono::system_clock::time_point current_checkpoint;
		std::chrono::system_clock::time_point start_time;
		void print_time(std::chrono::system_clock::time_point time_point, std::basic_iostream<char>& stream) const;
	}; // !Timer

	typedef unsigned long long int task_id;

	// abstract task class
	export class Task 
	{
		friend class ThreadPool;
	public:

		enum class TaskStatus
		{
			awaiting,
			completed
		};

		Task(const std::string& _description);

		// method for signaling the pool from the current task
		void send_signal();

		// abstract method that must be implemented by the user,
		// the body of this function must contain the path for solving the current task
		void virtual one_thread_method() = 0;

	protected:

		harpy::Task::TaskStatus status;

		// text description of the task (needed for beautiful logging)
		std::string description;

		// unique task ID
		harpy::task_id id;

		harpy::ThreadPool* thread_pool;

		// thread-running method
		void one_thread_pre_method();
	};

	// simple wrapper over std::thread for keeping track
	// state of each thread
	struct Thread 
	{
		std::thread _thread;
		std::atomic<bool> is_working;
	};

	export class ThreadPool 
	{

		friend void harpy::Task::send_signal();

	public:

		ThreadPool(int count_of_threads);

		// template function for adding a task to the queue
		template <typename TaskChild>
		harpy::task_id add_task(const TaskChild& task) 
		{
			std::lock_guard<std::mutex> lock(task_queue_mutex);
			task_queue.push(std::make_shared<TaskChild>(task));

			// assign a unique id to a new task
			// the minimum value of id is 1
			task_queue.back()->id = ++last_task_id;

			// associate a task with the current pool
			task_queue.back()->thread_pool = this;
			tasks_access.notify_one();

			return last_task_id;
		}

		// waiting for the current task queue to be completely processed or suspended,
		// returns the id of the task that first signaled and 0 otherwise
		harpy::task_id wait_signal();

		// wait for the current task queue to be fully processed,
		// ignoring any pause signals
		void wait();

		// pause processing
		void stop();

		// resumption of processing
		void start();

		// get result by id
		template <typename TaskChild>
		std::shared_ptr<TaskChild> get_result(harpy::task_id id)
		{
			auto elem = completed_tasks.find(id);

			if (elem != completed_tasks.end())
				return std::reinterpret_pointer_cast<TaskChild>(elem->second);
			else
				return nullptr;
		}

		// cleaning completed tasks
		void clear_completed();

		// setting the logging flag
		void set_logger_flag(bool flag);

		~ThreadPool();

	private:

		// mutexes blocking queues for thread-safe access
		std::mutex task_queue_mutex;
		std::mutex completed_tasks_mutex;
		std::mutex signal_queue_mutex;

		// mutex blocking serial output logger
		std::mutex logger_mutex;

		// mutex blocking functions waiting for results (wait* methods)
		std::mutex wait_mutex;

		std::condition_variable tasks_access;
		std::condition_variable wait_access;

		// set of available threads
		std::vector<harpy::Thread*> threads;

		// task queue
		std::queue<std::shared_ptr<Task>> task_queue;
		harpy::task_id last_task_id;

		// array of completed tasks in the form of a hash table
		std::unordered_map<harpy::task_id, std::shared_ptr<Task>> completed_tasks;
		unsigned long long completed_task_count;

		std::queue<task_id> signal_queue;

		// pool stop flag
		std::atomic<bool> stopped;
		// pause flag
		std::atomic<bool> paused;
		std::atomic<bool> ignore_signals;
		// flag to enable logging
		std::atomic<bool> logger_flag;

		harpy::Timer timer;

		// main function that initializes each thread
		void run(harpy::Thread* thread);

		// pause processing with signal emission
		void receive_signal(harpy::task_id id);

		// permission to start the next thread
		bool run_allowed() const;

		// checking the execution of all tasks from the queue
		bool is_comleted() const;

		// checking if at least one thread is busy
		bool is_standby() const;
	};

	struct MassivePart 
	{
		long long int begin;
		long long int size;
	};

	void separate_massive(long long int full_size, long long int part_size, int thread_count, std::vector<MassivePart>& parts);


} // !HARPY NAMESPACE


// METHODS ---------------------------------------------------------------------------------------------------------------------------------------------



harpy::Task::Task(const std::string& _description) 
{
	description = _description;
	id = 0;
	status = harpy::Task::TaskStatus::awaiting;
	thread_pool = nullptr;
}

void harpy::Task::send_signal() 
{
	thread_pool->receive_signal(id);
}

void harpy::Task::one_thread_pre_method() 
{
	one_thread_method();
	status = harpy::Task::TaskStatus::completed;
}

harpy::ThreadPool::ThreadPool(int count_of_threads)
{
	stopped = false;
	paused = true;
	logger_flag = false;
	last_task_id = 0;
	completed_task_count = 0;
	ignore_signals = true;

	for (int i = 0; i < count_of_threads; i++) 
	{
		harpy::Thread* th = new harpy::Thread;
		th->_thread = std::thread{ &ThreadPool::run, this, th };
		th->is_working = false;
		threads.push_back(th);
	}
}

harpy::ThreadPool::~ThreadPool() 
{
	stopped = true;
	tasks_access.notify_all();

	for (auto& thread : threads) 
	{
		thread->_thread.join();
		delete thread;
	}
}

bool harpy::ThreadPool::run_allowed() const 
{
	return (!task_queue.empty() && !paused);
}

void harpy::ThreadPool::run(harpy::Thread* _thread) 
{
	while (!stopped) 
	{
		std::unique_lock<std::mutex> lock(task_queue_mutex);

		// the current thread is in standby mode in case
		// if there are no jobs or the entire pool is suspended
		_thread->is_working = false;
		tasks_access.wait(lock, [this]()->bool { return run_allowed() || stopped; });
		_thread->is_working = true;

		if (run_allowed()) 
		{
			// a thread takes a task from the queue
			auto elem = std::move(task_queue.front());
			task_queue.pop();
			lock.unlock();

			if (logger_flag) 
			{
				std::lock_guard<std::mutex> lg(logger_mutex);
				std::cout << timer.make_checkpoint("Run task " + elem->description) << std::endl;
			}
			// solving task
			elem->one_thread_pre_method();
			if (logger_flag) 
			{
				std::lock_guard<std::mutex> lg(logger_mutex);
				std::cout << timer.make_checkpoint("End task " + elem->description) << std::endl;
			}

			// saving the result
			std::lock_guard<std::mutex> lg(completed_tasks_mutex);
			completed_tasks[elem->id] = elem;
			completed_task_count++;
		}
		// wake up threads that are waiting on the pool (wait* methods)
		wait_access.notify_all();
	}
}

void harpy::ThreadPool::start() 
{
	if (paused) 
	{
		paused = false;
		// give all threads a permission signal to access
		// to the tasks queue
		tasks_access.notify_all();
	}
}

void harpy::ThreadPool::stop()
{
	paused = true;
}

void harpy::ThreadPool::receive_signal(harpy::task_id id) 
{
	std::lock_guard<std::mutex> lock(signal_queue_mutex);
	signal_queue.emplace(id);

	if (!ignore_signals)
		stop();
}

void harpy::ThreadPool::wait() 
{
	std::lock_guard<std::mutex> lock_wait(wait_mutex);

	start();

	std::unique_lock<std::mutex> lock(task_queue_mutex);
	wait_access.wait(lock, [this]()->bool { return is_comleted(); });

	stop();
}

harpy::task_id harpy::ThreadPool::wait_signal() 
{
	std::lock_guard<std::mutex> lock_wait(wait_mutex);

	ignore_signals = false;

	signal_queue_mutex.lock();

	if (signal_queue.empty())
		start();
	else
		stop();

	signal_queue_mutex.unlock();

	std::unique_lock<std::mutex> lock(task_queue_mutex);
	wait_access.wait(lock, [this]()->bool { return is_comleted() || is_standby(); });

	ignore_signals = true;

	// at the moment all tasks by id from
	// signal_queues are considered completed
	std::lock_guard<std::mutex> lock_signals(signal_queue_mutex);

	if (signal_queue.empty())
		return 0;
	else 
	{
		harpy::task_id signal = std::move(signal_queue.front());
		signal_queue.pop();
		return signal;
	}
}

bool harpy::ThreadPool::is_comleted() const
{
	return completed_task_count == last_task_id;
}

bool harpy::ThreadPool::is_standby() const 
{
	if (!paused)
		return false;

	for (const auto& thread : threads)
		if (thread->is_working)
			return false;

	return true;
}

void harpy::ThreadPool::clear_completed() 
{
	std::scoped_lock lock(completed_tasks_mutex, signal_queue_mutex);
	completed_tasks.clear();

	while (!signal_queue.empty())
		signal_queue.pop();
}

void harpy::ThreadPool::set_logger_flag(bool flag) 
{
	logger_flag = flag;
}

void harpy::separate_massive(long long int full_size, long long int part_size, int thread_count, std::vector<MassivePart>& parts) 
{
	parts.clear();

	long long int full_parts = full_size / part_size;
	long long int last_pos = 0;

	if (full_parts > 0) {
		parts.resize(full_parts);
		for (auto& part : parts) {
			part.begin = last_pos;
			part.size = part_size;
			last_pos += part_size;
		}
	}

	// divide the remainder into smaller pieces between all threads
	long long int remains = full_size % part_size;

	if (remains == 0)
		return;

	long long int each_thread = remains / thread_count;

	if (each_thread == 0) 
	{
		parts.push_back(MassivePart{ last_pos, remains });
		return;
	}

	for (int i = 0; i < thread_count; i++) 
	{
		parts.push_back(MassivePart{ last_pos, each_thread });
		last_pos += each_thread;
	}

	if (remains % thread_count > 0)
		parts.push_back(MassivePart{ last_pos, remains % thread_count });
}

harpy::Timer::Timer() 
{
	current_checkpoint = std::chrono::system_clock::now();
	start_time = current_checkpoint;
}

std::string harpy::Timer::make_checkpoint(const std::string& message) 
{
	std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();

	std::stringstream ss;

	ss <<
		"ThreadId: " << std::setw(5) << std::this_thread::get_id() <<
		" ::: Message: " << std::setw(30) << message <<
		" ::: Time: ";

	print_time(now_time, ss);

	ss << " ::: Duration (h:m:s:ms): ";

	Duration(now_time - current_checkpoint).print(ss);

	ss << ", total: ";

	Duration(now_time - start_time).print(ss);

	current_checkpoint = now_time;

	return ss.str();
}

void harpy::Timer::print_time(std::chrono::system_clock::time_point time_point, std::basic_iostream<char>& stream) const 
{
	time_t time = std::chrono::system_clock::to_time_t(time_point);

	//std::unique_ptr<tm> time_tm(new tm);
	tm* time_tm = new tm();

	localtime_s(time_tm, &time);

	stream << std::put_time(time_tm, "%d.%m.%Y-%T");
	delete time_tm;
}

harpy::Timer::Duration::Duration(const std::chrono::duration<double>& d) 
{
	std::chrono::duration<double> duration = d;

	hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;

	minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;

	seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;

	milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}

void harpy::Timer::Duration::print(std::basic_iostream<char>& stream) const 
{
	stream.fill('0');

	stream <<
		std::setw(2) << hours.count() << ":" <<
		std::setw(2) << minutes.count() << ":" <<
		std::setw(2) << seconds.count() << ":" <<
		std::setw(3) << milliseconds.count();

	stream.fill();
}