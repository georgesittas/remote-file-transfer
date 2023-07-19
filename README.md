# Remote File Transfer

This is a client-server application that uses a file transferring protocol on top of TCP/IP for Remote File Transfer. When a
client requests a directory transfer, the server responds by transmitting each file in the target directory, until it's been
copied to the client's file system. The communication happens over POSIX sockets and the server uses threads for handling
multiple clients concurrently.

## Usage

```bash
# Compile the project
make

# Cleanup
make clean
```

### Running the server

```bash
cd server
./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>
```

### Running the client

```bash
cd client
./remoteClient -i <server_ip> -p <server_port> -d <directory>
```

#### Notes

- The parameter `<thread_pool_size>` sets the number of worker threads to be used.
- The server treats `server/test_files` as its current working directory for tranfers.

### Testing

```bash
cd client
./run_clients.sh <ip_address> <port>
```

This script tests the application using three clients and logs their outputs under the directory `client/log_files`.
The transferred files will be located under the `client` directory.

## Protocol

- The client begins by requesting a directory transfer: `<name_size> <name>`.
- The server responds with `<number_of_files>`.
- Then, for each file it sends `<filename_size> <filename> <file_size> <payload_size> <payload>`.
- After the transfer has been completed, the client sends an arbitrary byte value, representing
  an ACK response, and the transaction is gracefully terminated.

All transmitted integers take up 4 bytes, and the byte order is Little Endian.

## Architecture

The server spawns a _communication thread_ to serve each accepted connection with a client. Furthermore, it maintains a
queue for keeping track of the file transfers that need to be completed, as well as a _worker thread_ pool for processing
these transfers. If at any given point the queue is full, the communication thread blocks until at least one file is transferred.
On the other hand, if at any given point the queue is empty, the worker threads block until a new transfer task arrives. Files
are transferred atomically in blocks of a fixed size, so at most one file at a time can be written to a client's socket.

## Assumptions

- The client knows the server's file system hierarchy.
- The server doesn't contain any empty directory.
- The server will be running 24/7, so it can only be terminated forcefully (CTRL-C or kill).

## TODO

- [ ] Refactor code that does byte-level manipulation (masks etc) to reduce visual noise.
