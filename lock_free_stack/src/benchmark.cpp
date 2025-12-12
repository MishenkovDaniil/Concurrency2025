#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

#include "stack.hpp"

using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double, std::milli>;

struct BenchmarkResult
{
    std::string name;
    double avg_time;
    double max_time;
    double min_time;
    long long total_ops;
};

class BenchmarkStats
{
  private:
    std::vector<Duration> timings;
    std::string name;

  public:
    BenchmarkStats(const std::string &name_) : name(name_)
    {
    }

    void add_timing(Duration d)
    {
        timings.push_back(d);
    }

    BenchmarkResult get_result()
    {
        BenchmarkResult result;
        result.name = name;
        result.total_ops = timings.size();

        if (timings.empty())
        {
            result.avg_time = 0;
            result.max_time = 0;
            result.min_time = 0;
            return result;
        }

        double sum = 0;
        result.max_time = timings[0].count();
        result.min_time = timings[0].count();

        for (const auto &t : timings)
        {
            sum += t.count();
            result.max_time = std::max(result.max_time, t.count());
            result.min_time = std::min(result.min_time, t.count());
        }

        result.avg_time = sum / timings.size();
        return result;
    }
};

void balanced_workload(LFStack<int> &stack, int thread_id, int ops_per_thread, BenchmarkStats &stats)
{
    for (int i = 0; i < ops_per_thread; ++i)
    {
        // Push
        auto start = Clock::now();
        stack.push(thread_id * ops_per_thread + i);
        auto push_time = Clock::now() - start;
        stats.add_timing(std::chrono::duration_cast<Duration>(push_time));

        // Pop
        start = Clock::now();
        stack.pop();
        auto pop_time = Clock::now() - start;
        stats.add_timing(std::chrono::duration_cast<Duration>(pop_time));
    }
}

void push_heavy_workload(LFStack<int> &stack, int thread_id, int ops_per_thread, BenchmarkStats &stats)
{
    for (int i = 0; i < ops_per_thread; ++i)
    {
        // 4 push на каждый pop
        for (int j = 0; j < 4; ++j)
        {
            auto start = Clock::now();
            stack.push(thread_id * ops_per_thread + i * 4 + j);
            auto push_time = Clock::now() - start;
            stats.add_timing(std::chrono::duration_cast<Duration>(push_time));
        }

        auto start = Clock::now();
        stack.pop();
        auto pop_time = Clock::now() - start;
        stats.add_timing(std::chrono::duration_cast<Duration>(pop_time));
    }
}

void pop_heavy_workload(LFStack<int> &stack, int thread_id, int ops_per_thread, BenchmarkStats &stats)
{
    for (int i = 0; i < ops_per_thread * 4; ++i)
    {
        stack.push(i);
    }

    for (int i = 0; i < ops_per_thread; ++i)
    {
        auto start = Clock::now();
        stack.push(thread_id * ops_per_thread + i);
        auto push_time = Clock::now() - start;
        stats.add_timing(std::chrono::duration_cast<Duration>(push_time));

        // 4 pop на каждый push
        for (int j = 0; j < 4; ++j)
        {
            start = Clock::now();
            stack.pop();
            auto pop_time = Clock::now() - start;
            stats.add_timing(std::chrono::duration_cast<Duration>(pop_time));
        }
    }
}

void run_benchmark(const std::string &workload_name, void (*workload_func)(LFStack<int> &, int, int, BenchmarkStats &),
                   int num_threads, int ops_per_thread, std::vector<BenchmarkResult> &results)
{
    LFStack<int> stack;
    std::vector<std::thread> threads;
    std::vector<BenchmarkStats> stats;

    std::cout << "Running " << workload_name << " with " << num_threads << " threads...\n";

    // Резервируем место заранее чтобы избежать переаллокации
    stats.reserve(num_threads);
    threads.reserve(num_threads);

    // Создаем потоки
    for (int i = 0; i < num_threads; ++i)
    {
        stats.emplace_back(workload_name);
        threads.emplace_back([&stack, i, ops_per_thread, &workload_func, &stats]() {
            workload_func(stack, i, ops_per_thread, stats[i]);
        });
    }

    // Ждем завершения
    for (auto &t : threads)
    {
        t.join();
    }

    // Собираем результаты
    BenchmarkResult combined;
    combined.name = workload_name;
    combined.avg_time = 0;
    combined.max_time = 0;
    combined.min_time = std::numeric_limits<double>::max();
    combined.total_ops = 0;

    for (auto &s : stats)
    {
        auto result = s.get_result();
        combined.avg_time += result.avg_time;
        combined.max_time = std::max(combined.max_time, result.max_time);
        combined.min_time = std::min(combined.min_time, result.min_time);
        combined.total_ops += result.total_ops;
    }

    combined.avg_time /= num_threads;

    results.push_back(combined);

    std::cout << std::fixed << std::setprecision(4) << "  Avg time: " << combined.avg_time
              << " ms, Max: " << combined.max_time << " ms, Min: " << combined.min_time << " ms\n\n";
}

int main()
{
    std::vector<BenchmarkResult> all_results;

    int num_threads = 4;
    int ops_per_thread = 10000;

    std::cout << "=== BALANCED WORKLOAD ===\n";
    run_benchmark("Balanced (50/50 push/pop)", balanced_workload, num_threads, ops_per_thread, all_results);

    std::cout << "=== PUSH-HEAVY WORKLOAD ===\n";
    run_benchmark("Push-Heavy (80/20 push/pop)", push_heavy_workload, num_threads, ops_per_thread, all_results);

    std::cout << "=== POP-HEAVY WORKLOAD ===\n";
    run_benchmark("Pop-Heavy (20/80 push/pop)", pop_heavy_workload, num_threads, ops_per_thread, all_results);

    std::ofstream csv("benchmark_results.csv");
    csv << "Workload,Avg_Time_ms,Max_Time_ms,Min_Time_ms,Total_Ops\n";
    for (const auto &result : all_results)
    {
        csv << result.name << "," << std::fixed << std::setprecision(4) << result.avg_time << "," << result.max_time
            << "," << result.min_time << "," << result.total_ops << "\n";
    }
    csv.close();

    std::cout << "\nResults saved to benchmark_results.csv\n";

    return 0;
}
