#ifndef USERMOOD_H
#define USERMOOD_H


#include <definitions.h>

#include "usermooddialog.h"
#include "ui_usermooddialog.h"

#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ipepmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/ipresence.h>
#include <interfaces/iroster.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>

#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/rosterlabelorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/optionvalues.h>

#include <utils/options.h>
#include <utils/action.h>
#include <utils/filestorage.h>
#include <utils/widgetmanager.h>

#define USERMOOD_UUID "{df730f89-9cb1-472a-b61b-aea95594fde1}"
#define PEP_USERMOOD              4010

class MoodData
{
public:
    QString name;
    QIcon icon;
//    bool operator==(const MoodData &AOther) const {
//        return name==AOther.name;
//    }
//    bool operator!=(const MoodData &AOther) const {
//        return !operator==(AOther);
//    }
};


class UserMood :
        public QObject,
        public IPlugin,
        public IPEPHandler
    {
        Q_OBJECT;
        Q_INTERFACES(IPlugin IPEPHandler);

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

    //IPEPHandler
    virtual bool processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza);
    void isSetMood(const Jid &streamJid, const QString &moodKey, const QString &moodText);

protected slots:
//    void onOptionsOpened();
//    void onOptionsChanged(const OptionsNode &ANode);
//    void onRosterIndexInserted(const Jid &AContactJid, const QString &AMood);
    void onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
//    void onShowNotification(const QString &AContactJid);
//    void onNotificationActivated(int ANotifyId);
//    void onNotificationRemoved(int ANotifyId);
    void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu);
    void onSetMoodActionTriggered(bool);
    void onApplicationQuit();

protected:
    Action *createSetMoodAction(const Jid &AStreamJid, const QString &AFeature, QObject *AParent) const;
    void setContactMood(const QString &AContactJid, const QString &AMoodName, const QString &AMoodText);
    void setContactLabel();


private:
    IMainWindowPlugin *FMainWindowPlugin;
    IPresencePlugin *FPresencePlugin;
    IPEPManager *FPEPManager;
    IServiceDiscovery *FServiceDiscovery;
    IXmppStreams *FXmppStreams;
    IOptionsManager *FOptionsManager;
    IRosterPlugin *FRosterPlugin;
    IRostersModel *FRostersModel;
    IServiceDiscovery *FDiscovery;
    IRostersViewPlugin *FRostersViewPlugin;
    INotifications *FNotifications;

    int handlerId;
    int FUserMoodLabelId;

    //QMap<int, QVariant> FMoodCatalog;

    QMap<QString, MoodData> FMoodsCatalog;
    //QMap<Jid, QPair<QString, QString> > FStreamMood;
    QMap<QString, QPair<QString, QString> > FContactMood;

};

#endif // USERMOOD_H
