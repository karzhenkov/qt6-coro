#pragma once

#include "Context.h"
#include <QFuture>
#include <QFutureInterface>
#include <QObject>

namespace coro::detail {

template <typename Res>
class FutureInterface : public QFutureInterface<Res>
{
public:
  using QFutureInterfaceBase::runContinuation;
  using QFutureInterfaceBase::cleanContinuation;
};

template <typename Res>
struct Promise;

template <typename Res>
struct PromiseBase : PromiseChainItem
{
  FutureInterface<Res> _promise;
  bool _finished = false;

  PromiseBase() = default;

  template <typename T, typename... Args>
  PromiseBase(T& object, Args&&...)
      : PromiseChainItem(getContextFrom(object))
  {
  }

  ~PromiseBase()
  {
    if (_finished)
    {
      _promise.reportFinished();
    }
    else
    {
      _promise.cancelAndFinish();
      _promise.runContinuation();
    }

    _promise.cleanContinuation();
  }

  auto get_return_object()
  {
    using Self = Promise<Res>;
    auto& self = *static_cast<Self*>(this);
    _thisCoro = std::coroutine_handle<Self>::from_promise(self);

    _promise.reportStarted();
    return _promise.future();
  }

  void unhandled_exception()
  {
    _promise.reportException(std::current_exception());
    _finished = true;
  }

  auto initial_suspend() noexcept { return std::suspend_never(); }
  auto final_suspend() noexcept { return std::suspend_never(); }
};

template <typename Res>
struct Promise : PromiseBase<Res>
{
  using PromiseBase<Res>::PromiseBase;

  template <typename R>
  void return_value(R&& res)
  {
    this->_promise.reportResult(static_cast<R&&>(res));
    this->_finished = true;
  }
};

template <>
struct Promise<void> : PromiseBase<void>
{
  using PromiseBase::PromiseBase;

  void return_void() { _finished = true; }
};

template <typename T>
bool hasException(const QFuture<T>& future)
{
  return future.isCanceled() && QFuture<void>(future).onCanceled([] {}).isCanceled();
}

template <typename T>
bool hasResult(const QFuture<T>& future)
{
  return !future.isCanceled() && future.isFinished();
}

template <typename T>
bool isReady(const QFuture<T>& future) { return hasResult(future) || hasException(future); }

inline
QObject* threadingContext()
{
  static thread_local QObject object;
  return &object;
}

}  // namespace coro::detail

template <typename Res>
auto operator co_await(const QFuture<Res>& future)
{
  struct Awaiter
  {
    QFuture<Res> _future;
    std::shared_ptr<void> _self;
    const bool _ready = coro::detail::isReady(_future);

    Awaiter(const QFuture<Res>& future)
      : _future(future)
    {
    }

    bool await_ready() noexcept { return _ready; }

    auto await_resume()
    {
      if constexpr (std::is_same_v<Res, void>)
        _future.waitForFinished();
      else if constexpr (std::is_copy_constructible_v<Res>)
        return _future.result();
      else
        return _future.takeResult();
    }

    void await_suspend(std::coroutine_handle<> coro)
    {
      if (_future.isCanceled())
      {
        coro.destroy();
        return;
      }

      _self.reset(this, [](auto) {});
      std::weak_ptr self(_self);
      QObject* threadingContext = coro::detail::threadingContext();

      _future
          .then(threadingContext, [=](QFuture<Res>) {
            if (self.lock())
              coro.resume();
          })
          .onCanceled(threadingContext, [=] {
            if (self.lock())
              coro.destroy();
          });
    }
  };

  return Awaiter(future);
}

template <typename Res, typename... Args>
struct std::coroutine_traits<QFuture<Res>, Args...>
{
  using promise_type = coro::detail::Promise<Res>;
};
