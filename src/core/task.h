#pragma once

#include <future>
#include <thread>
#include <type_traits>
#include <utility>

// The worker owns its callable and input copies. Destroying its future never
// waits for a slow endpoint during app shutdown.
template <class Fn>
auto LaunchBackgroundTask(Fn&& fn)
    -> std::future<std::invoke_result_t<std::decay_t<Fn>&>> {
    using Result = std::invoke_result_t<std::decay_t<Fn>&>;
    std::packaged_task<Result()> task(std::forward<Fn>(fn));
    auto future = task.get_future();
    std::thread(std::move(task)).detach();
    return future;
}
