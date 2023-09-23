#include <QCoreApplication>
#include <QTcpSocket>

#include "coro/Delay.h"
#include "coro/Future.h"

const int tcpPort = 10000;

class Client : public QObject, public coro::IContext
{
  int _id;
  QTcpSocket _socket;

  Client(int id) : _id(id)
  {
    connect(&_socket, &QTcpSocket::errorOccurred, this, &QObject::deleteLater);
    connect(&_socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
  }

  QFuture<void> receiveData()
  {
    int dataCount = 0;

    while (true)
    {
      if (_socket.bytesAvailable() == 0)
        co_await QtFuture::connect(&_socket, &QTcpSocket::readyRead);

      auto data = _socket.readAll();
      dataCount += data.size();
      qDebug("%d: R %d", _id, dataCount);
    }
  }

public:
  ~Client() { qDebug("%d: context object destroyed", _id); }

  QFuture<void> run(int blockCount, int blockSize, int interval)
  {
    _socket.connectToHost(QHostAddress::LocalHost, tcpPort);
    co_await QtFuture::connect(&_socket, &QTcpSocket::connected);

    qDebug("%d: connected", _id);
    receiveData();

    int dataCount = 0;
    QByteArray dataBlock(blockSize, 0);

    while (blockCount-- > 0)
    {
      _socket.write(dataBlock);

      while (_socket.bytesToWrite() > 0)
      {
        dataCount += co_await QtFuture::connect(&_socket, &QTcpSocket::bytesWritten);
        qDebug("%d: W %d", _id, dataCount);
      }

      co_await coro::Delay(interval);
    }

    _socket.close();
    qDebug("%d: done", _id);
  }

  static auto create(int id) { return new coro::Context<Client>(id); }
};

QFuture<void> run()
{
  QFuture<void> futures[] = {
    Client::create(1)->run(10, 250, 2000),
    Client::create(2)->run(15, 500, 1000),
  };

  co_await QtFuture::whenAll(std::begin(futures), std::end(futures));

  QCoreApplication::quit();
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  run();
  return QCoreApplication::exec();
}
