#include "HttpHandlerSimpleRouter.h"
#include "HttpRequest.h"
#include <QtCore/QPointer>
#include <QtCore/QRegExp>
#include <QtCore/QVarLengthArray>
#include <QtCore/QUrl>
using namespace Pillow;

namespace Pillow
{
	struct Route
	{
		QByteArray method;
		QRegExp regExp;
		QStringList paramNames;
		
		virtual bool invoke(Pillow::HttpRequest* request) = 0;
	};
	
	struct HandlerRoute : public Route
	{
		QPointer<Pillow::HttpHandler> handler;
		
		virtual bool invoke(Pillow::HttpRequest *request)
		{
			if (!handler) return false;
			return handler->handleRequest(request);
		}
	};
	
	struct QObjectMetaCallRoute : public Route
	{
		QPointer<QObject> object;
		const char* member;
		
		virtual bool invoke(Pillow::HttpRequest *request)
		{
			if (!object) return false;
			return QMetaObject::invokeMethod(object, member, Q_ARG(Pillow::HttpRequest*, request));
		}
	};
	
	struct StaticRoute : public Route
	{
		int statusCode;
		Pillow::HttpHeaderCollection headers;
		QByteArray content;
		
		virtual bool invoke(Pillow::HttpRequest *request)
		{
			request->writeResponse(statusCode, headers, content);
			return true;
		}		
	};

	//
	// HttpHandlerSimpleRouterPrivate
	//

	class HttpHandlerSimpleRouterPrivate
	{
	public:
		QList<Route*> routes;
		HttpHandlerSimpleRouter::RoutingErrorAction methodMismatchAction;
		HttpHandlerSimpleRouter::RoutingErrorAction unmatchedRequestAction;
	};
}

//
// HttpHandlerSimpleRouter
//

HttpHandlerSimpleRouter::HttpHandlerSimpleRouter(QObject* parent /* = 0 */)
	: Pillow::HttpHandler(parent), d_ptr(new HttpHandlerSimpleRouterPrivate)
{
	d_ptr->methodMismatchAction = Passthrough;
	d_ptr->unmatchedRequestAction = Passthrough;
}

HttpHandlerSimpleRouter::~HttpHandlerSimpleRouter()
{
	foreach (Route* route, d_ptr->routes)
		delete route;
	delete d_ptr;
}

void HttpHandlerSimpleRouter::addRoute(const QByteArray& method, const QString &path, Pillow::HttpHandler *handler)
{
	HandlerRoute* route = new HandlerRoute();
	route->method = method;
	route->regExp = pathToRegExp(path, &route->paramNames);
	route->handler = handler;
	d_ptr->routes.append(route);
}

void HttpHandlerSimpleRouter::addRoute(const QByteArray& method, const QString& path, QObject* object, const char* member)
{
	QObjectMetaCallRoute* route = new QObjectMetaCallRoute();
	route->method = method;
	route->regExp = pathToRegExp(path, &route->paramNames);
	route->object = object;
	route->member = member;
	d_ptr->routes.append(route);
}

void HttpHandlerSimpleRouter::addRoute(const QByteArray& method, const QString& path, int statusCode, const Pillow::HttpHeaderCollection& headers, const QByteArray& content /*= QByteArray()*/)
{
	StaticRoute* route = new StaticRoute();
	route->method = method;
	route->regExp = pathToRegExp(path, &route->paramNames);
	route->statusCode = statusCode;
	route->headers = headers;
	route->content = content;
	d_ptr->routes.append(route);
}

QRegExp Pillow::HttpHandlerSimpleRouter::pathToRegExp(const QString &p, QStringList* outParamNames)
{
	QString path = p;

	QRegExp paramRegex(":(\\w+)"); QString paramReplacement("([\\w_-]+)");
	QStringList paramNames;
	int pos = 0;
	while (pos >= 0)
	{
		pos = paramRegex.indexIn(path, pos);
		if (pos >= 0)
		{
			paramNames << paramRegex.cap(1);
			pos += paramRegex.matchedLength();
		}
	}
	path.replace(paramRegex, paramReplacement);

	QRegExp splatRegex("\\*(\\w+)"); QString splatReplacement("(.*)");
	pos = 0;
	while (pos >= 0)
	{
		pos = splatRegex.indexIn(path, pos);
		if (pos >= 0)
		{
			paramNames << splatRegex.cap(1);
			pos += splatRegex.matchedLength();
		}
	}
	path.replace(splatRegex, splatReplacement);

	if (outParamNames)
		*outParamNames = paramNames;
	
	path = "^" + path + "$";
	return QRegExp(path);
}

bool HttpHandlerSimpleRouter::handleRequest(Pillow::HttpRequest *request)
{
	QVarLengthArray<Route*, 16> matchedRoutes;
	foreach (Route* route, d_ptr->routes) 
	{
		QString requestPath = QUrl::fromPercentEncoding(request->requestPath());
		if (route->regExp.indexIn(requestPath) != -1)
		{
			matchedRoutes.append(route);
			if (route->method.isEmpty() || route->method == request->requestMethod())
			{
				for (int i = 0, iE = route->paramNames.size(); i < iE; ++i)
					request->setRequestParam(route->paramNames.at(i), route->regExp.cap(i + 1));
				route->invoke(request);
				return true;
			}
		}
	}
	
	if (matchedRoutes.isEmpty())
	{
		if (unmatchedRequestAction() == Return4xxResponse)
		{
			request->writeResponse(404);
			return true;
		}
	}
	else
	{
		if (methodMismatchAction() == Return4xxResponse)
		{
			QByteArray allowedMethods;
			for (int i = 0, iE = matchedRoutes.size(); i < iE; ++i)
			{
				if (i > 0) allowedMethods.append(", ");
				allowedMethods.append(matchedRoutes.at(i)->method);
			}
			request->writeResponse(405, HttpHeaderCollection() << HttpHeader("Allow", allowedMethods));
			return true;
		}
	}
	
	return false;
}

Pillow::HttpHandlerSimpleRouter::RoutingErrorAction Pillow::HttpHandlerSimpleRouter::unmatchedRequestAction() const
{
	return d_ptr->unmatchedRequestAction;
}

void Pillow::HttpHandlerSimpleRouter::setUnmatchedRequestAction(Pillow::HttpHandlerSimpleRouter::RoutingErrorAction action)
{
	d_ptr->unmatchedRequestAction = action;
}

Pillow::HttpHandlerSimpleRouter::RoutingErrorAction Pillow::HttpHandlerSimpleRouter::methodMismatchAction() const
{
	return d_ptr->methodMismatchAction;
}

void Pillow::HttpHandlerSimpleRouter::setMethodMismatchAction(Pillow::HttpHandlerSimpleRouter::RoutingErrorAction action)
{
	d_ptr->methodMismatchAction = action;
}
