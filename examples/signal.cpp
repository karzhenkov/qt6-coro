#include "coro/Future.h"
#include "coro/Signal.h"
#include "coro/TimedReceiver.h"
#include <QCoreApplication>
#include <QTimer>

class Sender : public QObject
{
  Q_OBJECT

public:

  Sender() { startTimer(1000); }
  ~Sender() { qInfo() << "Sender destroyed"; }

signals:
  void s0();
  void s1(int);
  void s2(int, QString);

private:
  int _count = 0;

  void timerEvent(QTimerEvent*) override
  {
    qInfo() << _count;

    switch (_count++ % 3)
    {
    case 0:
      emit s0();
      break;
    case 1:
      emit s1(123);
      break;
    case 2:
      emit s2(234, QStringLiteral("second argument"));
      break;
    }
  }
};

QFuture<void> run()
{
  Sender sender;

  co_await coro::connect<&Sender::s0>(sender);
  qInfo() << "Sender::s0";

  auto a = co_await coro::connect<&Sender::s1>(sender);
  qInfo() << "Sender::s1" << a;

  auto [b, c] = co_await coro::connect<&Sender::s2>(sender);
  qInfo() << "Sender::s2" << b << c;

  co_await coro::connect<&Sender::s0>(sender);
  qInfo() << "Sender::s0 again";

  coro::TimedReceiver receiver(11000);

  int count = 0;
  while (co_await coro::connectOpt<&Sender::s0>(sender, receiver))
    qInfo() << "Sender::s0 loop" << ++count;

  qInfo() << "done";
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  coro::whenCompletedOrCanceled(run()).then([] { qApp->quit(); });
  return QCoreApplication::exec();
}

#include "signal.moc"
