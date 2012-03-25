#include "usermood.h"

#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QDebug>

#define ADR_STREAM_JID                  Action::DR_StreamJid

UserMood::UserMood()
{
    FMainWindowPlugin = NULL;
    FPEPManager = NULL;
    FServiceDiscovery = NULL;
    FXmppStreams = NULL;
    FOptionsManager = NULL;
    FDiscovery = NULL;

}

UserMood::~UserMood()
{

}

void UserMood::pluginInfo(IPluginInfo *APluginInfo)
{
        APluginInfo->name = tr("User Mood");
        APluginInfo->description = tr("TLDR");
        APluginInfo->version = "0.1";
        APluginInfo->author = "Alexey Ivanov";
        APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins";
	APluginInfo->dependences.append(MAINWINDOW_UUID);
}

bool UserMood::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
    AInitOrder=501;

	IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

    plugin = APluginManager->pluginInterface("IPEPManager").value(0);
    if (plugin)
    {
        FPEPManager = qobject_cast<IPEPManager *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
    if (plugin)
    {
    FServiceDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
    if (plugin)
    {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
    if (plugin)
    {
            FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
            if (FPresencePlugin)
            {
            //        connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
              //              SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
            }
    }

    plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
    if (plugin)
    {
        FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
    if (plugin)
    {
        FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
    if (plugin)
    {
        FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    }

    plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
    if (plugin)
    {
        FNotifications = qobject_cast<INotifications *>(plugin->instance());
        if (FNotifications)
        {
            connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
            connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
        }
    }

    plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
    if (plugin)
    {
        FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
    }

    connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
    connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

    connect (APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));


	return FMainWindowPlugin!=NULL;
}

bool UserMood::initObjects()
{
    handlerId = FPEPManager->insertNodeHandler(MOOD_PROTOCOL_URL, this);

    IDiscoFeature feature;
    feature.active = true;
    feature.name = tr("User mood");
    feature.var = MOOD_PROTOCOL_URL;

    FServiceDiscovery->insertDiscoFeature(feature);

    feature.name = tr("User mood notification");
    feature.var = MOOD_NOTIFY_PROTOCOL_URL;
    FServiceDiscovery->insertDiscoFeature(feature);

    if (FRostersViewPlugin)
    {
	    connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)),
		    SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
	    connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)),
		    SLOT(onRosterIndexToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)));
    }

    if (FRostersViewPlugin)
    {
        IRostersLabel label;
        label.order = RLO_USERMOOD;
        label.value = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERMOOD);
        FUserMoodLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);
    }

	FMoodsCatalog.insert("_remove_mood", tr("Without Mood"));
    FMoodsCatalog.insert("afraid", tr("Afraid"));
    FMoodsCatalog.insert("amazed", tr("Amazed"));
    FMoodsCatalog.insert("angry", tr("Angry"));
    FMoodsCatalog.insert("amorous", tr("Amorous"));
    FMoodsCatalog.insert("annoyed", tr("Annoyed"));
    FMoodsCatalog.insert("anxious", tr("Anxious"));
    FMoodsCatalog.insert("aroused", tr("Aroused"));
    FMoodsCatalog.insert("ashamed", tr("Ashamed"));
    FMoodsCatalog.insert("bored", tr("Bored"));
    FMoodsCatalog.insert("brave", tr("Brave"));
    FMoodsCatalog.insert("calm", tr("Calm"));
    FMoodsCatalog.insert("cautious", tr("Cautious"));
    FMoodsCatalog.insert("cold", tr("Cold"));
    FMoodsCatalog.insert("confident", tr("Confident"));
    FMoodsCatalog.insert("confused", tr("Confused"));
    FMoodsCatalog.insert("contemplative", tr("Contemplative"));
    FMoodsCatalog.insert("contented", tr("Contented"));
    FMoodsCatalog.insert("cranky", tr("Cranky"));
    FMoodsCatalog.insert("crazy", tr("Crazy"));
    FMoodsCatalog.insert("creative", tr("Creative"));
    FMoodsCatalog.insert("curious", tr("Curious"));
    FMoodsCatalog.insert("dejected", tr("Dejected"));
    FMoodsCatalog.insert("depressed", tr("Depressed"));
    FMoodsCatalog.insert("disappointed", tr("Disappointed"));
    FMoodsCatalog.insert("disgusted", tr("Disgusted"));
    FMoodsCatalog.insert("dismayed", tr("Dismayed"));
    FMoodsCatalog.insert("distracted", tr("Distracted"));
    FMoodsCatalog.insert("embarrassed", tr("Embarrassed"));
    FMoodsCatalog.insert("envious", tr("Envious"));
    FMoodsCatalog.insert("excited", tr("Excited"));
    FMoodsCatalog.insert("flirtatious", tr("Flirtatious"));
    FMoodsCatalog.insert("frustrated", tr("Frustrated"));
    FMoodsCatalog.insert("grumpy", tr("Grumpy"));
    FMoodsCatalog.insert("guilty", tr("Guilty"));
    FMoodsCatalog.insert("happy", tr("Happy"));
    FMoodsCatalog.insert("hopeful", tr("Hopeful"));
    FMoodsCatalog.insert("hot", tr("Hot"));
    FMoodsCatalog.insert("humbled", tr("Humbled"));
    FMoodsCatalog.insert("humiliated", tr("Humiliated"));
    FMoodsCatalog.insert("hungry", tr("Hungry"));
    FMoodsCatalog.insert("hurt", tr("Hurt"));
    FMoodsCatalog.insert("impressed", tr("Impressed"));
    FMoodsCatalog.insert("in_awe", tr("In awe"));
    FMoodsCatalog.insert("in_love", tr("In love"));
    FMoodsCatalog.insert("indignant", tr("Indignant"));
    FMoodsCatalog.insert("interested", tr("Interested"));
    FMoodsCatalog.insert("intoxicated", tr("Intoxicated"));
    FMoodsCatalog.insert("invincible", tr("Invincible"));
    FMoodsCatalog.insert("jealous", tr("Jealous"));
    FMoodsCatalog.insert("lonely", tr("Lonely"));
    FMoodsCatalog.insert("lucky", tr("Lucky"));
    FMoodsCatalog.insert("mean", tr("Mean"));
    FMoodsCatalog.insert("moody", tr("Moody"));
    FMoodsCatalog.insert("nervous", tr("Nervous"));
    FMoodsCatalog.insert("neutral", tr("Neutral"));
    FMoodsCatalog.insert("offended", tr("Offended"));
    FMoodsCatalog.insert("outraged", tr("Outraged"));
    FMoodsCatalog.insert("playful", tr("Playful"));
    FMoodsCatalog.insert("proud", tr("Proud"));
    FMoodsCatalog.insert("relaxed", tr("Relaxed"));
    FMoodsCatalog.insert("relieved", tr("Relieved"));
    FMoodsCatalog.insert("remorseful", tr("Remorseful"));
    FMoodsCatalog.insert("restless", tr("Restless"));
    FMoodsCatalog.insert("sad", tr("Sad"));
    FMoodsCatalog.insert("sarcastic", tr("Sarcastic"));
    FMoodsCatalog.insert("serious", tr("Serious"));
    FMoodsCatalog.insert("shocked", tr("Shocked"));
    FMoodsCatalog.insert("shy", tr("Shy"));
    FMoodsCatalog.insert("sick", tr("Sick"));
    FMoodsCatalog.insert("sleepy", tr("Sleepy"));
    FMoodsCatalog.insert("spontaneous", tr("Spontaneous"));
    FMoodsCatalog.insert("stressed", tr("Stressed"));
    FMoodsCatalog.insert("strong", tr("Strong"));
    FMoodsCatalog.insert("surprised", tr("Surprised"));
    FMoodsCatalog.insert("thankful", tr("Thankful"));
    FMoodsCatalog.insert("thirsty", tr("Thirsty"));
    FMoodsCatalog.insert("tired", tr("Tired"));
    FMoodsCatalog.insert("undefined", tr("Undefined"));
    FMoodsCatalog.insert("weak", tr("Weak"));
    FMoodsCatalog.insert("worried", tr("Worried"));

    return true;
}

