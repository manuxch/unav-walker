/*! \file thread_pool.hpp
 * \brief Pool de threads genérico para tareas concurrentes.
 *
 * Implementación estándar C++20:
 *   - Cola de tareas protegida por mutex
 *   - condition_variable para espera eficiente
 *   - Destructor que espera a que todas las tareas terminen (join)
 *
 * Uso:
 *   ThreadPool pool(std::thread::hardware_concurrency());
 *   auto fut = pool.enqueue([&]{ return compute(); });
 *   auto result = fut.get();
 *
 * \author Manuel Carlevaro
 * \date 2026-02-26
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool {
public:
    /// Crea el pool con \p n_threads workers.
    explicit ThreadPool(std::size_t n_threads)
        : stop_(false)
    {
        if (n_threads == 0)
            n_threads = 1;
        workers_.reserve(n_threads);
        for (std::size_t i = 0; i < n_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }

    /// Encola una tarea y devuelve un std::future con su resultado.
    /// La tarea puede ser cualquier callable que devuelva un valor.
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using ReturnT = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<ReturnT()>>(
            [f = std::forward<F>(f),
             ... args = std::forward<Args>(args)]() mutable {
                return f(std::forward<Args>(args)...);
            });
        std::future<ReturnT> res = task->get_future();
        {
            std::unique_lock lock(queue_mutex_);
            if (stop_)
                throw std::runtime_error("ThreadPool: enqueue en pool detenido");
            tasks_.emplace([task] { (*task)(); });
        }
        condition_.notify_one();
        return res;
    }

    /// Espera a que terminen todas las tareas y destruye los workers.
    ~ThreadPool()
    {
        {
            std::unique_lock lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (auto& w : workers_) w.join();
    }

    /// Número de threads del pool.
    std::size_t size() const { return workers_.size(); }

private:
    void worker_loop()
    {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock lock(queue_mutex_);
                condition_.wait(lock, [this] {
                    return stop_ || !tasks_.empty();
                });
                if (stop_ && tasks_.empty()) return;
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::thread>          workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex                        queue_mutex_;
    std::condition_variable           condition_;
    bool                              stop_;
};
