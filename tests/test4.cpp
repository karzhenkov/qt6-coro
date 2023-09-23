#include "coro/Future.h"
#include <QScopeGuard>
#include <QTest>
#include <vector>

struct MyContext : coro::IContext
{
  std::vector<int>& _trace;

  MyContext(std::vector<int>& trace)
    : _trace(trace)
  {
  }

  ~MyContext() { push(99); }

  QFuture<void> func1()
  {
    QScopeGuard guard([this] { push(19); });
    push(10);
    co_await coro::optional(func2());
    push(11);
    co_await coro::optional(func3());
    push(12);
    co_await func4();
    push(13);
  }

  QFuture<void> func2()
  {
    QScopeGuard guard([this] { push(29); });
    push(20);
    co_await std::suspend_always();
    push(21);
  }

  QFuture<void> func3()
  {
    QScopeGuard guard([this] { push(39); });
    push(30);
    co_await std::suspend_always();
    push(31);
  }

  QFuture<int> func4()
  {
    QScopeGuard guard([this] { push(49); });
    push(40);
    int value = co_await func5().onCanceled([] { return 123; });
    push(41);
    co_return value;
  }

  QFuture<int> func5()
  {
    QScopeGuard guard([this] { push(59); });
    push(50);
    throw std::logic_error("thrown by coroutine");
    push(51);
    co_return 321;
  }

  void push(int num) const { _trace.push_back(num); }
};

auto getContextList()
{
  using Item = coro::detail::PromiseChainItem;
  QVector<const void*> res;
  for (Item* item = Item::_first; item != nullptr; item = item->_next)
    res += item->_context;
  return res;
}

class Test4 : public QObject
{
  Q_OBJECT

  std::vector<int> _trace;

private slots:
  void init() { _trace.clear(); }

  void cleanup()
  {
    QCOMPARE(getContextList().size(), 0);
    QCOMPARE(_trace, std::vector({ 10, 20, 29, 11, 30, 39, 12, 40, 50, 59, 49, 19, 99 }));
  }

  void test1()
  {
    auto context = new coro::Context<MyContext>(_trace);
    auto future = context->func1();
    QVERIFY(!future.isFinished());

    auto contextList = getContextList();
    QCOMPARE(contextList.size(), 2);
    QVERIFY(contextList[0] == context->getCoroutineContext());
    QVERIFY(contextList[1] == context->getCoroutineContext());

    delete context;

    QVERIFY(future.isCanceled());
    QVERIFY_THROWS_EXCEPTION(std::logic_error, future.waitForFinished());
  }

  void test2()
  {
    auto future = coro::Context<MyContext>(_trace).func1();
    QVERIFY(future.isCanceled());
    QVERIFY_THROWS_EXCEPTION(std::logic_error, future.waitForFinished());
  }
};

QTEST_GUILESS_MAIN(Test4)

#include "test4.moc"
