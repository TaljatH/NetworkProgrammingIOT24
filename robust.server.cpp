#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>

// std
using namespace std;

// config
const int PORT = 8080;
const int MAX_CLIENTS = 100;
const int BUFFER_SIZE = 4096;
const int THREAD_POOL_SIZE = 4;
const int CLIENT_TIMEOUT_SEC = 60;

// task
struct Task {
    int client_fd;
    string data;
};

// queue
queue<Task> taskQueue;
mutex queueMutex;
condition_variable taskCond;

// state
struct ClientState {
    string buffer;
    chrono::steady_clock::time_point lastActive;
};

unordered_map<int, ClientState> clientStates;
mutex clientMapMutex;

// helper
int makeSocketNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// close
void closeClient(int fd, fd_set& master_set) {
    close(fd);
    FD_CLR(fd, &master_set);
    lock_guard<mutex> lock(clientMapMutex);
    clientStates.erase(fd);
    cout << "Closed client " << fd << "\n";
}

// worker
void workerThread() {
    while (true) {
        Task task;

        {
            unique_lock<mutex> lock(queueMutex);
            taskCond.wait(lock, [] { return !taskQueue.empty(); });
            task = taskQueue.front();
            taskQueue.pop();
        }

        string response = "Echo: " + task.data;

        // send
        size_t totalSent = 0;
        while (totalSent < response.size()) {
            ssize_t sent = send(task.client_fd, response.c_str() + totalSent, response.size() - totalSent, 0);
            if (sent <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    this_thread::sleep_for(chrono::milliseconds(10));
                    continue;
                }
                break;
            }
            totalSent += sent;
        }
    }
}

// main
int main() {
    signal(SIGPIPE, SIG_IGN);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen");
        return 1;
    }

    makeSocketNonBlocking(listen_fd);

    fd_set master_set, read_set;
    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);
    int max_fd = listen_fd;

    cout << "Server started on 127.0.0.1:" << PORT << "\n";

    // pool
    vector<thread> workers;
    for (int i = 0; i < THREAD_POOL_SIZE; ++i)
        workers.emplace_back(workerThread);

    // loop
    while (true) {
        read_set = master_set;

        timeval timeout = {1, 0};
        int activity = select(max_fd + 1, &read_set, nullptr, nullptr, &timeout);
        if (activity < 0 && errno != EINTR) {
            perror("select");
            continue;
        }

        // timeout
        auto now = chrono::steady_clock::now();
        {
            lock_guard<mutex> lock(clientMapMutex);
            for (auto it = clientStates.begin(); it != clientStates.end();) {
                if (chrono::duration_cast<chrono::seconds>(now - it->second.lastActive).count() > CLIENT_TIMEOUT_SEC) {
                    cout << "Client " << it->first << " timed out\n";
                    closeClient(it->first, master_set);
                    it = clientStates.erase(it);
                } else {
                    ++it;
                }
            }
        }

        // read
        for (int fd = 0; fd <= max_fd; ++fd) {
            if (!FD_ISSET(fd, &read_set)) continue;

            if (fd == listen_fd) {
                // accept
                sockaddr_in client_addr{};
                socklen_t len = sizeof(client_addr);
                int client_fd;
                while ((client_fd = accept(listen_fd, (sockaddr*)&client_addr, &len)) >= 0) {
                    makeSocketNonBlocking(client_fd);
                    FD_SET(client_fd, &master_set);
                    if (client_fd > max_fd) max_fd = client_fd;
                    {
                        lock_guard<mutex> lock(clientMapMutex);
                        clientStates[client_fd].lastActive = chrono::steady_clock::now();
                    }
                    cout << "Accepted client " << client_fd << "\n";
                }
            } else {
                // recv
                char buffer[BUFFER_SIZE];
                ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    if (bytes == 0 || errno == ECONNRESET) {
                        cout << "Client " << fd << " disconnected\n";
                    } else {
                        perror("recv");
                    }
                    closeClient(fd, master_set);
                    continue;
                }

                lock_guard<mutex> lock(clientMapMutex);
                clientStates[fd].lastActive = chrono::steady_clock::now();
                clientStates[fd].buffer.append(buffer, bytes);

                size_t pos;
                while ((pos = clientStates[fd].buffer.find('\n')) != string::npos) {
                    string message = clientStates[fd].buffer.substr(0, pos + 1);
                    clientStates[fd].buffer.erase(0, pos + 1);

                    if (message.size() > BUFFER_SIZE) {
                        string err = "ERROR: Message too long\n";
                        send(fd, err.c_str(), err.size(), 0);
                        closeClient(fd, master_set);
                        break;
                    }

                    {
                        lock_guard<mutex> qlock(queueMutex);
                        taskQueue.push({fd, message});
                    }
                    taskCond.notify_one();
                }
            }
        }
    }

    for (auto& t : workers) t.join();
    close(listen_fd);
    return 0;
}
