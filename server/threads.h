#ifndef THREADS_H_
#define THREADS_H_

#include <map>
#include <queue>
#include <string>

extern "C" {
	#include <pthread.h>
}

// Client will request files in a directory relative to this path.
#define STARTDIR "./test_files/"

struct Task {
	int fd; // Socket file descriptor
	std::string name; // Name of file to be processed

	Task(int _fd, std::string _name) : fd(_fd), name(_name) { }
};

struct SharedData {
	int block_size; // Files are transmitted in blocks of this size
	int task_capacity; // Maximum number of available tasks in queue
	std::queue<Task> tasks;

	pthread_mutex_t log_mutex; // Protects writing to std::cerr (for logging)
	pthread_mutex_t queue_mutex; // Protects access to the task queue
	pthread_cond_t cond_nonfull; // Condition needed for communication thread
	pthread_cond_t cond_nonempty; // Condition needed for worker thread

	std::map<int, pthread_mutex_t*> fd_to_mutex; // Protects writing to a socket fd
};

extern SharedData data;

// Starting points for worker and communication threads, respectively.
void* worker_thread(void* arg);
void* communication_thread(void* arg);

#endif // THREADS_H_
