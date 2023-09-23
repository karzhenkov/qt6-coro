#include "coro/Future.h"
#include <QScopeGuard>
#include <QTest>
#include <optional>
#include <vector>

struct MyContext : coro::IContext
{
  std::optional<QPromise<void>> _promise{std::in_place};
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
    co_await func2(this);
    push(11);
  }

  static QFuture<void> func2(MyContext* self)
  {
    QScopeGuard guard([self] { self->push(29); });
    self->push(20);
    co_await self->func3();
    self->push(21);
  }

  QFuture<void> func3()
  {
    QScopeGuard guard([this] { push(39); });
    push(30);
    co_await _promise->future();
    push(31);
    co_return;
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

class Test1 : public QObject
{
  Q_OBJECT

  std::vector<int> _trace;
  std::unique_ptr<MyContext> _context;
  QFuture<void> _future;

private slots:
  void init()
  {
    _context.reset(new coro::Context<MyContext>(_trace));
    _trace.clear();

    _future = _context->func1();
    auto contextList = getContextList();

    QVERIFY(!_future.isFinished());
    QCOMPARE(contextList.size(), 2);
    QVERIFY(contextList[0] == _context->getCoroutineContext());
    QVERIFY(contextList[1] == _context->getCoroutineContext());
  }

  void cleanup()
  {
    QVERIFY(_future.isFinished());
    QCOMPARE(getContextList().size(), 0);
  }

  void test_finished()
  {
    _context->_promise->finish();
    QVERIFY(!_future.isCanceled());
    QCOMPARE(_trace, std::vector({ 10, 20, 30, 31, 39, 21, 29, 11, 19 }));
  }

  void test_canceled_1()
  {
    _context->_promise.reset();
    QVERIFY(_future.isCanceled());
    QCOMPARE(_trace, std::vector({ 10, 20, 30, 39, 29, 19 }));
  }

  void test_canceled_2()
  {
    _context.reset();
    QVERIFY(_future.isCanceled());
    QCOMPARE(_trace, std::vector({ 10, 20, 30, 39, 29, 19, 99 }));
  }
};

QTEST_GUILESS_MAIN(Test1)

#include "test1.moc"
