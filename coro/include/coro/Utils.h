#pragma once

#include <QFuture>
#include <QTimer>
#include <optional>
#include <coroutine>

namespace coro {

inline
auto optional(QFuture<void> future)
{
  return future
    .then([] { return true; })
    .onCanceled([] { return false; });
}

template <typename T>
auto optional(QFuture<T> future)
{
  return future
    .then([](T v) { return std::optional<T>(std::move(v)); })
    .onCanceled([] { return std::nullopt; });
}

inline
auto whenCompletedOrCanceled(QFuture<void> future)
{
  return future.onCanceled([] {});
}

struct Cancel
{
  bool await_ready() noexcept { return false; }
  void await_resume() noexcept {}
  void await_suspend(std::coroutine_handle<> coro) { coro.destroy(); }
};

struct Yield
{
  std::shared_ptr<void> _self{this, [](auto) {}};

  bool await_ready() noexcept { return false; }

  void await_resume() noexcept {}

  void await_suspend(std::coroutine_handle<> coro)
  {
    std::weak_ptr self(_self);
    QTimer::singleShot(0, [=] {
      if (self.lock())
        coro.resume();
    });
  }
};

}  // namespace coro
