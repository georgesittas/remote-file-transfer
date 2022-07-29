#include <string>
#include <cstring>
#include <cstdlib>
#include <iostream>

extern "C" {
	#include <netdb.h>
	#include <unistd.h>
	#include <sys/wait.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
}

#include "reader.h"
#include "cla_parser.h"
#include "syscall_utils.h"

void copy_directory(Reader& reader, std::string& target_directory);

static bool get_args(int argc, char *argv[],
	                 std::string* server_ip, int* port, std::string* directory) {
	ClaParser cla_parser(argc, argv);

	if (!cla_parser.valid_args()) {
		return false;
	}

	*server_ip = cla_parser.get_argument(std::string("-i"));
	std::string port_ = cla_parser.get_argument(std::string("-p"));
	*directory = cla_parser.get_argument(std::string("-d"));

	if (port_.empty() || server_ip->empty() || directory->empty()) {
		return false;
	}

	*port = atoi(port_.c_str());

	return true;
}

int main(int argc, char* argv[]) {
	std::string server_ip;
	int port;
	std::string directory;

	// Process command line arguments
	if (!get_args(argc, argv, &server_ip, &port, &directory)) {
		std::cerr << "Invalid program arguments\n";
		exit(EXIT_FAILURE);
	}

	std::cerr << "\n"
			  << "Client's parameters are:\n\n"
	          << "serverIP: " << server_ip << "\n"
	          << "port: " << port << "\n"
	          << "directory: " << directory << "\n\n";

	// Configure sockets to request data from the server
	int sock;
	call_or_exit(sock = socket(AF_INET, SOCK_STREAM, 0), "socket (client)");

	struct hostent* hostent;
	struct sockaddr_in server;

	if ((hostent = gethostbyname(server_ip.c_str())) == nullptr) {
		herror("gethostbyname");
		exit(EXIT_FAILURE);
	}

	server.sin_family = AF_INET;
	memcpy(&server.sin_addr, hostent->h_addr, hostent->h_length);
	server.sin_port = htons(port);

	std::cerr << "Connecting to " << server_ip << " on port " << port << "...\n";

	call_or_exit(
		connect(sock, (struct sockaddr *) &server, sizeof(server)),
		"connect (client)"
	);

	std::cerr << "Connected succesfully\n\n";

	int dirname_size;
	std::string msg;

	// Transmit request (size + payload) for 'directory' over to the server
	dirname_size = directory.size();
	for (int i = 0; i < 4; i++) {
		msg += (char) (dirname_size >> (i * 8)) & 0xFF;
	}

	msg += directory;
	call_or_exit(write_(sock, msg.c_str(), msg.size()), "write_ (client)");

	// Read and replicate locally the requested directory from the server
	Reader reader(sock);
	copy_directory(reader, directory);

	// Let the server know that the transaction has been completed
	msg = " ";
	call_or_exit(write_(sock, msg.c_str(), msg.size()), "write_ (client)");

	std::cerr << "\n"
	          << "Transfer has been completed. Closing the connection...\n\n";

	call_or_exit(close(sock), "close socket (client)");

	return 0;
}
