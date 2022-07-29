#include "syscall_utils.h"

#include <iostream>

extern "C" {
	#include <unistd.h>
	#include <sys/types.h>
}

ssize_t write_(int fd, const char* buf, size_t nbytes) {
	ssize_t nwritten;
	for (size_t to_write = nbytes; to_write > 0; ) {
		if ((nwritten = write(fd, buf, to_write)) < 0) {
			if (errno == EINTR) {
				continue;
			}

			return -1;
		}

		buf += nwritten;
		to_write -= nwritten;
	}

	return nbytes;
}
