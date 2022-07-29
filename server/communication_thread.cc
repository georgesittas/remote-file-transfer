#include "threads.h"

#include <map>
#include <queue>
#include <string>
#include <vector>
#include <iostream>

extern "C" {
	#include <dirent.h>
	#include <unistd.h>
	#include <pthread.h>
	#include <sys/stat.h>
	#include <sys/types.h>
}

#include "reader.h"
#include "syscall_utils.h"

static std::string read_dirname(Reader& reader) {
	// Read the directory that the client wants to copy. The first 4 bytes
	// are the payload's size in bytes (least significant byte comes first)

	int nbytes = 0;
	for (int byte, i = 0; i < 4; i++) {
		byte = reader.next();
		nbytes |= byte << (i * 8);
	}

	// Create the target directory path as per the client's request
	std::string dirname(STARTDIR);
	for (int i = 0; i < nbytes; i++) {
		dirname += (char) reader.next();
	}

	// If the client selected the default directory, omit the "." in the path
	if (dirname.back() == '.') {
		dirname = std::string(STARTDIR);
	}

	// Add a trailing slash if it's not there (needed for process_directory)
	if (dirname.back() != '/') {
		dirname += '/';
	}

	return dirname;
}

static void process_directory(std::string& dirname, std::vector<std::string>& filenames) {
	DIR* dp = opendir(dirname.c_str());

	if (dp == nullptr) {
		int status = pthread_mutex_lock(&data.log_mutex);
		pthread_call_or_exit(status, "pthread_mutex_lock (log_mutex)");

		std::cerr << "[Thread " << pthread_self()
		          << "]: Failed to open directory: " << dirname << "\n\n";

		status = pthread_mutex_unlock(&data.log_mutex);
		pthread_call_or_exit(status, "pthread_mutex_unlock (log_mutex)");

		return;
	}

	for (struct dirent* direntp; (direntp = readdir(dp)) != nullptr; ) {
		std::string entry_name = direntp->d_name;

		// Avoid current and parent directory entries so as to not create cycles
		if (entry_name != "." && entry_name != "..") {
			entry_name = dirname + entry_name;

			struct stat st_buf;
			call_or_exit(stat(entry_name.c_str(), &st_buf), "stat (communication thread)");

			if (S_ISDIR(st_buf.st_mode)) {
				entry_name += "/";
				process_directory(entry_name, filenames);
			} else {
				filenames.push_back(entry_name);
			}
		}
	}

	call_or_exit(closedir(dp), "closedir (communication thread)");
}

void* communication_thread(void* arg) {
	int fd = *((int *) arg);
	delete (int *) arg;

	Reader reader(fd);
	std::string dirname = read_dirname(reader);

	int status = pthread_mutex_lock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_lock (log_mutex)");

	std::cerr << "[Thread " << pthread_self()
	          << "]: About to scan directory " << dirname << "\n";

	status = pthread_mutex_unlock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_unlock (log_mutex)");

	// Scan the target directory and add all file names in 'filenames'
	std::vector<std::string> filenames; // All filenames under the directory
	process_directory(dirname, filenames);

	// Tell the client know how many files he's about to receive
	std::string msg = "";
	for (int i = 0, n_files = filenames.size(); i < 4; i++) {
		msg += (char) (n_files >> (i * 8)) & 0xFF;
	}

	call_or_exit(write_(fd, msg.c_str(), msg.size()), "write_ (communication thread)");

	// Create a lock for the client's socket
	data.fd_to_mutex[fd] = new pthread_mutex_t();
	pthread_mutex_init(data.fd_to_mutex[fd], nullptr);

	// Create all needed tasks to delegate to the worker threads
	for (std::string filename : filenames) {
		status = pthread_mutex_lock(&data.log_mutex);
		pthread_call_or_exit(status, "pthread_mutex_lock (log_mutex)");

		std::cerr << "[Thread " << pthread_self()
		          << "]: Adding file " + filename + " to the queue...\n";

		status = pthread_mutex_unlock(&data.log_mutex);
		pthread_call_or_exit(status, "pthread_mutex_unlock (log_mutex)");

		// Create new task to add to the task queue, unless it's at max capacity
		status = pthread_mutex_lock(&data.queue_mutex);
		pthread_call_or_exit(status, "pthread_mutex_lock (queue_mutex)");

		while (((int) data.tasks.size()) > data.task_capacity) {
			status = pthread_cond_wait(&data.cond_nonfull, &data.queue_mutex);
			pthread_call_or_exit(status, "pthread_cond_wait (cond_nonfull, queue_mutex)");
		}

		data.tasks.push(Task(fd, filename));

		// Broadcast to worker threads that a new task is available in the task queue
		status = pthread_cond_broadcast(&data.cond_nonempty);
		pthread_call_or_exit(status, "pthread_cond_broadcast (cond_nonempty)");

		status = pthread_mutex_unlock(&data.queue_mutex);
		pthread_call_or_exit(status, "pthread_mutex_unlock (queue_mutex)");
	}

	// Block until one byte is received from the client as an ACK (finished) response
	reader.next();

	status = pthread_mutex_lock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_lock (log_mutex)");

	std::cerr << "[Thread " << pthread_self()
	          << "]: Transaction completed successfully, terminating...\n";

	status = pthread_mutex_unlock(&data.log_mutex);
	pthread_call_or_exit(status, "pthread_mutex_unlock (log_mutex)");

	// Do some cleanup since the transfer has been completed
	status = pthread_mutex_destroy(data.fd_to_mutex[fd]);
	pthread_call_or_exit(status, "pthread_mutex_destroy (socket mutex)");

	delete data.fd_to_mutex[fd];
	data.fd_to_mutex.erase(fd);

	call_or_exit(close(fd), "close (communication thread");

	return nullptr;
}
