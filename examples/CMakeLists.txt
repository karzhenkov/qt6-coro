link_libraries(coro)
qt_add_executable(basic basic.cpp)
qt_add_executable(signal signal.cpp)

link_libraries(Qt::Network)
qt_add_executable(client client.cpp)
qt_add_executable(server server.cpp)
