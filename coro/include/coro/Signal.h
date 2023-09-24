#pragma once

#include "coro/Utils.h"
#include <QObject>
#include <concepts>
#include <tuple>

namespace coro {
namespace detail {

template <typename... Args>
struct SigResult
{
  using type = std::tuple<Args...>;
};

template <typename T>
struct SigResult<T>
{
  using type = T;
};

template <>
struct SigResult<>
{
  using type = void;
};

template <auto, bool Opt>
class Signal;

template <bool Opt, std::derived_from<QObject> T, typename... SigArgs, void (T::*SigFunc)(SigArgs...)>
class Signal<SigFunc, Opt>
{
public:
  auto operator()(T* sender, const QObject* receiver) const
  {
    using Result = typename SigResult<SigArgs...>::type;

    QPromise<Result> promise;
    promise.start();
    auto future = promise.future();

    if (sender && receiver)
    {
      auto sigHandler = [=, promise = std::move(promise)](SigArgs... args) mutable {
        sender->disconnect(receiver);
        if constexpr (sizeof...(SigArgs) > 0)
          promise.addResult(Result(args...));
        promise.finish();
      };

      QObject::connect(sender, SigFunc, receiver, std::move(sigHandler));
    }

    if constexpr(Opt)
      return optional(future);
    else
      return future;
  }

  auto operator()(T& sender, const QObject* receiver) const
  {
    return operator()(&sender, receiver);
  }

  auto operator()(T& sender, const QObject& receiver = QObject()) const
  {
    return operator()(&sender, &receiver);
  }
};

}  // namespace detail

template <auto SigFunc>
constexpr detail::Signal<SigFunc, false> connect;

template <auto SigFunc>
constexpr detail::Signal<SigFunc, true> connectOpt;

}  // namespace coro
