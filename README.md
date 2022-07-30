# Remote File Transferring

For this project, I've implemented a file transferring protocol on top of TCP/IP and created a client-server application which uses it for
remote file transferring. Specifically, a client may make a request for a directory transfer, in which case the server will respond by
sending each file in said directory, in such a way that the client can replicate the same hierarchy locally. The communication happens over
POSIX sockets and the server is multithreaded, so that each client may be served by a different thread. 

## Details

The server spawns a _communication thread_ for each accepted connection with a client, which serves that client. Furthermore, it maintains
a queue for keeping track of the file transfers that need to be completed, as well as a _worker thread_ pool for handling these transfers.
If at any given point the queue is full, the communication thread blocks until at least one file is transferred. On the other hand, if
at any given point the queue is empty, the worker threads block until a new transfer task arrives. Files are to be transferred atomically
in blocks of a fixed size (in bytes). So, at most one file at a time can be written to a client's socket.

## Protocol

- The client requests a directory transfer by sending a message of the form `<size> <name>`.
- The server responds by sending a message of the form `<nfiles>`, designating the expected number of files.
- After that, the server sends for each file:
  - A message of the form `<filename_size> <filename> <file_size>`, designating the file's metadata.
  - Messages of the form `<payload_size> <payload>`, designating the file's data.
- After the transfer has been completed, the client responds with a message of 1 (arbitrary) byte, designating an ACK response,
  in order to gracefully terminate the transaction and release acquired resources.

Note: all transmitted integers take up 4 bytes, and the transmission order is least-significant-byte-first.

## Usage

Type `make` inside the repo's main directory, in order to produce the server / client executables, located under `server` and `client`,
respectively. The command `make clean` is also provided, if one wishes to clean the directory of all executables and object files, recursively.

### Running the server

```
cd server
./dataServer -p <port> -s <thread_pool_size> -q <queue_size> -b <block_size>
```

### Running the client

```
cd client
./remoteClient -i <server_ip> -p <server_port> -d <directory>
```

#### Notes

- The option `<thread_pool_size>` designates the number of worker threads to be used.
- The server starts looking at `server/test_files/`. So, if `<directory>` is `.`, then `test_files/` will be transferred.

### Testing

A simple script has been provided to test the application with three clients that run concurrently:

```
cd client
./run_clients.sh <ip_address> <port>
```

This script logs the client's outputs under the directory `client/log_files`, which it creates upon execution. The transferred files will be
located under `client/`

### Assumptions

- The client knows the server's file system hierarchy.
- The server doesn't contain any empty directory.
- The server will be running 24/7, so it can only be terminated forcefully (CTRL-C or kill).

## To Do

- Abstract away code that does byte-level manipulation (masks etc), because it's visual noise.


