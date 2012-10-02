#include "usermood.h"

#include <QDebug>

#define ADR_STREAM_JID	Action::DR_StreamJid
#define RDR_MOOD_NAME	452

#define DIC_PUBSUB		"pubsub"
#define DIT_PEP			"pep"

UserMood::UserMood()
{
	FMainWindowPlugin = NULL;
	FPEPManager = NULL;
	FDiscovery = NULL;
	FXmppStreams = NULL;
	FOptionsManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;

	//FUserMoodLabelId = -1;

}

UserMood::~UserMood()
{

}

void UserMood::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("User Mood");
	APluginInfo->description = tr("Allows you to send and receive information about user moods");
	APluginInfo->version = "0.2";
	APluginInfo->author = "Alexey Ivanov aka krab";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins";
	APluginInfo->dependences.append(PEPMANAGER_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(PRESENCE_UUID);
}

bool UserMood::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder = 11;

	IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0);
	if(plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPEPManager").value(0);
	if(plugin)
	{
		FPEPManager = qobject_cast<IPEPManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0, NULL);
	if(plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0, NULL);
	if(plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0, NULL);
	if(plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if(FPresencePlugin)
		{
			//        connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
			//              SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0, NULL);
	if(plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if(FRostersModel)
		{
			connect(FRostersModel->instance(), SIGNAL(indexInserted(IRosterIndex *)), SLOT(onRosterIndexInserted(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0, NULL);
	if(plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0, NULL);
	if(plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if(FNotifications)
		{
			connect(FNotifications->instance(), SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(), SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0, NULL);
	if(plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	connect(Options::instance(), SIGNAL(optionsOpened()), SLOT(onOptionsOpened()));
	connect(Options::instance(), SIGNAL(optionsChanged(const OptionsNode &)), SLOT(onOptionsChanged(const OptionsNode &)));

	connect(APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));


	return FMainWindowPlugin != NULL;
}

bool UserMood::initObjects()
{
	handlerId = FPEPManager->insertNodeHandler(MOOD_PROTOCOL_URL, this);

	IDiscoFeature feature;
	feature.active = true;
	feature.name = tr("User mood");
	feature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERMOOD);
	feature.description = tr("Supports the exchange of information about user moods");
	feature.var = MOOD_PROTOCOL_URL;

	FDiscovery->insertDiscoFeature(feature);

	feature.name = tr("User mood notification");
	feature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERMOOD);
	feature.description = tr("Supports the exchange of information about user moods");
	feature.var = MOOD_NOTIFY_PROTOCOL_URL;
	FDiscovery->insertDiscoFeature(feature);

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_USERMOOD_NOTIFY;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERMOOD);
		notifyType.title = tr("When receiving mood");
		notifyType.kindMask = INotification::PopupWindow;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_USERMOOD,notifyType);
	}

	if(FRostersModel)
	{
		FRostersModel->insertDefaultDataHolder(this);
	}

	if(FRostersViewPlugin)
	{
		connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)),SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
		connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(indexToolTips(IRosterIndex *, int , QMultiMap<int, QString> &)),SLOT(onRosterIndexToolTips(IRosterIndex *, int , QMultiMap<int, QString> &)));
	}

	if(FRostersViewPlugin)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, rosterDataTypes())
		findData.insertMulti(RDR_TYPE, type);
		QList<IRosterIndex *> indexes = FRostersModel->rootIndex()->findChilds(findData, true);

		IRostersLabel label;
		label.order = RLO_USERMOOD;
		label.value = RDR_MOOD_NAME;
		FUserMoodLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);

		foreach(IRosterIndex * index, indexes)
		FRostersViewPlugin->rostersView()->insertLabel(FUserMoodLabelId, index);
	}

	FMoodsCatalog.insert(MOOD_NULL, MoodData(tr("Without mood")));
	FMoodsCatalog.insert(MOOD_AFRAID, MoodData(UMI_AFRAID, tr("Afraid")));
	FMoodsCatalog.insert(MOOD_AMAZED, MoodData(UMI_AMAZED, tr("Amazed")));
	FMoodsCatalog.insert(MOOD_ANGRY, MoodData(UMI_ANGRY, tr("Angry")));
	FMoodsCatalog.insert(MOOD_AMOROUS, MoodData(UMI_AMOROUS, tr("Amorous")));
	FMoodsCatalog.insert(MOOD_ANNOYED, MoodData(UMI_ANNOYED, tr("Annoyed")));
	FMoodsCatalog.insert(MOOD_ANXIOUS, MoodData(UMI_ANXIOUS, tr("Anxious")));
	FMoodsCatalog.insert(MOOD_AROUSED, MoodData(UMI_AROUSED, tr("Aroused")));
	FMoodsCatalog.insert(MOOD_ASHAMED, MoodData(UMI_ASHAMED, tr("Ashamed")));
	FMoodsCatalog.insert(MOOD_BORED, MoodData(UMI_BORED, tr("Bored")));
	FMoodsCatalog.insert(MOOD_BRAVE, MoodData(UMI_BRAVE, tr("Brave")));
	FMoodsCatalog.insert(MOOD_CALM, MoodData(UMI_CALM, tr("Calm")));
	FMoodsCatalog.insert(MOOD_CAUTIOUS, MoodData(UMI_CAUTIOUS, tr("Cautious")));
	FMoodsCatalog.insert(MOOD_COLD, MoodData(UMI_COLD, tr("Cold")));
	FMoodsCatalog.insert(MOOD_CONFIDENT, MoodData(UMI_CONFIDENT, tr("Confident")));
	FMoodsCatalog.insert(MOOD_CONFUSED, MoodData(UMI_CONFUSED, tr("Confused")));
	FMoodsCatalog.insert(MOOD_CONTEMPLATIVE, MoodData(UMI_CONTEMPLATIVE, tr("Contemplative")));
	FMoodsCatalog.insert(MOOD_CONTENTED, MoodData(UMI_CONTENTED, tr("Contented")));
	FMoodsCatalog.insert(MOOD_CRANKY, MoodData(UMI_CRANKY, tr("Cranky")));
	FMoodsCatalog.insert(MOOD_CRAZY, MoodData(UMI_CRAZY, tr("Crazy")));
	FMoodsCatalog.insert(MOOD_CREATIVE, MoodData(UMI_CREATIVE, tr("Creative")));
	FMoodsCatalog.insert(MOOD_CURIOUS, MoodData(UMI_CURIOUS, tr("Curious")));
	FMoodsCatalog.insert(MOOD_DEJECTED, MoodData(UMI_DEJECTED, tr("Dejected")));
	FMoodsCatalog.insert(MOOD_DEPRESSED, MoodData(UMI_DEPRESSED, tr("Depressed")));
	FMoodsCatalog.insert(MOOD_DISAPPOINTED, MoodData(UMI_DISAPPOINTED, tr("Disappointed")));
	FMoodsCatalog.insert(MOOD_DISGUSTED, MoodData(UMI_DISGUSTED, tr("Disgusted")));
	FMoodsCatalog.insert(MOOD_DISMAYED, MoodData(UMI_DISMAYED, tr("Dismayed")));
	FMoodsCatalog.insert(MOOD_DISTRACTED, MoodData(UMI_DISTRACTED, tr("Distracted")));
	FMoodsCatalog.insert(MOOD_EMBARRASSED, MoodData(UMI_EMBARRASSED, tr("Embarrassed")));
	FMoodsCatalog.insert(MOOD_ENVIOUS, MoodData(UMI_ENVIOUS, tr("Envious")));
	FMoodsCatalog.insert(MOOD_EXCITED, MoodData(UMI_EXCITED, tr("Excited")));
	FMoodsCatalog.insert(MOOD_FLIRTATIOUS, MoodData(UMI_FLIRTATIOUS, tr("Flirtatious")));
	FMoodsCatalog.insert(MOOD_FRUSTRATED, MoodData(UMI_FRUSTRATED, tr("Frustrated")));
	FMoodsCatalog.insert(MOOD_GRUMPY, MoodData(UMI_GRUMPY, tr("Grumpy")));
	FMoodsCatalog.insert(MOOD_GUILTY, MoodData(UMI_GUILTY, tr("Guilty")));
	FMoodsCatalog.insert(MOOD_HAPPY, MoodData(UMI_HAPPY, tr("Happy")));
	FMoodsCatalog.insert(MOOD_HOPEFUL, MoodData(UMI_HOPEFUL, tr("Hopeful")));
	FMoodsCatalog.insert(MOOD_HOT, MoodData(UMI_HOT, tr("Hot")));
	FMoodsCatalog.insert(MOOD_HUMBLED, MoodData(UMI_HUMBLED, tr("Humbled")));
	FMoodsCatalog.insert(MOOD_HUMILIATED, MoodData(UMI_HUMILIATED, tr("Humiliated")));
	FMoodsCatalog.insert(MOOD_HUNGRY, MoodData(UMI_HUNGRY, tr("Hungry")));
	FMoodsCatalog.insert(MOOD_HURT, MoodData(UMI_HURT, tr("Hurt")));
	FMoodsCatalog.insert(MOOD_IMPRESSED, MoodData(UMI_IMPRESSED, tr("Impressed")));
	FMoodsCatalog.insert(MOOD_IN_AWE, MoodData(UMI_IN_AWE, tr("In awe")));
	FMoodsCatalog.insert(MOOD_IN_LOVE, MoodData(UMI_IN_LOVE, tr("In love")));
	FMoodsCatalog.insert(MOOD_INDIGNANT, MoodData(UMI_INDIGNANT, tr("Indignant")));
	FMoodsCatalog.insert(MOOD_INTERESTED, MoodData(UMI_INTERESTED, tr("Interested")));
	FMoodsCatalog.insert(MOOD_INTOXICATED, MoodData(UMI_INTOXICATED, tr("Intoxicated")));
	FMoodsCatalog.insert(MOOD_INVINCIBLE, MoodData(UMI_INVINCIBLE, tr("Invincible")));
	FMoodsCatalog.insert(MOOD_JEALOUS, MoodData(UMI_JEALOUS, tr("Jealous")));
	FMoodsCatalog.insert(MOOD_LONELY, MoodData(UMI_LONELY, tr("Lonely")));
	FMoodsCatalog.insert(MOOD_LUCKY, MoodData(UMI_LUCKY, tr("Lucky")));
	FMoodsCatalog.insert(MOOD_MEAN, MoodData(UMI_MEAN, tr("Mean")));
	FMoodsCatalog.insert(MOOD_MOODY, MoodData(UMI_MOODY, tr("Moody")));
	FMoodsCatalog.insert(MOOD_NERVOUS, MoodData(UMI_NERVOUS, tr("Nervous")));
	FMoodsCatalog.insert(MOOD_NEUTRAL, MoodData(UMI_NEUTRAL, tr("Neutral")));
	FMoodsCatalog.insert(MOOD_OFFENDED, MoodData(UMI_OFFENDED, tr("Offended")));
	FMoodsCatalog.insert(MOOD_OUTRAGED, MoodData(UMI_OUTRAGED, tr("Outraged")));
	FMoodsCatalog.insert(MOOD_PLAYFUL, MoodData(UMI_PLAYFUL, tr("Playful")));
	FMoodsCatalog.insert(MOOD_PROUD, MoodData(UMI_PROUD, tr("Proud")));
	FMoodsCatalog.insert(MOOD_RELAXED, MoodData(UMI_RELAXED, tr("Relaxed")));
	FMoodsCatalog.insert(MOOD_RELIEVED, MoodData(UMI_RELIEVED, tr("Relieved")));
	FMoodsCatalog.insert(MOOD_REMORSEFUL, MoodData(UMI_REMORSEFUL, tr("Remorseful")));
	FMoodsCatalog.insert(MOOD_RESTLESS, MoodData(UMI_RESTLESS, tr("Restless")));
	FMoodsCatalog.insert(MOOD_SAD, MoodData(UMI_SAD, tr("Sad")));
	FMoodsCatalog.insert(MOOD_SARCASTIC, MoodData(UMI_SARCASTIC, tr("Sarcastic")));
	FMoodsCatalog.insert(MOOD_SERIOUS, MoodData(UMI_SERIOUS, tr("Serious")));
	FMoodsCatalog.insert(MOOD_SHOCKED, MoodData(UMI_SHOCKED, tr("Shocked")));
	FMoodsCatalog.insert(MOOD_SHY, MoodData(UMI_SHY, tr("Shy")));
	FMoodsCatalog.insert(MOOD_SICK, MoodData(UMI_SICK, tr("Sick")));
	FMoodsCatalog.insert(MOOD_SLEEPY, MoodData(UMI_SLEEPY, tr("Sleepy")));
	FMoodsCatalog.insert(MOOD_SPONTANEOUS, MoodData(UMI_SPONTANEOUS, tr("Spontaneous")));
	FMoodsCatalog.insert(MOOD_STRESSED, MoodData(UMI_STRESSED, tr("Stressed")));
	FMoodsCatalog.insert(MOOD_STRONG, MoodData(UMI_STRONG, tr("Strong")));
	FMoodsCatalog.insert(MOOD_SURPRISED, MoodData(UMI_SURPRISED, tr("Surprised")));
	FMoodsCatalog.insert(MOOD_THANKFUL, MoodData(UMI_THANKFUL, tr("Thankful")));
	FMoodsCatalog.insert(MOOD_THIRSTY, MoodData(UMI_THIRSTY, tr("Thirsty")));
	FMoodsCatalog.insert(MOOD_TIRED, MoodData(UMI_TIRED, tr("Tired")));
	FMoodsCatalog.insert(MOOD_UNDEFINED, MoodData(UMI_UNDEFINED, tr("Undefined")));
	FMoodsCatalog.insert(MOOD_WEAK, MoodData(UMI_WEAK, tr("Weak"))); 
	FMoodsCatalog.insert(MOOD_WORRIED, MoodData(UMI_WORRIED, tr("Worried")));

	return true;
}


