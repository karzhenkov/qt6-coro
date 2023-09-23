#pragma once

#include "Future.h"
#include <QTimer>
#include <optional>

namespace coro {

class Delay
{
public:
  Delay(int milliseconds)
    : _timer(std::in_place)
    , _future(QtFuture::connect(&*_timer, &QTimer::timeout))
  {
    _timer->setSingleShot(true);
    _timer->start(milliseconds);
  }

  void cancel() { _timer.reset(); }

  auto operator co_await() { return ::operator co_await(_future); }

private:
  std::optional<QTimer> _timer;
  QFuture<void> _future;
};

}  // namespace coro