bool UserMood::processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);

    QString senderJid;
    QString moodName;
    QString moodText;

    QDomElement replyElem = AStanza.document().firstChildElement("message");

    if (!replyElem.isNull())
    {
        senderJid = replyElem.attribute("from");
        QDomElement eventElem = replyElem.firstChildElement("event");

        if (!eventElem.isNull())
        {
            QDomElement itemsElem = eventElem.firstChildElement("items");

            if (!itemsElem.isNull())
            {
                QDomElement itemElem = itemsElem.firstChildElement("item");

                if (!itemElem.isNull())
                {
                    QDomElement moodElem = itemElem.firstChildElement("mood");

                    if (!moodElem.isNull())
                    {
                        QDomElement choiseElem = moodElem.firstChildElement();

                        if (!choiseElem.isNull() && FMoodsCatalog.contains(moodElem.firstChildElement().nodeName()))
                        {
                            moodName = moodElem.firstChildElement().nodeName();
                        }

                        QDomElement textElem = moodElem.firstChildElement("text");

                        if (!moodElem.isNull())
                        {
                            moodText = textElem.text();
                        }
                    }
                }
            }
        }
    }
    setContactMood(senderJid,moodName,moodText);

    return true;
}

void UserMood::isSetMood(const Jid &streamJid, const QString &moodName, const QString &moodText)
{
    QDomDocument doc("");
    QDomElement root = doc.createElement("item");
    doc.appendChild(root);

    QDomElement mood = doc.createElementNS(MOOD_PROTOCOL_URL, "mood"); 
    root.appendChild(mood);

	if (moodName != FMoodsCatalog.value("_remove_mood"))
	{
		QDomElement name = doc.createElement(FMoodsCatalog.key(moodName));
		mood.appendChild(name);
	}
	else
	{
		QDomElement name = doc.createElement("");
		mood.appendChild(name);
	}

	if (moodName != FMoodsCatalog.value("_remove_mood"))
	{
		QDomElement text = doc.createElement("text");
		mood.appendChild(text);

		QDomText t1 = doc.createTextNode(moodText);
		text.appendChild(t1);
	}

    qDebug() << doc.toString();

    FPEPManager->publishItem(streamJid, MOOD_PROTOCOL_URL, root);
}

