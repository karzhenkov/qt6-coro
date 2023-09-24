#pragma once

#include <QTimer>

namespace coro {

class TimedReceiver
{
  QObject* _object = nullptr;
  QTimer _timer;

public:
  TimedReceiver() { _timer.setSingleShot(true); }

  TimedReceiver(int ms)
      : TimedReceiver()
  {
    reset(ms);
  }

  ~TimedReceiver() { delete _object; }

  void reset(int ms)
  {
    delete std::exchange(_object, new QObject);
    _timer.start(ms);
    _timer.callOnTimeout(_object, [this] {
      delete std::exchange(_object, nullptr);
    });
  }

  operator const QObject*() const { return _object; }
};

}  // namespace coro
