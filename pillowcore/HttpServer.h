#ifndef _PILLOW_HTTPSERVER_H_
#define _PILLOW_HTTPSERVER_H_

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QHostAddress>

namespace Pillow
{
	class HttpConnection;
	
	//
	// HttpServer
	//
	
	class HttpServerPrivate;
	
	class HttpServer : public QTcpServer
	{
		Q_OBJECT
		Q_PROPERTY(QHostAddress serverAddress READ serverAddress)
		Q_PROPERTY(int serverPort READ serverPort)
		Q_PROPERTY(bool listening READ isListening)
		Q_DECLARE_PRIVATE(HttpServer)
		HttpServerPrivate* d_ptr;
		
	private slots:
		void connection_closed(Pillow::HttpConnection* request);
		
	protected:
#if QT_VERSION < 0x050000
        void incomingConnection(int socketDescriptor);
#else
        void incomingConnection(qintptr socketDescriptor);
#endif
		HttpConnection* createHttpConnection();
	
	public:
		HttpServer(QObject* parent = 0);
		HttpServer(const QHostAddress& serverAddress, quint16 serverPort, QObject *parent = 0);
		~HttpServer();		
		
	signals:
		void requestReady(Pillow::HttpConnection* connection); // There is a request ready to be handled on this connection.
	};
		
	//
	// HttpLocalServer
	//
		
	class HttpLocalServer : public QLocalServer
	{
		Q_OBJECT
		Q_DECLARE_PRIVATE(HttpServer)
		HttpServerPrivate* d_ptr;
		
	private slots:
		void this_newConnection();
		void connection_closed(Pillow::HttpConnection* request);

	public:
		HttpLocalServer(QObject* parent = 0);
		HttpLocalServer(const QString& serverName, QObject *parent = 0);
		
	signals:
		void requestReady(Pillow::HttpConnection* connection); // There is a request ready to be handled on this connection.
	};	
}

#endif // _PILLOW_HTTPSERVER_H_