int UserMood::rosterDataOrder() const
{
	return RDHO_DEFAULT;
}

QList<int> UserMood::rosterDataRoles() const
{
	static const QList<int> indexRoles = QList<int>() << RDR_MOOD_NAME;
	return indexRoles;
}

QList<int> UserMood::rosterDataTypes() const
{
	static const QList<int> indexTypes = QList<int>() << RIT_STREAM_ROOT << RIT_CONTACT;
	return indexTypes;
}

QVariant UserMood::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	if(ARole == RDR_MOOD_NAME)
	{
		QIcon pic = contactMoodIcon(AIndex->data(RDR_PREP_BARE_JID).toString());
		return pic;
	}
	return QVariant();
}

bool UserMood::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

bool UserMood::processPEPEvent(const Jid &AStreamJid, const Stanza &AStanza)
{
	Jid ASenderJid;
	QString AMoodName;
	QString AMoodText;

	QDomElement replyElem = AStanza.document().firstChildElement("message");

	if(!replyElem.isNull())
	{
		ASenderJid = replyElem.attribute("from");
		QDomElement eventElem = replyElem.firstChildElement("event");
		if(!eventElem.isNull())
		{
			QDomElement itemsElem = eventElem.firstChildElement("items");
			if(!itemsElem.isNull())
			{
				QDomElement itemElem = itemsElem.firstChildElement("item");
				if(!itemElem.isNull())
				{
					QDomElement moodElem = itemElem.firstChildElement("mood");
					if(!moodElem.isNull())
					{
						QDomElement choiseElem = moodElem.firstChildElement();
						if(!choiseElem.isNull() && FMoodsCatalog.contains(moodElem.firstChildElement().nodeName()))
						{
							AMoodName = moodElem.firstChildElement().nodeName();
						}
						QDomElement textElem = moodElem.firstChildElement("text");
						if(!moodElem.isNull())
						{
							AMoodText = textElem.text();
						}
					}
				}
			}
		}
	}
	setContactMood(AStreamJid, ASenderJid, AMoodName, AMoodText);

	return true;
}

