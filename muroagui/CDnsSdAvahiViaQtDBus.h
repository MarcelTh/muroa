/*
 * CDnsSdAvahiViaQtDBus.h
 *
 *  Created on: 21 Mar 2010
 *      Author: martin
 */

#ifndef CDNSSD_H_
#define CDNSSD_H_

#include "CDnsSdBase.h"

#include <QObject>
#include <QtDBus/QtDBus>


class CDnsSdAvahiViaQtDBus : public CDnsSdBase {
	Q_OBJECT;
public:
	CDnsSdAvahiViaQtDBus();
	virtual ~CDnsSdAvahiViaQtDBus();

signals:
	void servicesChanged();
	void serviceFound(QString serviceName, QString hostName, QString domainName, int portNr);
	void serviceRemoved(QString serviceName, QString domainName);

public slots:
	void gotServiceBrowser(QDBusPendingCallWatcher* result);

	void newItemHandler(int interface, int protocol, QString name, QString stype, QString domain, unsigned flags);
	void delItemHandler(int interface, int protocol, QString name, QString stype, QString domain, unsigned flags);

private:
	QDBusConnection m_dbusConn;
	QDBusInterface *m_if;
	QDBusInterface *m_serviceBrowser;

	void resolveService(int interface, int protocol, QString name, QString stype, QString domain, unsigned flags);
};

#endif /* CDNSSD_H_ */
