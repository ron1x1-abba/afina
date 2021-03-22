#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

    Executor::Executor(size_t low_watermark = 4, size_t high_watermark = 8, size_t idle_time = 1000, size_t max_queue_size = 30) 
        : low_watermark(low_watermark), high_watermark(high_watermark), idle_time(idle_time), max_queue_size(max_queue_size) {
            std::unique_lock<std::mutex> lock(mutex);
            state = State::kRun;
            cur_running = 0;
            for(size_t i = 0; i < low_watermark; ++i) {
                ++cur_threads;
                threads.push_back(std::thread(perform, this));
            }
        }
    
    Executor::~Executor() {
        Stop(true);
    }

    void Executor::Stop(bool await) {
        std::unique_lock<std::mutex> lock(mutex);
        if (state != State::kStopped) {
            state = State::kStopping;
        }
    }

    void perform(Executor *executor) {
        std::unique_lock<std::mutex> lock(executor->mutex);
        size_t waiting_time = executor->idle_time;
        while (executor->state == Executor::State::kRun) {
            bool timeout_var = false;
            while((executor->state == Executor::State::kRun) && (executor->tasks.empty())) {
                auto point = std::chrono::steady_clock::now();
                auto timeout = executor->empty_condition.wait_for(lock, std::chrono::duration<std::chrono::milliseconds>(std::chrono::milliseconds {waiting_time}));
                if (timeout == std::cv_status::timeout) {
                    timeout_var = true;
                    break;
                }
                else {
                    waiting_time -= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - point).count();
                }
            }
            if (timeout_var && (executor->cur_threads > executor->low_watermark)) {
                break;
            }
            if (executor->tasks.empty()) {
                break;
            }
            executor->empty_condition.wait(lock);
        }
        --(executor->cur_threads);
        if((executor->cur_threads == 0) && (executor->state == Executor::State::kStopping)) {
            executor->state = Executor::State::kStopped;
            executor->stop.notify_all();
        }
}
}
} // namespace Afina