void UserMood::setMood(const Jid &AStreamJid, const QString &AMoodKey, const QString &AMoodText)
{
	QDomDocument doc("");
	QDomElement root = doc.createElement("item");
	doc.appendChild(root);

	QDomElement mood = doc.createElementNS(MOOD_PROTOCOL_URL, "mood");
	root.appendChild(mood);
	if(AMoodKey != MOOD_NULL)
	{
		QDomElement name = doc.createElement(AMoodKey);
		mood.appendChild(name);
	}
	else
	{
		QDomElement name = doc.createElement("");
		mood.appendChild(name);
	}
	if(AMoodKey != MOOD_NULL)
	{
		QDomElement text = doc.createElement("text");
		mood.appendChild(text);

		QDomText t1 = doc.createTextNode(AMoodText);
		text.appendChild(t1);
	}
	FPEPManager->publishItem(AStreamJid, MOOD_PROTOCOL_URL, root);
}

void UserMood::onShowNotification(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FNotifications && /*FNotifications->notifications().isEmpty() &&*/ FContactsMood.contains(AContactJid.pBare()) /*&& AContactJid.pBare() != AStreamJid.pBare()*/)
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERMOOD);
		if ((notify.kinds & INotification::PopupWindow) > 0)
		{
			notify.typeId = NNT_USERMOOD;
			notify.data.insert(NDR_ICON,contactMoodIcon(AContactJid));
			notify.data.insert(NDR_POPUP_CAPTION,tr("User Mood Notification"));
			notify.data.insert(NDR_POPUP_TITLE,QString("%1 %2").arg(FNotifications->contactName(AStreamJid, AContactJid)).arg(tr("changed mood")));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AContactJid));
			notify.data.insert(NDR_POPUP_HTML,QString("[%1] %2").arg(contactMoodName(AContactJid)).arg(contactMoodText(AContactJid)));
			FNotifies.insert(FNotifications->appendNotification(notify),AContactJid);
		}
	}
}

