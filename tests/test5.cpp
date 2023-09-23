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

  QFuture<std::unique_ptr<int>> func1()
  {
    QScopeGuard guard([this] { push(19); });
    push(10);
    co_await coro::optional(func2());
    push(11);
    co_return co_await func3();
  }

  QFuture<void> func2()
  {
    QScopeGuard guard([this] { push(29); });
    push(20);
    co_await std::suspend_always();
    push(21);
  }

  QFuture<std::unique_ptr<int>> func3()
  {
    QScopeGuard guard([this] { push(39); });
    push(30);
    int value = co_await func4().onCanceled([] { return 123; });
    push(31);
    co_return std::make_unique<int>(value);
  }

  QFuture<int> func4()
  {
    QScopeGuard guard([this] { push(49); });
    push(40);
    co_await std::suspend_always();
    push(41);
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

class Test5 : public QObject
{
  Q_OBJECT

  std::vector<int> _trace;

private slots:
  void init() { _trace.clear(); }

  void cleanup()
  {
    QCOMPARE(getContextList().size(), 0);
    QCOMPARE(_trace, std::vector({ 10, 20, 29, 11, 30, 40, 49, 31, 39, 19, 99 }));
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

    QVERIFY(future.isFinished());
    QVERIFY(!future.isCanceled());
  }

  void test2()
  {
    auto future = coro::Context<MyContext>(_trace).func1();
    QVERIFY(future.isFinished());
    QVERIFY(!future.isCanceled());
    QVERIFY(*future.takeResult() == 123);
  }
};

QTEST_GUILESS_MAIN(Test5)

#include "test5.moc"
