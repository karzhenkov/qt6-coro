#include <QCoreApplication>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>

#include "coro/Future.h"

const int tcpPort = 10000;

QFuture<void> runClientSession(QTcpSocket* socket, int clientId)
{
  qDebug("%d: connected", clientId);
  QScopeGuard guard([clientId] { qDebug("%d: disconnected", clientId); } );
  QObject::connect(socket, &QTcpSocket::disconnected, [socket] { socket->deleteLater(); });
  int dataCount = 0;

  while (true)
  {
    if (socket->bytesAvailable() == 0)
      co_await QtFuture::connect(socket, &QTcpSocket::readyRead);

    QByteArray data = socket->readAll();
    socket->write(data);

    dataCount += data.length();
    qDebug("%d: %d", clientId, dataCount);
  }
}

QFuture<void> acceptConnections(QTcpServer& server)
{
  int clientCount = 0;

  while (true)
  {
    if (!server.hasPendingConnections())
      co_await QtFuture::connect(&server, &QTcpServer::newConnection);

    runClientSession(server.nextPendingConnection(), ++clientCount);
  }
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  QTcpServer server;
  if (!server.listen(QHostAddress::Any, tcpPort))
    return -1;

  acceptConnections(server);
  return QCoreApplication::exec();
}
