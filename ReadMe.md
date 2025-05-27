# Simple Echo TCP Server

## Design Choices

- **select()**: Efficiently handles multiple clients in one main thread using I/O multiplexing.  
- **Non-blocking sockets**: Avoids blocking on slow/unresponsive clients.  
- **Thread pool**: Fixed 4-worker threads process client tasks asynchronously.  
- **Timeouts**: Disconnects clients idle for 30 seconds.  
- **Buffering & Protocol**: Buffers data until newline (`\n`) to form complete messages.  
- **Echo Protocol**: Sends back messages prefixed with "Echo: ".  
- **Robustness**: Limits buffer size, retries partial sends, ignores SIGPIPE, and handles disconnects smoothly.

## Known Limitations

- Maximum buffered message size per client is limited (e.g., 1024 bytes).
- Server only supports simple line-based echo protocol.
- No encryption or authentication implemented.
- Limited to 4 worker threads — not dynamically scalable.

---

## How to Build and Run

### 1. Build the Server

```bash
mkdir build
cd build
cmake ..
make
```

### 2 Create new file called "test.sh"

## 2.1 copy paste this code

```bash
#!/bin/bash
for ((n=1; n<=10; n++)); do
    (echo "test $n" | nc 127.0.0.1 8080) &
    sleep 1
done
```
## 2.2 Run this code on your terminal
```bash
chmod +x test.sh
```



### 3 Run the server
- **3.1 New terminal**: Make a new terminal and type in 
```bash 
cd build
```

- **3.2 Run the server**
```bash
./server
```

## 3.1 Run this code on a new terminal to test the server

```bash
./test.sh
```



