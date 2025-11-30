#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>
#include <iostream>

#include "thread_pool.hpp"

static void* print_task(void* arg)
{
    const char* msg = static_cast<const char*>(arg);
    std::cout << "[task] " << msg << std::endl;
    delete[] msg;
    return nullptr;
}

// Create M FIFO in /tmp and return their paths
static std::vector<std::string> create_fifos(int M)
{
    std::vector<std::string> paths;
    paths.reserve(M);
    for (int i = 0; i < M; ++i)
    {
        std::string path = "/tmp/threadpool_fifo_" + std::to_string(i);

        // remove if exists
        ::unlink(path.c_str());

        if (::mkfifo(path.c_str(), 0666) != 0)
        {
            std::perror("mkfifo");
        }
        paths.push_back(path);
    }
    return paths;
}

// test writer: write N messages to FIFO in a endless loop
static void test_writer_multiplex(int M, int Nmsgs, int delay_ms)
{
    // open write-end for each FIFO
    std::vector<int> write_fds(M, -1);
    for (int i = 0; i < M; ++i)
    {
        std::string path = "/tmp/threadpool_fifo_" + std::to_string(i);
        for (;;)
        {
            int fd = ::open(path.c_str(), O_WRONLY | O_NONBLOCK);
            if (fd >= 0)
            {
                write_fds[i] = fd;
                break;
            }
            // wait and retry
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    for (int k = 0; k < Nmsgs; ++k)
    {
        int i = k % M;
        char msg[128];
        int len = std::snprintf(msg, sizeof(msg), "msg %d to fifo %d", k, i);
        if (write_fds[i] >= 0)
        {
            ssize_t w = ::write(write_fds[i], msg, len);
            (void)w;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }

    for (int i = 0; i < M; ++i)
    {
        if (write_fds[i] >= 0)
            ::close(write_fds[i]);
    }
}

static std::atomic<bool> g_running{true};

static void handle_sigint(int)
{
    g_running.store(false);
}

static void run_master_multiplex(ThreadPool* pool, int M)
{
    auto fifo_paths = create_fifos(M);

    // Open FIFOs in non-blocking mode for reading
    std::vector<int> fds(M, -1);
    for (int i = 0; i < M; ++i)
    {
        int fd = ::open(fifo_paths[i].c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0)
        {
            std::perror("open FIFO for read");
        }
        fds[i] = fd;
    }

    // Keep write-ends open to avoid EOF
    std::vector<int> write_fds(M, -1);
    for (int i = 0; i < M; ++i)
    {
        int wfd = ::open(fifo_paths[i].c_str(), O_WRONLY | O_NONBLOCK);
        write_fds[i] = wfd;
    }

    std::vector<pollfd> pfds(M);
    for (int i = 0; i < M; ++i)
    {
        pfds[i].fd = fds[i];
        pfds[i].events = POLLIN;
        pfds[i].revents = 0;
    }

    std::cout << "Master listening on " << M << " FIFO descriptors. Write to them to enqueue tasks.\n";
    std::cout << "Usage: echo \"hello\" > /tmp/threadpool_fifo_0\n";

    char buf[1024];

    while (g_running.load())
    {
        int ret = ::poll(pfds.data(), pfds.size(), 500 /* ms */);
        if (ret < 0)
        {
            std::perror("poll");
            break;
        }
        else if (ret == 0)
        {
            // timeout: continue
            continue;
        }

        for (int i = 0; i < M; ++i)
        {
            if (pfds[i].revents & POLLIN)
            {
                ssize_t n = ::read(pfds[i].fd, buf, sizeof(buf) - 1);
                if (n > 0)
                {
                    buf[n] = '\0';
                    char* msg = new char[n + 1];
                    if (msg)
                    {
                        std::memcpy(msg, buf, n + 1);
                        Task* t = new Task;
                        t->func = &print_task;
                        t->arg = msg;
                        pool->submit(t);
                    }
                }
                else if (n == 0)
                {
                    // EOF: reopen read-end
                    ::close(pfds[i].fd);
                    int fd = ::open(fifo_paths[i].c_str(), O_RDONLY | O_NONBLOCK);
                    if (fd >= 0)
                    {
                        pfds[i].fd = fd;
                    }
                    else
                    {
                        std::perror("reopen FIFO read-end");
                    }
                }
                else
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        std::perror("read");
                    }
                }
            }
        }
    }

    pool->wait_completion();

    for (int i = 0; i < M; ++i)
    {
        if (pfds[i].fd >= 0)
            ::close(pfds[i].fd);
        if (write_fds[i] >= 0)
            ::close(write_fds[i]);
        ::unlink(fifo_paths[i].c_str());
    }
}

int main(int argc, char** argv)
{
    // Set up Ctrl+C handler for graceful exit from the endless loop
    ::signal(SIGINT, handle_sigint);

    int num_workers = 4;
    int M = 4; // FIFO count
    if (argc < 3) {
        std::cout   << "WARN: default num_workers = 4, num_fifos = 4 are used\n" \
                    << "\tUsage: " << argv[0] << " <num_workers> <num_fifos>\n";
    }
    if (argc >= 2)
        num_workers = std::atoi(argv[1]);
    if (argc >= 3)
        M = std::atoi(argv[2]);

    ThreadPool* pool = new ThreadPool(num_workers);

    // start test writer thread that writes messages to FIFO
    // Start a writer thread (until SIGINT) that generates load
    std::thread writer([M]
    {
        int k = 0;
        while (g_running.load())
        {
            test_writer_multiplex(M, /*Nmsgs*/ 16, /*delay_ms*/ 50);
            ++k;
        }
    });

    // Start the master function (reads and enqueues tasks)
    run_master_multiplex(pool, M);

    if (writer.joinable())
        writer.join();

    delete pool;
    return 0;
}