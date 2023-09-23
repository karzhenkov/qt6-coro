#pragma once

// workaround for QTBUG-73263
#if __GNUG__
#include <bits/c++config.h>
#endif

#include <concepts>
#include <coroutine>
#include <type_traits>

namespace coro {

template <typename T>
concept ContextBase = requires(const T& object)
{
  requires std::is_abstract_v<T>;
  requires std::has_virtual_destructor_v<T>;
  { object.getCoroutineContext() } -> std::same_as<const void*>;
};

namespace detail {

struct PromiseChainItem
{
  inline static thread_local PromiseChainItem* _first = nullptr;

  PromiseChainItem** _prev = &_first;
  PromiseChainItem* _next = _first;

  const void* const _context;
  std::coroutine_handle<> _thisCoro;

  PromiseChainItem(const void* context = nullptr)
    : _context(context)
  {
    if (context)
    {
      if (_first)
        _first->_prev = &_next;
      _first = this;
    }
  }

  ~PromiseChainItem()
  {
    if (_context)
    {
      *_prev = _next;
      if (_next)
        _next->_prev = _prev;
    }
  }

  static void destroyForContext(void* context)
  {
    PromiseChainItem** pp = &_first;
    while (auto p = *pp)
    {
      if (p->_context == context)
      {
        p->_thisCoro.destroy();
        pp = &_first;
        continue;
      }

      pp = &p->_next;
    }
  }
};

template <typename T>
const void* getContextFrom(const T& object)
{
  if constexpr (ContextBase<T>)
    return object.getCoroutineContext();
  else
    return nullptr;
}

}  // namespace detail

class IContext
{
public:
  virtual ~IContext() = default;
  virtual const void* getCoroutineContext() const = 0;
};

template <ContextBase T>
class Context final : public T
{
  using T::T;

public:
  ~Context() override { detail::PromiseChainItem::destroyForContext(this); }
  const void* getCoroutineContext() const override { return this; }
};

}  // namespace coro
