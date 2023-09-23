# C++20 coroutines for Qt6

## A basic example

```C++
class Example : public coro::IContext
{
public:
  QFuture<QString> myCoroutine(int milliseconds)
  {
    co_await coro::Delay(milliseconds);

    // "Example" is considered a coroutine context object.
    // If it gets destroyed while awaiting, we'll never get here.
    // Local variables (if any) are guaranteed to be destroyed
    // before destroying the context object.

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
```
This example and also a more elaborated TCP example can be found it the "examples" subfolder.

## Key points

- Everything here is about `co_await`/`co_return` only.
  The keyword `co_yield` is not covered.

- A coroutine is executed within a single thread.

- Return object of a coroutine function is `QFuture`.

- The primary thing that can be "co_awaited" is also `QFuture`;
  the result of `co_await` is normally the outcome of `QFuture`
  (which can be a resulting value or an exception).
  Cancellation is treated differently than the normal outcome.

- Cancellation of the "co_awaited" `QFuture` results in the following:
  - the `co_await` operator never returns control;
  - local variables of the "co_awaiting" coroutine are destroyed by destroying the coroutine frame;
  - the "co_awaiting" coroutine (i. e. its `QFuture` return object) is canceled as well.

- A "co_awaiting" coroutine is canceled in the same way when its context object (if any) is destroyed.

- A coroutine has a context object if the following conditions are met:
  - the coroutine function is a non-static member function of some class;
  - the instance of that class (the context object)
    is created using the `coro::Context` template (see the above example),
    which implies that the class satisfies the `coro::ContextBase` concept
    (it's enough to inherit from the `coro::IContext` convenience interface).

- Definitions of `coro::ContextBase` and `coro::IContext` are as follows:
```C++
namespace coro
{
  template <typename T>
  concept ContextBase = requires(const T& object)
  {
    requires std::is_abstract_v<T>;
    requires std::has_virtual_destructor_v<T>;
    { object.getCoroutineContext() } -> std::same_as<const void*>;
  };

  class IContext
  {
  public:
    virtual ~IContext() = default;
    virtual const void* getCoroutineContext() const = 0;
  };
}

```

## Usage

To use the library, add the subfolder "coro" to the header search path.
The library doesn't contain any separately compiled code and depends only on `Qt::Core`.

## Requirements

- Qt 6
- A compiler supporting C++20 coroutines and concepts (e.g. GCC 11 or MSVC 2022)
