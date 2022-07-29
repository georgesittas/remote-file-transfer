#ifndef SYSCALL_UTILS_H_
#define SYSCALL_UTILS_H_

#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
	#include <sys/types.h>
}

// This is used in order to reduce boilerplate error handling in syscalls.
// Note: appending a ';' does not affect the program -- it's a null statement.

#define call_or_exit(call, err) \
  if ((call) < 0) { \
    perror(err); \
    exit(EXIT_FAILURE); \
  }

// This is used in order to reduce boilerplate error handling in pthread calls.
// Note: same as above.

#define pthread_call_or_exit(status, err) \
  if ((status) != 0) { \
    fprintf(stderr, "%s: %s\n", (err), strerror( (status) )); \
    exit(EXIT_FAILURE); \
  }

// Wrapper around the 'write' system call. In case of signal interruption,
// the writing continues from where it was stopped. In case of error, it
// returns -1, otherwise it returns 'nbytes', signalling success.

ssize_t write_(int fd, const char* buf, size_t nbytes);

#endif // SYSCALL_UTILS_H_
