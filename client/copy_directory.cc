#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include <iostream>

extern "C" {
	#include <fcntl.h>
	#include <unistd.h>
	#include <dirent.h>
	#include <sys/wait.h>
	#include <sys/stat.h>
	#include <sys/types.h>
}

#include "reader.h"
#include "syscall_utils.h"

// Directories will be replicated inside this directory by default
#define STARTDIR "./"

static std::string read_filename(Reader& reader) {
	int filename_size = 0;
	std::string filename = "";

	for (int byte, i = 0; i < 4; i++) {
		byte = reader.next();
		filename_size |= byte << (i * 8);
	}

	while (filename_size-- > 0) {
		filename += (char) reader.next();
	}

	return filename;
}

std::string trim_prefix_if_needed(std::string path, std::string& target_directory) {
	return target_directory == "." ? path : path.substr(path.find(target_directory));
}

static int replicate_and_open(std::string& filename) {
	DIR* dp;
	std::string curr_path(STARTDIR);

	for (char c : filename) {
		curr_path += c;

		// For each directory along the filename's path, create it if needed
		if (c == '/') {
			if ((dp = opendir(curr_path.c_str())) == nullptr) {
				if (errno == ENOENT) {
					call_or_exit(mkdir(curr_path.c_str(), 0700), "mkdir (client)");
				} else {
					perror("opendir (client)");
					exit(EXIT_FAILURE);
				}
			} else {
				call_or_exit(closedir(dp), "closedir (client)");
			}
		}
	}

	int fd;
	call_or_exit(
		fd = open(curr_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0600),
		"open file (client)"
	);

	return fd;
}

void copy_directory(Reader& reader, std::string& target_directory) {
	int nfiles = 0; // Number of files contained in the target directory

	for (int byte, i = 0; i < 4; i++) {
		byte = reader.next();
		nfiles |= byte << (i * 8);
	}

	std::cerr << "About to read " << nfiles << " files from the server\n\n";

	while (nfiles-- > 0) {
		int file_size = 0;
		std::string filename;

		filename = trim_prefix_if_needed(read_filename(reader), target_directory);
		int fd = replicate_and_open(filename);

		// Read the file's size
		for (int byte, i = 0; i < 4; i++) {
			byte = reader.next();
			file_size |= byte << (i * 8);
		}

		for (int nread = 0; nread < file_size; ) {
			int payload_size = 0;

			// Read the payload size first
			for (int byte, i = 0; i < 4; i++) {
				byte = reader.next();
				payload_size |= byte << (i * 8);
			}

			// Then, read the actual payload
			std::string payload = "";
			for (int i = 0; i < payload_size; i++) {
				payload += (char) reader.next();
			}

			// Finally, write the payload to the local file and update nread count
			nread += payload_size;
			call_or_exit(write_(fd, payload.c_str(), payload.size()), "write_ file (client)");
		}

		std::cerr << "Received: " << filename << "\n";

		call_or_exit(close(fd), "close file (client)");
	}
}
