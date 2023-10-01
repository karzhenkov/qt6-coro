#include "coro/Future.h"
#include "coro/Utils.h"
#include <QTest>
#include <QTimer>

struct MyContext : coro::IContext
{
  int _sum = 0;
  int _busy = false;
  int _count = 0;

  QFuture<int> myCoroutine(int num)
  {
    _sum += num;
    if (_busy)
      co_return -num;

    _busy = true;
    co_await coro::Yield();
    _busy = false;

    _count += 1;
    co_return _sum;
  }
};

class Test7 : public QObject
{
  Q_OBJECT

private slots:
  void test_Yield()
  {
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);

    std::vector<int> res;
    auto push = [&res](int value) { res.push_back(value); };

    auto context = coro::make_unique<MyContext>();

    QTimer::singleShot(100, [&] {
      QFuture<void> futures[] = {
        context->myCoroutine(10).then(push),
        context->myCoroutine(200).then(push),
        context->myCoroutine(3000).then(push),
      };

      QFuture<void> whenAll(QtFuture::whenAll(std::begin(futures), std::end(futures)));
      whenAll.then([&] { loop.exit(1); });
    });

    QVERIFY(loop.exec() == 1);
    QCOMPARE(res, std::vector({ -200, -3000, 3210 }));
    QCOMPARE(context->_count, 1);
    QCOMPARE(context->_sum, 3210);
  }
};

QTEST_GUILESS_MAIN(Test7)

#include "test7.moc"