void UserMood::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifications->removeNotification(ANotifyId);
	}
}

void UserMood::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifies.remove(ANotifyId);
	}
}

bool UserMood::isSupported(const Jid &AStreamJid) const
{
	bool supported = false;
	IDiscoInfo dinfo = FDiscovery!=NULL ? FDiscovery->discoInfo(AStreamJid, AStreamJid.domain()) : IDiscoInfo();
	for (int i=0; !supported && i<dinfo.identity.count(); i++)
	{
		const IDiscoIdentity &ident = dinfo.identity.at(i);
		supported = ident.category==DIC_PUBSUB && ident.type==DIT_PEP;
	}
	return supported;
}

void UserMood::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
	if(ALabelId == RLID_DISPLAY && AIndexes.count() == 1)
	{
		IRosterIndex *index = AIndexes.first();
		if(index->type() == RIT_STREAM_ROOT)
		{
			Jid AStreamJid = index->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresencePlugin != NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
			if(presence && presence->isOpen())
			{
				int show = index->data(RDR_SHOW).toInt();
				if(show != IPresence::Offline && show != IPresence::Error && isSupported(AStreamJid))
				{
					Action *action = createSetMoodAction(AStreamJid, MOOD_PROTOCOL_URL, AMenu);
					AMenu->addAction(action, AG_RVCM_USERMOOD, false);
				}
			}
		}
	}
}

