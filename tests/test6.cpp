#include "coro/Future.h"
#include "coro/Utils.h"
#include "coro/Delay.h"
#include <QTest>
#include <QTimer>

QFuture<void> myCoroutine(bool& b1, bool& b2)
{
  b1 = true;
  co_await coro::Delay(100);
  b2 = true;
  co_await coro::Cancel();
}

class Test6 : public QObject
{
  Q_OBJECT

  static inline bool b1 = false;
  static inline bool b2 = false;

private slots:
  void test_Cancel()
  {
    auto future = myCoroutine(b1, b2);

    QVERIFY(!future.isFinished());
    QVERIFY(b1);
    QVERIFY(!b2);

    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);

    future
        .then([&] { loop.exit(123); })
        .onCanceled([&] { loop.exit(234); });

    QCOMPARE(loop.exec(), 234);
    QVERIFY(b2);
  }
};

QTEST_GUILESS_MAIN(Test6)

#include "test6.moc"
