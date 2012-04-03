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

    MoodData data;
    data.name = tr("Without mood");
    FMoodsCatalog.insert(MOOD_NULL, data);
    data.name = tr("Afraid");
    FMoodsCatalog.insert(MOOD_AFRAID, data);
    data.name = tr("Amazed");
    FMoodsCatalog.insert(MOOD_AMAZED, data);
    data.name = tr("Angry");
    FMoodsCatalog.insert(MOOD_ANGRY, data);
    data.name = tr("Amorous");
    FMoodsCatalog.insert(MOOD_AMOROUS, data);
    data.name = tr("Annoyed");
    FMoodsCatalog.insert(MOOD_ANNOYED, data);
    data.name = tr("Anxious");
    FMoodsCatalog.insert(MOOD_ANXIOUS, data);
    data.name = tr("Aroused");
    FMoodsCatalog.insert(MOOD_AROUSED, data);
    data.name = tr("Ashamed");
    FMoodsCatalog.insert(MOOD_ASHAMED, data);
    data.name = tr("Bored");
    FMoodsCatalog.insert(MOOD_BORED, data);
    data.name = tr("Brave");
    FMoodsCatalog.insert(MOOD_BRAVE, data);
    data.name = tr("Calm");
    FMoodsCatalog.insert(MOOD_CALM, data);
    data.name = tr("Cautious");
    FMoodsCatalog.insert(MOOD_CAUTIOUS, data);
    data.name = tr("Cold");
    FMoodsCatalog.insert(MOOD_COLD, data);
    data.name = tr("Confident");
    FMoodsCatalog.insert(MOOD_CONFIDENT, data);
    data.name = tr("Confused");
    FMoodsCatalog.insert(MOOD_CONFUSED, data);
    data.name = tr("Contemplative");
    FMoodsCatalog.insert(MOOD_CONTEMPLATIVE, data);
    data.name = tr("Contented");
    FMoodsCatalog.insert(MOOD_CONTENTED, data);
    data.name = tr("Cranky");
    FMoodsCatalog.insert(MOOD_CRANKY, data);
    data.name = tr("Crazy");
    FMoodsCatalog.insert(MOOD_CRAZY, data);
    data.name = tr("Creative");
    FMoodsCatalog.insert(MOOD_CREATIVE, data);
    data.name = tr("Curious");
    FMoodsCatalog.insert(MOOD_CURIOUS, data);
    data.name = tr("Dejected");
    FMoodsCatalog.insert(MOOD_DEJECTED, data);
    data.name = tr("Depressed");
    FMoodsCatalog.insert(MOOD_DEPRESSED, data);
    data.name = tr("Disappointed");
    FMoodsCatalog.insert(MOOD_DISAPPOINTED, data);
    data.name = tr("Disgusted");
    FMoodsCatalog.insert(MOOD_DISGUSTED, data);
    data.name = tr("Dismayed");
    FMoodsCatalog.insert(MOOD_DISMAYED, data);
    data.name = tr("Distracted");
    FMoodsCatalog.insert(MOOD_DISTRACTED, data);
    data.name = tr("Embarrassed");
    FMoodsCatalog.insert(MOOD_EMBARRASSED, data);
    data.name = tr("Envious");
    FMoodsCatalog.insert(MOOD_ENVIOUS, data);
    data.name = tr("Excited");
    FMoodsCatalog.insert(MOOD_EXCITED, data);
    data.name = tr("Flirtatious");
    FMoodsCatalog.insert(MOOD_FLIRTATIOUS, data);
    data.name = tr("Frustrated");
    FMoodsCatalog.insert(MOOD_FRUSTRATED, data);
    data.name = tr("Grumpy");
    FMoodsCatalog.insert(MOOD_GRUMPY, data);
    data.name = tr("Guilty");
    FMoodsCatalog.insert(MOOD_GUILTY, data);
    data.name = tr("Happy");
    FMoodsCatalog.insert(MOOD_HAPPY, data);
    data.name = tr("Hopeful");
    FMoodsCatalog.insert(MOOD_HOPEFUL, data);
    data.name = tr("Hot");
    FMoodsCatalog.insert(MOOD_HOT, data);
    data.name = tr("Humbled");
    FMoodsCatalog.insert(MOOD_HUMBLED, data);
    data.name = tr("Humiliated");
    FMoodsCatalog.insert(MOOD_HUMILIATED, data);
    data.name = tr("Hungry");
    FMoodsCatalog.insert(MOOD_HUNGRY, data);
    data.name = tr("Hurt");
    FMoodsCatalog.insert(MOOD_HURT, data);
    data.name = tr("Impressed");
    FMoodsCatalog.insert(MOOD_IMPRESSED, data);
    data.name = tr("In awe");
    FMoodsCatalog.insert(MOOD_IN_AWE, data);
    data.name = tr("In love");
    FMoodsCatalog.insert(MOOD_IN_LOVE, data);
    data.name = tr("Indignant");
    FMoodsCatalog.insert(MOOD_INDIGNANT, data);
    data.name = tr("Interested");
    FMoodsCatalog.insert(MOOD_INTERESTED, data);
    data.name = tr("Intoxicated");
    FMoodsCatalog.insert(MOOD_INTOXICATED, data);
    data.name = tr("Invincible");
    FMoodsCatalog.insert(MOOD_INVINCIBLE, data);
    data.name = tr("Jealous");
    FMoodsCatalog.insert(MOOD_JEALOUS, data);
    data.name = tr("Lonely");
    FMoodsCatalog.insert(MOOD_LONELY, data);
    data.name = tr("Lucky");
    FMoodsCatalog.insert(MOOD_LUCKY, data);
    data.name = tr("Mean");
    FMoodsCatalog.insert(MOOD_MEAN, data);
    data.name = tr("Moody");
    FMoodsCatalog.insert(MOOD_MOODY, data);
    data.name = tr("Nervous");
    FMoodsCatalog.insert(MOOD_NERVOUS, data);
    data.name = tr("Neutral");
    FMoodsCatalog.insert(MOOD_NEUTRAL, data);
    data.name = tr("Offended");
    FMoodsCatalog.insert(MOOD_OFFENDED, data);
    data.name = tr("Outraged");
    FMoodsCatalog.insert(MOOD_OUTRAGED, data);
    data.name = tr("Playful");
    FMoodsCatalog.insert(MOOD_PLAYFUL, data);
    data.name = tr("Proud");
    FMoodsCatalog.insert(MOOD_PROUD, data);
    data.name = tr("Relaxed");
    FMoodsCatalog.insert(MOOD_RELAXED, data);
    data.name = tr("Relieved");
    FMoodsCatalog.insert(MOOD_RELIEVED, data);
    data.name = tr("Remorseful");
    FMoodsCatalog.insert(MOOD_REMORSEFUL, data);
    data.name = tr("Restless");
    FMoodsCatalog.insert(MOOD_RESTLESS, data);
    data.name = tr("Sad");
    FMoodsCatalog.insert(MOOD_SAD, data);
    data.name = tr("Sarcastic");
    FMoodsCatalog.insert(MOOD_SARCASTIC, data);
    data.name = tr("Serious");
    FMoodsCatalog.insert(MOOD_SERIOUS, data);
    data.name = tr("Shocked");
    FMoodsCatalog.insert(MOOD_SHOCKED, data);
    data.name = tr("Shy");
    FMoodsCatalog.insert(MOOD_SHY, data);
    data.name = tr("Sick");
    FMoodsCatalog.insert(MOOD_SICK, data);
    data.name = tr("Sleepy");
    FMoodsCatalog.insert(MOOD_SLEEPY, data);
    data.name = tr("Spontaneous");
    FMoodsCatalog.insert(MOOD_SPONTANEOUS, data);
    data.name = tr("Stressed");
    FMoodsCatalog.insert(MOOD_STRESSED, data);
    data.name = tr("Strong");
    FMoodsCatalog.insert(MOOD_STRONG, data);
    data.name = tr("Surprised");
    FMoodsCatalog.insert(MOOD_SURPRISED, data);
    data.name = tr("Thankful");
    FMoodsCatalog.insert(MOOD_THANKFUL, data);
    data.name = tr("Thirsty");
    FMoodsCatalog.insert(MOOD_THIRSTY, data);
    data.name = tr("Tired");
    FMoodsCatalog.insert(MOOD_TIRED, data);
    data.name = tr("Undefined");
    FMoodsCatalog.insert(MOOD_UNDEFINED, data);
    data.name = tr("Weak");
    FMoodsCatalog.insert(MOOD_WEAK, data);
    data.name = tr("Worried");
    FMoodsCatalog.insert(MOOD_WORRIED, data);

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

void UserMood::isSetMood(const Jid &streamJid, const QString &moodKey, const QString &moodText)
{
    QDomDocument doc("");
    QDomElement root = doc.createElement("item");
    doc.appendChild(root);

    QDomElement mood = doc.createElementNS(MOOD_PROTOCOL_URL, "mood"); 
    root.appendChild(mood);

    if (moodKey != MOOD_NULL)
	{
        QDomElement name = doc.createElement(moodKey);
		mood.appendChild(name);
	}
	else
    {
		QDomElement name = doc.createElement("");
		mood.appendChild(name);
	}

    if (moodKey != MOOD_NULL)
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
            QString tip = QString("%1 <div style='margin-left:10px;'>%2<br>%3</div>").arg(tr("Mood:")).arg(FMoodsCatalog.value(FContactMood.value(contactJid).first).name).arg(text.replace("\n","<br>"));
            AToolTips.insert(RTTO_USERMOOD,tip);
        }
    }
}

void UserMood::onApplicationQuit()
{
    FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserMood)
