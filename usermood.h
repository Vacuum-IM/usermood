#ifndef USERMOOD_H
#define USERMOOD_H

#include <definitions.h>
#include "usermooddialog.h"
#include "ui_usermooddialog.h"

#include <interfaces/imainwindow.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>

#include <definitions/menuicons.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rostertooltiporders.h>

#include <utils/action.h>
#include <utils/filestorage.h>
#include <utils/iconstorage.h>
#include <utils/menu.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>

#define USERMOOD_UUID "{df730f89-9cb1-472a-b61b-aea95594fde1}"
#define PEP_USERMOOD 4010

class MoodData
{
public:
	MoodData() {}
	MoodData(const QString &locname)
	: locname(locname) {}
	MoodData(const QString &icon, const QString &locname)
	: locname(locname), icon(IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(icon)) {}
	QString locname;
	QIcon icon;
};

class MoodContact
{
public:
	QString keyname;
	QString text;
};

class UserMood :
	public QObject,
	public IPlugin,
	public IRosterDataHolder,
	public IPEPHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterDataHolder IPEPHandler);

public:
	UserMood();
	~UserMood();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return USERMOOD_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }

	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);

//    virtual QIcon getIcoByBareJid(const QString &ABareJid) const;

	//IPEPHandler
	virtual bool processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza);
	void isSetMood(const Jid &streamJid, const QString &AMoodKey, const QString &AMoodText);

signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);

protected slots:
//    void onOptionsOpened();
//    void onOptionsChanged(const OptionsNode &ANode);
//    void onRosterIndexInserted(const Jid &AContactJid, const QString &AMood);
	void onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int, QString> &AToolTips);
//    void onShowNotification(const QString &AContactJid);
//    void onNotificationActivated(int ANotifyId);
//    void onNotificationRemoved(int ANotifyId);
	void onRosterIndexInserted(IRosterIndex *AIndex);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
	void onSetMoodActionTriggered(bool);
	void onApplicationQuit();

protected:
	Action *createSetMoodAction(const Jid &AStreamJid, const QString &AFeature, QObject *AParent) const;
	void setContactMood(const Jid &ASenderJid, const QString &AMoodName, const QString &AMoodText);
	void updateDataHolder(const Jid &ASenderJid = Jid::null);


private:
	IMainWindowPlugin *FMainWindowPlugin;
	IPresencePlugin *FPresencePlugin;
	IPEPManager *FPEPManager;
	IServiceDiscovery *FServiceDiscovery;
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
	IRostersModel *FRostersModel;
	IServiceDiscovery *FDiscovery;
	IRostersViewPlugin *FRostersViewPlugin;
	INotifications *FNotifications;

	int handlerId;
	int FUserMoodLabelId;

	QMap<QString, MoodData> FMoodsCatalog;
	QMap<QString, MoodContact> FContactsMood;

};

#endif // USERMOOD_H


