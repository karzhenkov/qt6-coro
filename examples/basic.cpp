#include "coro/Context.h"
#include "coro/Delay.h"
#include <QCoreApplication>
#include <QRandomGenerator>
#include <QScopeGuard>

class Example : public coro::IContext
{
public:
  ~Example() { qInfo() << "context object destroyed"; }

  QFuture<QString> myCoroutine(int milliseconds)
  {
    QScopeGuard guard([] { qInfo() << "local variables destroyed"; });

    co_await coro::Delay(milliseconds);

    // "Example" is considered a coroutine context object.
    // If it gets destroyed while awaiting, we'll never get here.
    // Local variables (if any) are guaranteed to be destroyed
    // before destroying the context object.

    qInfo() << "coroutine continuation";
    co_return QStringLiteral("finished");
  }
};

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  // Create a context and schedule its deletion.
  Example* context = new coro::Context<Example>;
  int deleteAfter = QRandomGenerator::global()->bounded(500, 1500);
  QTimer::singleShot(deleteAfter, [context] { delete context; });

  // Start a coroutine and attach continuations.
  context->myCoroutine(1000)
      .onCanceled([] { return QStringLiteral("canceled"); })
      .then([](QString result) { qInfo() << result; });

  QTimer::singleShot(2000, [] { QCoreApplication::quit(); });
  return QCoreApplication::exec();
}
