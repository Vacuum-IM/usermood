#ifndef USERMOOD_H
#define USERMOOD_H

#include "iusermood.h"
#include "usermooddialog.h"
#include "ui_usermooddialog.h"

#include <interfaces/imainwindow.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>

#include <definitions/menuicons.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rostertooltiporders.h>

#include <utils/action.h>
#include <utils/advanceditemdelegate.h>
#include <utils/menu.h>
#include <utils/options.h>
#include <utils/widgetmanager.h>

class UserMood :
	public QObject,
	public IPlugin,
	public IUserMood,
	public IRosterDataHolder,
	public IRostersLabelHolder,
	public IOptionsHolder,
	public IPEPHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IUserMood IRosterDataHolder IRostersLabelHolder IOptionsHolder IPEPHandler);

public:
	UserMood();
	~UserMood();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return USERMOOD_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IRostersLabelHolder
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const;
	//IPEPHandler
	virtual bool processPEPEvent(const Jid &streamJid, const Stanza &stanza);
	//IUserMood
	virtual void setMood(const Jid &streamJid, const Mood &mood);
	virtual QIcon moodIcon(const QString &keyname) const;
	virtual QString moodName(const QString &keyname) const;
	virtual QString contactMoodKey(const Jid &streamJid, const Jid &contactJid) const;
	virtual QIcon contactMoodIcon(const Jid &streamJid, const Jid &contactJid) const;
	virtual QString contactMoodName(const Jid &streamJid, const Jid &contactJid) const;
	virtual QString contactMoodText(const Jid &streamJid, const Jid &contactJid) const;

signals:
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
	//IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);

protected slots:
	//INotification
	void onShowNotification(const Jid &streamJid, const Jid &senderJid);
	void onNotificationActivated(int ANotifyId);
	void onNotificationRemoved(int ANotifyId);
	//IRostersView
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	//IXmppStreams
	void onStreamClosed(IXmppStream *AXmppStream);
	//IPresencePlugin
	void onContactStateChanged(const Jid &streamJid, const Jid &contactJid, bool AStateOnline);
	//IOptionsHolder
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);

	void onSetMoodActionTriggered(bool);
	void onApplicationQuit();

protected:
	void addMood(const QString &name, const QString &locname);
	Action *createSetMoodAction(const Jid &streamJid, const QString &AFeature, QObject *AParent) const;
	void setContactMood(const Jid &streamJid, const Jid &senderJid, const Mood &mood);

	//IRosterDataHolder
	void updateDataHolder(const Jid &streamJid, const Jid &senderJid);

private:
	IMainWindowPlugin *FMainWindowPlugin;
	IPresencePlugin *FPresencePlugin;
	IPEPManager *FPEPManager;
	IServiceDiscovery *FDiscovery;
	IXmppStreams *FXmppStreams;
	IOptionsManager *FOptionsManager;
	IRoster *FRoster;
	IRosterPlugin *FRosterPlugin;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
	INotifications *FNotifications;

private:
	int handlerId;
	bool FMoodIconsVisible;
	quint32 FMoodLabelId;

	QMap<int, Jid> FNotifies;
	QHash<QString, MoodData> FMoods;
	QHash<Jid, QHash <QString, Mood> > FContacts;
};

#endif // USERMOOD_H
