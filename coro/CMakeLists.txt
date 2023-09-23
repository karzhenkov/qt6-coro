qt_add_library(coro INTERFACE)

target_sources(coro PRIVATE
  include/coro/Context.h
  include/coro/Delay.h
  include/coro/Future.h
)

target_include_directories(coro INTERFACE include)

target_link_libraries(coro INTERFACE Qt::Core)
