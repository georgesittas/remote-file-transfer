#include "threads.h"

#include <map>
#include <queue>
#include <string>
#include <iostream>

extern "C" {
	#include <fcntl.h>
	#include <pthread.h>
	#include <sys/stat.h>
	#include <sys/types.h>
}

#include "reader.h"
#include "syscall_utils.h"

static void process_task(Task& task) {
	int file_fd;
	call_or_exit(file_fd = open(task.name.c_str(), O_RDONLY), "open file (worker thread)");

	Reader reader(file_fd);

	int status = pthread_mutex_lock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_lock (log_mutex");

	std::cerr << "[Thread " << pthread_self()
	          << "]: About to read file " << task.name << "\n";

	status = pthread_mutex_unlock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_unlock (log_mutex");

	struct stat st_buf;
	call_or_exit(stat(task.name.c_str(), &st_buf), "stat (worker thread)");

	std::string msg;
	int filename_size = task.name.size();

	// Create message: <file name size> <file name> <file size> (4 + n bytes + 4 bytes)
	for (int i = 0; i < 4; i++) {
		msg += (char) (filename_size >> (i * 8)) & 0xFF;
	}

	msg += task.name;

	for (int i = 0, size = st_buf.st_size; i < 4; i++) {
		msg += (char) (size >> (i * 8)) & 0xFF;
	}

	// The following lock is required so that only one file is transmitted at a time
	status = pthread_mutex_lock(data.fd_to_mutex[task.fd]);
	pthread_call_or_exit(status, "pthread_mutex_lock (worker thread: socket fd)");

	call_or_exit(write_(task.fd, msg.c_str(), msg.size()), "write_ (worker thread)");

	// Send file data as messages of the form: <payload size> <payload> (in blocks)
	for (int ch, nread; true; ) {
		msg = "";
		nread = 0;

		while (nread < data.block_size) {
			ch = reader.next();
			if (reader.eof()) {
				break;
			}

			nread++;
			msg += (char) ch;
		}

		if (nread == 0) {
			break;
		}

		std::string msg_size = "";
		for (int i = 0; i < 4; i++) {
			msg_size += (char) (nread >> (i * 8)) & 0xFF;
		}

		msg = msg_size + msg;
		call_or_exit(write_(task.fd, msg.c_str(), msg.size()), "write_ (worker thread)");
	}

	status = pthread_mutex_unlock(data.fd_to_mutex[task.fd]);
	pthread_call_or_exit(status, "pthread_mutex_unlock (worker thread: socket fd)");

	status = pthread_mutex_lock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_lock (log_mutex");

	std::cerr << "[Thread " << pthread_self()
	          << "]: Transferred file " << task.name << " successfully\n";

	status = pthread_mutex_unlock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_unlock (log_mutex");

	call_or_exit(close(file_fd), "close file (worker)");
}

void* worker_thread(void* arg) {
	while (true) {
		int status = pthread_mutex_lock(&data.queue_mutex);
		pthread_call_or_exit(status, "pthread_mutex_lock (queue_mutex)");

		while (data.tasks.empty()) {
			status = pthread_cond_wait(&data.cond_nonempty, &data.queue_mutex);
			pthread_call_or_exit(status, "pthread_cond_wait (cond_nonempty, queue_mutex)");
		}

		Task task = data.tasks.front();
		data.tasks.pop();

		// Broadcast to communication threads that the queue has space for new tasks
		status = pthread_cond_broadcast(&data.cond_nonfull);
		pthread_call_or_exit(status, "pthread_cond_broadcast (cond_nonfull)");

		status = pthread_mutex_unlock(&data.queue_mutex);
		pthread_call_or_exit(status, "pthread_mutex_unlock (queue_mutex)");

		status = pthread_mutex_lock(&data.log_mutex);
		pthread_call_or_exit(status, "pthread_mutex_lock (log_mutex)");

		std::cerr << "[Thread " << pthread_self()
		          << "]: Received task: <" << task.name << ", socket=" << task.fd << ">\n";

		status = pthread_mutex_unlock(&data.log_mutex);
		pthread_call_or_exit(status, "pthread_mutex_unlock (log_mutex)");

		process_task(task);
	}

	return nullptr;
}
