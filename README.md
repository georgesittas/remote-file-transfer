# Remote File Transferring

This is a client-server application, which uses a custom file transferring protocol on top of TCP/IP for Remote File Transfer.
A client can request a directory transfer, in which case the server will respond by sending back each file in the target directory,
until the hierarchy has been fully copied to the client's file system. The communication happens over POSIX sockets and the server
uses threads for serving multiple clients concurrently.

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

- The option `<thread_pool_size>` designates the number of worker threads to be used.
- The server starts looking at `server/test_files`. So, if `<directory>` is `.`, then `test_files` will be transferred.

### Testing

```bash
cd client
./run_clients.sh <ip_address> <port>
```

This script tests the application using three clients and logs their outputs under the directory `client/log_files`.
The transferred files will be located under the `client` directory.

## Protocol

- The client requests a directory transfer by sending a message of the form `<size> <name>`.
- The server responds by sending a message of the form `<nfiles>`, designating the expected number of files.
- For each file, the server sends:
  - A message of the form `<filename_size> <filename> <file_size>`, designating the file's metadata.
  - Messages of the form `<payload_size> <payload>`, designating the file's data.
- After the transfer has been completed, the client responds with a message of 1 (arbitrary) byte, representing
  an ACK response, in order to gracefully terminate the transaction and release the acquired system resources.

Note: all transmitted integers take up 4 bytes, and the transmission order is least-significant-byte-first.

## Architecture

The server spawns a _communication thread_ for each accepted connection with a client, which serves that client. Furthermore, it maintains
a queue for keeping track of the file transfers that need to be completed, as well as a _worker thread_ pool for handling these transfers.
If at any given point the queue is full, the communication thread blocks until at least one file is transferred. On the other hand, if
at any given point the queue is empty, the worker threads block until a new transfer task arrives. Files are to be transferred atomically
in blocks of a fixed size (in bytes). So, at most one file at a time can be written to a client's socket.

## Assumptions

- The client knows the server's file system hierarchy.
- The server doesn't contain any empty directory.
- The server will be running 24/7, so it can only be terminated forcefully (CTRL-C or kill).

## TODO

- [ ] Refactor code that does byte-level manipulation (masks etc) to reduce visual noise.