Action *UserMood::createSetMoodAction(const Jid &AStreamJid, const QString &AFeature, QObject *AParent) const
{
	if(AFeature == MOOD_PROTOCOL_URL)
	{
		Action *action = new Action(AParent);
		action->setText(tr("Mood"));
		QIcon menuicon;
		if(!contactMoodIcon(AStreamJid).isNull())
			menuicon = contactMoodIcon(AStreamJid);
		else
			menuicon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERMOOD);
		action->setIcon(menuicon);
		action->setData(ADR_STREAM_JID, AStreamJid.full());
		connect(action, SIGNAL(triggered(bool)), SLOT(onSetMoodActionTriggered(bool)));
		return action;
	}
	return NULL;
}

void UserMood::onSetMoodActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if(action)
	{
		Jid AStreamJid = action->data(ADR_STREAM_JID).toString();
		UserMoodDialog *dialog;
		dialog = new UserMoodDialog(this, FMoodsCatalog, AStreamJid);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void UserMood::setContactMood(const Jid &AStreamJid, const Jid &ASenderJid, const QString &AMoodName, const QString &AMoodText)
{
	if((contactMoodKey(ASenderJid) != AMoodName) || contactMoodText(ASenderJid) != AMoodText)
	{
		if(!AMoodName.isEmpty())
		{
			MoodContact data;
			data.keyname = AMoodName;
			data.text = AMoodText;
			FContactsMood.insert(ASenderJid.pBare(), data);
			onShowNotification(AStreamJid, ASenderJid);
		}
		else
			FContactsMood.remove(ASenderJid.pBare());
	}
	updateDataHolder(ASenderJid);
}

void UserMood::updateDataHolder(const Jid &ASenderJid)
{
	if(FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, rosterDataTypes())
		findData.insert(RDR_TYPE, type);
		if(!ASenderJid.isEmpty())
			findData.insert(RDR_PREP_BARE_JID, ASenderJid.pBare());
		QList<IRosterIndex *> indexes = FRostersModel->rootIndex()->findChilds(findData, true);
		foreach(IRosterIndex *index, indexes)
		{
			emit rosterDataChanged(index, RDR_MOOD_NAME);
		}
	}
}

