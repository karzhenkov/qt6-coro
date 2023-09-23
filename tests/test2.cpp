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

  static QFuture<void> func1(MyContext* self)
  {
    QScopeGuard guard([self] { self->push(19); });
    self->push(10);
    co_await self->func2();
    self->push(11);
  }

  QFuture<void> func2()
  {
    QScopeGuard guard([this] { push(29); });
    push(20);
    co_await func3(this);
    push(21);
  }

  static QFuture<void> func3(MyContext* self)
  {
    QScopeGuard guard([self] { self->push(39); });
    self->push(30);
    co_await self->_promise->future();
    self->push(31);
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

class Test2 : public QObject
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

    _future = MyContext::func1(_context.get());
    auto contextList = getContextList();

    QVERIFY(!_future.isFinished());
    QCOMPARE(contextList.size(), 1);
    QVERIFY(contextList[0] == _context->getCoroutineContext());
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
    QCOMPARE(_trace, std::vector({ 10, 20, 30, 29, 19, 99, 39 }));
  }
};

QTEST_GUILESS_MAIN(Test2)

#include "test2.moc"
