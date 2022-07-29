#ifndef READER_H_
#define READER_H_

#include <cstdio>
#include <cstdlib>

extern "C" {
	#include <unistd.h>
	#include <sys/types.h>
}

#define BUFSIZE 4096

class Reader {
  public:
  	Reader(int fd) : fd_(fd), eof_(false), pos_(0), lim_(0) {}

  	// Returns the next available character in the stream corresponding to 'fd',
  	// or eof(), if there are no more characters to read.

  	int next() {
  		if (pos_ == lim_) {
  			do {
  				lim_ = read(fd_, buf_, BUFSIZE);
  			} while (lim_ < 0 && errno == EINTR);

  			if (lim_ < 0 && errno != EINTR) {
				perror("read");
				exit(EXIT_FAILURE);
  			} else if (lim_ == 0) {
  				eof_ = true;
  				return -1;
  			}

  			pos_ = 0;
  		}

  		return buf_[pos_++];
  	}

  	bool eof() { return eof_; }

  private:
  	int fd_;
  	unsigned char buf_[BUFSIZE];

  	bool eof_;

  	ssize_t pos_;
  	ssize_t lim_;
};

#endif // READER_H_
