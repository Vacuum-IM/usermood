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

void UserMood::addMood(const QString &name, const QString &locname)
{
	FMoodsCatalog.insert(name, MoodData(locname));
}

void UserMood::addMood(const QString &name, const QString &icon, const QString &locname)
{
	FMoodsCatalog.insert(name, MoodData(icon, locname));
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

	addMood(MOOD_NULL, tr("Without mood"));

	addMood(MOOD_AFRAID, UMI_AFRAID, tr("Afraid"));
	addMood(MOOD_AMAZED, UMI_AMAZED, tr("Amazed"));
	addMood(MOOD_ANGRY, UMI_ANGRY, tr("Angry"));
	addMood(MOOD_AMOROUS, UMI_AMOROUS, tr("Amorous"));
	addMood(MOOD_ANNOYED, UMI_ANNOYED, tr("Annoyed"));
	addMood(MOOD_ANXIOUS, UMI_ANXIOUS, tr("Anxious"));
	addMood(MOOD_AROUSED, UMI_AROUSED, tr("Aroused"));
	addMood(MOOD_ASHAMED, UMI_ASHAMED, tr("Ashamed"));
	addMood(MOOD_BORED, UMI_BORED, tr("Bored"));
	addMood(MOOD_BRAVE, UMI_BRAVE, tr("Brave"));
	addMood(MOOD_CALM, UMI_CALM, tr("Calm"));
	addMood(MOOD_CAUTIOUS, UMI_CAUTIOUS, tr("Cautious"));
	addMood(MOOD_COLD, UMI_COLD, tr("Cold"));
	addMood(MOOD_CONFIDENT, UMI_CONFIDENT, tr("Confident"));
	addMood(MOOD_CONFUSED, UMI_CONFUSED, tr("Confused"));
	addMood(MOOD_CONTEMPLATIVE, UMI_CONTEMPLATIVE, tr("Contemplative"));
	addMood(MOOD_CONTENTED, UMI_CONTENTED, tr("Contented"));
	addMood(MOOD_CRANKY, UMI_CRANKY, tr("Cranky"));
	addMood(MOOD_CRAZY, UMI_CRAZY, tr("Crazy"));
	addMood(MOOD_CREATIVE, UMI_CREATIVE, tr("Creative"));
	addMood(MOOD_CURIOUS, UMI_CURIOUS, tr("Curious"));
	addMood(MOOD_DEJECTED, UMI_DEJECTED, tr("Dejected"));
	addMood(MOOD_DEPRESSED, UMI_DEPRESSED, tr("Depressed"));
	addMood(MOOD_DISAPPOINTED, UMI_DISAPPOINTED, tr("Disappointed"));
	addMood(MOOD_DISGUSTED, UMI_DISGUSTED, tr("Disgusted"));
	addMood(MOOD_DISMAYED, UMI_DISMAYED, tr("Dismayed"));
	addMood(MOOD_DISTRACTED, UMI_DISTRACTED, tr("Distracted"));
	addMood(MOOD_EMBARRASSED, UMI_EMBARRASSED, tr("Embarrassed"));
	addMood(MOOD_ENVIOUS, UMI_ENVIOUS, tr("Envious"));
	addMood(MOOD_EXCITED, UMI_EXCITED, tr("Excited"));
	addMood(MOOD_FLIRTATIOUS, UMI_FLIRTATIOUS, tr("Flirtatious"));
	addMood(MOOD_FRUSTRATED, UMI_FRUSTRATED, tr("Frustrated"));
	addMood(MOOD_GRUMPY, UMI_GRUMPY, tr("Grumpy"));
	addMood(MOOD_GUILTY, UMI_GUILTY, tr("Guilty"));
	addMood(MOOD_HAPPY, UMI_HAPPY, tr("Happy"));
	addMood(MOOD_HOPEFUL, UMI_HOPEFUL, tr("Hopeful"));
	addMood(MOOD_HOT, UMI_HOT, tr("Hot"));
	addMood(MOOD_HUMBLED, UMI_HUMBLED, tr("Humbled"));
	addMood(MOOD_HUMILIATED, UMI_HUMILIATED, tr("Humiliated"));
	addMood(MOOD_HUNGRY, UMI_HUNGRY, tr("Hungry"));
	addMood(MOOD_HURT, UMI_HURT, tr("Hurt"));
	addMood(MOOD_IMPRESSED, UMI_IMPRESSED, tr("Impressed"));
	addMood(MOOD_IN_AWE, UMI_IN_AWE, tr("In awe"));
	addMood(MOOD_IN_LOVE, UMI_IN_LOVE, tr("In love"));
	addMood(MOOD_INDIGNANT, UMI_INDIGNANT, tr("Indignant"));
	addMood(MOOD_INTERESTED, UMI_INTERESTED, tr("Interested"));
	addMood(MOOD_INTOXICATED, UMI_INTOXICATED, tr("Intoxicated"));
	addMood(MOOD_INVINCIBLE, UMI_INVINCIBLE, tr("Invincible"));
	addMood(MOOD_JEALOUS, UMI_JEALOUS, tr("Jealous"));
	addMood(MOOD_LONELY, UMI_LONELY, tr("Lonely"));
	addMood(MOOD_LUCKY, UMI_LUCKY, tr("Lucky"));
	addMood(MOOD_MEAN, UMI_MEAN, tr("Mean"));
	addMood(MOOD_MOODY, UMI_MOODY, tr("Moody"));
	addMood(MOOD_NERVOUS, UMI_NERVOUS, tr("Nervous"));
	addMood(MOOD_NEUTRAL, UMI_NEUTRAL, tr("Neutral"));
	addMood(MOOD_OFFENDED, UMI_OFFENDED, tr("Offended"));
	addMood(MOOD_OUTRAGED, UMI_OUTRAGED, tr("Outraged"));
	addMood(MOOD_PLAYFUL, UMI_PLAYFUL, tr("Playful"));
	addMood(MOOD_PROUD, UMI_PROUD, tr("Proud"));
	addMood(MOOD_RELAXED, UMI_RELAXED, tr("Relaxed"));
	addMood(MOOD_RELIEVED, UMI_RELIEVED, tr("Relieved"));
	addMood(MOOD_REMORSEFUL, UMI_REMORSEFUL, tr("Remorseful"));
	addMood(MOOD_RESTLESS, UMI_RESTLESS, tr("Restless"));
	addMood(MOOD_SAD, UMI_SAD, tr("Sad"));
	addMood(MOOD_SARCASTIC, UMI_SARCASTIC, tr("Sarcastic"));
	addMood(MOOD_SERIOUS, UMI_SERIOUS, tr("Serious"));
	addMood(MOOD_SHOCKED, UMI_SHOCKED, tr("Shocked"));
	addMood(MOOD_SHY, UMI_SHY, tr("Shy"));
	addMood(MOOD_SICK, UMI_SICK, tr("Sick"));
	addMood(MOOD_SLEEPY, UMI_SLEEPY, tr("Sleepy"));
	addMood(MOOD_SPONTANEOUS, UMI_SPONTANEOUS, tr("Spontaneous"));
	addMood(MOOD_STRESSED, UMI_STRESSED, tr("Stressed"));
	addMood(MOOD_STRONG, UMI_STRONG, tr("Strong"));
	addMood(MOOD_SURPRISED, UMI_SURPRISED, tr("Surprised"));
	addMood(MOOD_THANKFUL, UMI_THANKFUL, tr("Thankful"));
	addMood(MOOD_THIRSTY, UMI_THIRSTY, tr("Thirsty"));
	addMood(MOOD_TIRED, UMI_TIRED, tr("Tired"));
	addMood(MOOD_UNDEFINED, UMI_UNDEFINED, tr("Undefined"));
	addMood(MOOD_WEAK, UMI_WEAK, tr("Weak"));
	addMood(MOOD_WORRIED, UMI_WORRIED, tr("Worried"));

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
	if (FNotifications && FContactsMood.contains(AContactJid.pBare()) /*&& AContactJid.pBare() != AStreamJid.pBare()*/)
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERMOOD);
		if ((notify.kinds & INotification::PopupWindow) > 0)
		{
			notify.typeId = NNT_USERMOOD;
			notify.data.insert(NDR_ICON,contactMoodIcon(AContactJid));
			notify.data.insert(NDR_STREAM_JID,AStreamJid.full());
			notify.data.insert(NDR_CONTACT_JID,AContactJid.full());
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
				if(show != IPresence::Offline && show != IPresence::Error && FPEPManager->isSupported(AStreamJid))
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