void UserMood::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
        if (ALabelId==RLID_DISPLAY && AIndexes.count()==1)
        {
                IRosterIndex *index = AIndexes.first();
                if (index->type() == RIT_STREAM_ROOT)
                {
                        Jid streamJid = index->data(RDR_STREAM_JID).toString();
                        IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
                        if (presence && presence->isOpen())
                        {
							    Jid contactJid = index->data(RDR_FULL_JID).toString();
                                int show = index->data(RDR_SHOW).toInt();
                                QStringList features = FDiscovery!=NULL ? FDiscovery->discoInfo(streamJid,contactJid).features : QStringList();
                                if (show!=IPresence::Offline && show!=IPresence::Error && !features.contains(MOOD_PROTOCOL_URL))
                                {
                                        Action *action = createSetMoodAction(streamJid,MOOD_PROTOCOL_URL,AMenu);
                                        AMenu->addAction(action,100,true);
                                }
                        }
                }
        }
}


Action *UserMood::createSetMoodAction(const Jid &AStreamJid, const QString &AFeature, QObject *AParent) const
{
       if (AFeature == MOOD_PROTOCOL_URL)
        {
                Action *action = new Action(AParent);
                action->setText(tr("Mood"));
                action->setIcon(RSR_STORAGE_MENUICONS,MNI_USERMOOD);
				action->setData(ADR_STREAM_JID,AStreamJid.full());
                connect(action,SIGNAL(triggered(bool)),SLOT(onSetMoodActionTriggered(bool)));
                return action;
        }
        return NULL;
}

void UserMood::onSetMoodActionTriggered(bool)
{
        Action *action = qobject_cast<Action *>(sender());
        if (action)
        {
            Jid streamJid = action->data(ADR_STREAM_JID).toString();
			userMoodDialog *dialog;
            dialog = new userMoodDialog(FMoodsCatalog,FContactMood,streamJid,this);
                    //userMoodDialog[streamJid].insert(contactJid,dialog);
                    //connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onEditNoteDialogDestroyed()));
            WidgetManager::showActivateRaiseWindow(dialog);
        }
}

void UserMood::setContactMood(const QString &AContactJid, const QString &AMoodName, const QString &AMoodText)
{
    if ((FContactMood.value(AContactJid).first != AMoodName) || FContactMood.value(AContactJid).second != AMoodText)
    {
        if (!AMoodName.isEmpty())
            FContactMood.insert(AContactJid,QPair<QString, QString>(AMoodName,AMoodText));
        else
            FContactMood.remove(AContactJid);
    }
    setContactLabel();
}

void UserMood::setContactLabel()
{
    foreach (const QString &AContactJid, FContactMood.keys())
    {

        QMultiMap<int, QVariant> findData;
        findData.insert(RDR_TYPE,RIT_CONTACT);
        findData.insert(RDR_PREP_BARE_JID,AContactJid);
        foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
            if (!FContactMood.value(AContactJid).first.isEmpty() && (AContactJid == index->data(RDR_PREP_BARE_JID).toString())) /*Options::node(OPV_UT_SHOW_ROSTER_LABEL).value().toBool()*/

            FRostersViewPlugin->rostersView()->insertLabel(FUserMoodLabelId,index);
        else
            FRostersViewPlugin->rostersView()->removeLabel(FUserMoodLabelId,index);
    }
}

void UserMood::onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
    if (ALabelId==RLID_DISPLAY || ALabelId==FUserMoodLabelId)
    {
        QString contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
        if (!FContactMood.value(contactJid).first.isEmpty())
        {
            QString text = FContactMood.value(contactJid).second;
            QString tip = QString("%1 <div style='margin-left:10px;'>%2<br>%3</div>").arg(tr("Mood:")).arg(FMoodsCatalog.value(FContactMood.value(contactJid).first)).arg(text.replace("\n","<br>"));
            AToolTips.insert(RTTO_USERMOOD,tip);
        }
    }
}

void UserMood::onApplicationQuit()
{
    FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserMood)