void UserMood::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if(FRostersViewPlugin && rosterDataTypes().contains(AIndex->type()))
	{
		FRostersViewPlugin->rostersView()->insertLabel(FUserMoodLabelId, AIndex);
	}
}

void UserMood::onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int, QString> &AToolTips)
{
	if(ALabelId == RLID_DISPLAY || ALabelId == FUserMoodLabelId)
	{
		Jid AContactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if(!contactMoodKey(AContactJid).isEmpty())
		{
			QString tooltip_full = QString("%1 <div style='margin-left:10px;'>%2<br>%3</div>")
					.arg(tr("Mood:")).arg(contactMoodName(AContactJid)).arg(contactMoodText(AContactJid));
			QString tooltip_short = QString("%1 <div style='margin-left:10px;'>%2</div>")
					.arg(tr("Mood:")).arg(contactMoodName(AContactJid));
			AToolTips.insert(RTTO_USERMOOD, contactMoodText(AContactJid).isEmpty() ? tooltip_short : tooltip_full);
		}
	}
}

QIcon UserMood::moodIcon(const QString &keyname) const
{
	return FMoodsCatalog.value(keyname).icon;
}

QString UserMood::moodName(const QString &keyname) const
{
	return FMoodsCatalog.value(keyname).locname;
}

QString UserMood::contactMoodKey(const Jid &contactJid) const
{
	return FContactsMood.value(contactJid.pBare()).keyname;
}

QIcon UserMood::contactMoodIcon(const Jid &contactJid) const
{
	return FMoodsCatalog.value(FContactsMood.value(contactJid.pBare()).keyname).icon;
}

QString UserMood::contactMoodName(const Jid &contactJid) const
{
	return FMoodsCatalog.value(FContactsMood.value(contactJid.pBare()).keyname).locname;
}

QString UserMood::contactMoodText(const Jid &contactJid) const
{
	QString text = FContactsMood.value(contactJid.pBare()).text;
	return text.replace("\n", "<br>");
}

void UserMood::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserMood)
