#include "usermood.h"

#include <QDebug>

#define ADR_STREAM_JID                  Action::DR_StreamJid
#define RDR_MOOD_NAME                   452

UserMood::UserMood()
{
	FMainWindowPlugin = NULL;
	FPEPManager = NULL;
	FServiceDiscovery = NULL;
	FXmppStreams = NULL;
	FOptionsManager = NULL;
	FDiscovery = NULL;
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
	APluginInfo->version = "0.1";
	APluginInfo->author = "Alexey Ivanov aka krab";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins";
	APluginInfo->dependences.append(MAINWINDOW_UUID);
	APluginInfo->dependences.append(PEPMANAGER_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(PRESENCE_UUID);
	APluginInfo->dependences.append(ROSTERSMODEL_UUID);
	APluginInfo->dependences.append(ROSTERSVIEW_UUID);
	APluginInfo->dependences.append(NOTIFICATIONS_UUID);
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
		FServiceDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
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

	FServiceDiscovery->insertDiscoFeature(feature);

	feature.name = tr("User mood notification");
	feature.var = MOOD_NOTIFY_PROTOCOL_URL;
	feature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERMOOD);
	feature.description = tr("Supports the exchange of information about user moods");
	FServiceDiscovery->insertDiscoFeature(feature);

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

	MoodData data;
	data.locname = tr("Without mood");
	FMoodsCatalog.insert(MOOD_NULL, data);
	data.locname = tr("Afraid");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AFRAID);
	FMoodsCatalog.insert(MOOD_AFRAID, data);
	data.locname = tr("Amazed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AMAZED);
	FMoodsCatalog.insert(MOOD_AMAZED, data);
	data.locname = tr("Angry");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ANGRY);
	FMoodsCatalog.insert(MOOD_ANGRY, data);
	data.locname = tr("Amorous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AMOROUS);
	FMoodsCatalog.insert(MOOD_AMOROUS, data);
	data.locname = tr("Annoyed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ANNOYED);
	FMoodsCatalog.insert(MOOD_ANNOYED, data);
	data.locname = tr("Anxious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ANXIOUS);
	FMoodsCatalog.insert(MOOD_ANXIOUS, data);
	data.locname = tr("Aroused");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AROUSED);
	FMoodsCatalog.insert(MOOD_AROUSED, data);
	data.locname = tr("Ashamed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ASHAMED);
	FMoodsCatalog.insert(MOOD_ASHAMED, data);
	data.locname = tr("Bored");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_BORED);
	FMoodsCatalog.insert(MOOD_BORED, data);
	data.locname = tr("Brave");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_BRAVE);
	FMoodsCatalog.insert(MOOD_BRAVE, data);
	data.locname = tr("Calm");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CALM);
	FMoodsCatalog.insert(MOOD_CALM, data);
	data.locname = tr("Cautious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CAUTIOUS);
	FMoodsCatalog.insert(MOOD_CAUTIOUS, data);
	data.locname = tr("Cold");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_COLD);
	FMoodsCatalog.insert(MOOD_COLD, data);
	data.locname = tr("Confident");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONFIDENT);
	FMoodsCatalog.insert(MOOD_CONFIDENT, data);
	data.locname = tr("Confused");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONFUSED);
	FMoodsCatalog.insert(MOOD_CONFUSED, data);
	data.locname = tr("Contemplative");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONTEMPLATIVE);
	FMoodsCatalog.insert(MOOD_CONTEMPLATIVE, data);
	data.locname = tr("Contented");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONTENTED);
	FMoodsCatalog.insert(MOOD_CONTENTED, data);
	data.locname = tr("Cranky");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CRANKY);
	FMoodsCatalog.insert(MOOD_CRANKY, data);
	data.locname = tr("Crazy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CRAZY);
	FMoodsCatalog.insert(MOOD_CRAZY, data);
	data.locname = tr("Creative");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CREATIVE);
	FMoodsCatalog.insert(MOOD_CREATIVE, data);
	data.locname = tr("Curious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CURIOUS);
	FMoodsCatalog.insert(MOOD_CURIOUS, data);
	data.locname = tr("Dejected");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DEJECTED);
	FMoodsCatalog.insert(MOOD_DEJECTED, data);
	data.locname = tr("Depressed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DEPRESSED);
	FMoodsCatalog.insert(MOOD_DEPRESSED, data);
	data.locname = tr("Disappointed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISAPPOINTED);
	FMoodsCatalog.insert(MOOD_DISAPPOINTED, data);
	data.locname = tr("Disgusted");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISGUSTED);
	FMoodsCatalog.insert(MOOD_DISGUSTED, data);
	data.locname = tr("Dismayed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISMAYED);
	FMoodsCatalog.insert(MOOD_DISMAYED, data);
	data.locname = tr("Distracted");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISTRACTED);
	FMoodsCatalog.insert(MOOD_DISTRACTED, data);
	data.locname = tr("Embarrassed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_EMBARRASSED);
	FMoodsCatalog.insert(MOOD_EMBARRASSED, data);
	data.locname = tr("Envious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ENVIOUS);
	FMoodsCatalog.insert(MOOD_ENVIOUS, data);
	data.locname = tr("Excited");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_EXCITED);
	FMoodsCatalog.insert(MOOD_EXCITED, data);
	data.locname = tr("Flirtatious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_FLIRTATIOUS);
	FMoodsCatalog.insert(MOOD_FLIRTATIOUS, data);
	data.locname = tr("Frustrated");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_FRUSTRATED);
	FMoodsCatalog.insert(MOOD_FRUSTRATED, data);
	data.locname = tr("Grumpy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_GRUMPY);
	FMoodsCatalog.insert(MOOD_GRUMPY, data);
	data.locname = tr("Guilty");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_GUILTY);
	FMoodsCatalog.insert(MOOD_GUILTY, data);
	data.locname = tr("Happy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HAPPY);
	FMoodsCatalog.insert(MOOD_HAPPY, data);
	data.locname = tr("Hopeful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HOPEFUL);
	FMoodsCatalog.insert(MOOD_HOPEFUL, data);
	data.locname = tr("Hot");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HOT);
	FMoodsCatalog.insert(MOOD_HOT, data);
	data.locname = tr("Humbled");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HUMBLED);
	FMoodsCatalog.insert(MOOD_HUMBLED, data);
	data.locname = tr("Humiliated");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HUMILIATED);
	FMoodsCatalog.insert(MOOD_HUMILIATED, data);
	data.locname = tr("Hungry");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HUNGRY);
	FMoodsCatalog.insert(MOOD_HUNGRY, data);
	data.locname = tr("Hurt");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HURT);
	FMoodsCatalog.insert(MOOD_HURT, data);
	data.locname = tr("Impressed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_IMPRESSED);
	FMoodsCatalog.insert(MOOD_IMPRESSED, data);
	data.locname = tr("In awe");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_IN_AWE);
	FMoodsCatalog.insert(MOOD_IN_AWE, data);
	data.locname = tr("In love");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_IN_LOVE);
	FMoodsCatalog.insert(MOOD_IN_LOVE, data);
	data.locname = tr("Indignant");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INDIGNANT);
	FMoodsCatalog.insert(MOOD_INDIGNANT, data);
	data.locname = tr("Interested");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INTERESTED);
	FMoodsCatalog.insert(MOOD_INTERESTED, data);
	data.locname = tr("Intoxicated");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INTOXICATED);
	FMoodsCatalog.insert(MOOD_INTOXICATED, data);
	data.locname = tr("Invincible");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INVINCIBLE);
	FMoodsCatalog.insert(MOOD_INVINCIBLE, data);
	data.locname = tr("Jealous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_JEALOUS);
	FMoodsCatalog.insert(MOOD_JEALOUS, data);
	data.locname = tr("Lonely");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_LONELY);
	FMoodsCatalog.insert(MOOD_LONELY, data);
	data.locname = tr("Lucky");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_LUCKY);
	FMoodsCatalog.insert(MOOD_LUCKY, data);
	data.locname = tr("Mean");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_MEAN);
	FMoodsCatalog.insert(MOOD_MEAN, data);
	data.locname = tr("Moody");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_MOODY);
	FMoodsCatalog.insert(MOOD_MOODY, data);
	data.locname = tr("Nervous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_NERVOUS);
	FMoodsCatalog.insert(MOOD_NERVOUS, data);
	data.locname = tr("Neutral");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_NEUTRAL);
	FMoodsCatalog.insert(MOOD_NEUTRAL, data);
	data.locname = tr("Offended");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_OFFENDED);
	FMoodsCatalog.insert(MOOD_OFFENDED, data);
	data.locname = tr("Outraged");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_OUTRAGED);
	FMoodsCatalog.insert(MOOD_OUTRAGED, data);
	data.locname = tr("Playful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_PLAYFUL);
	FMoodsCatalog.insert(MOOD_PLAYFUL, data);
	data.locname = tr("Proud");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_PROUD);
	FMoodsCatalog.insert(MOOD_PROUD, data);
	data.locname = tr("Relaxed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_RELAXED);
	FMoodsCatalog.insert(MOOD_RELAXED, data);
	data.locname = tr("Relieved");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_RELIEVED);
	FMoodsCatalog.insert(MOOD_RELIEVED, data);
	data.locname = tr("Remorseful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_REMORSEFUL);
	FMoodsCatalog.insert(MOOD_REMORSEFUL, data);
	data.locname = tr("Restless");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_RESTLESS);
	FMoodsCatalog.insert(MOOD_RESTLESS, data);
	data.locname = tr("Sad");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SAD);
	FMoodsCatalog.insert(MOOD_SAD, data);
	data.locname = tr("Sarcastic");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SARCASTIC);
	FMoodsCatalog.insert(MOOD_SARCASTIC, data);
	data.locname = tr("Serious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SERIOUS);
	FMoodsCatalog.insert(MOOD_SERIOUS, data);
	data.locname = tr("Shocked");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SHOCKED);
	FMoodsCatalog.insert(MOOD_SHOCKED, data);
	data.locname = tr("Shy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SHY);
	FMoodsCatalog.insert(MOOD_SHY, data);
	data.locname = tr("Sick");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SICK);
	FMoodsCatalog.insert(MOOD_SICK, data);
	data.locname = tr("Sleepy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SLEEPY);
	FMoodsCatalog.insert(MOOD_SLEEPY, data);
	data.locname = tr("Spontaneous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SPONTANEOUS);
	FMoodsCatalog.insert(MOOD_SPONTANEOUS, data);
	data.locname = tr("Stressed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_STRESSED);
	FMoodsCatalog.insert(MOOD_STRESSED, data);
	data.locname = tr("Strong");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_STRONG);
	FMoodsCatalog.insert(MOOD_STRONG, data);
	data.locname = tr("Surprised");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SURPRISED);
	FMoodsCatalog.insert(MOOD_SURPRISED, data);
	data.locname = tr("Thankful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_THANKFUL);
	FMoodsCatalog.insert(MOOD_THANKFUL, data);
	data.locname = tr("Thirsty");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_THIRSTY);
	FMoodsCatalog.insert(MOOD_THIRSTY, data);
	data.locname = tr("Tired");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_TIRED);
	FMoodsCatalog.insert(MOOD_TIRED, data);
	data.locname = tr("Undefined");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_UNDEFINED);
	FMoodsCatalog.insert(MOOD_UNDEFINED, data);
	data.locname = tr("Weak");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_WEAK);
	FMoodsCatalog.insert(MOOD_WEAK, data);
	data.locname = tr("Worried");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_WORRIED);
	FMoodsCatalog.insert(MOOD_WORRIED, data);

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
		QIcon pic = FMoodsCatalog.value(FContactsMood.value(AIndex->data(RDR_PREP_BARE_JID).toString()).keyname).icon;
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
	setContactMood(ASenderJid, AMoodName, AMoodText);

	return true;
}

void UserMood::isSetMood(const Jid &streamJid, const QString &AMoodKey, const QString &AMoodText)
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

	FPEPManager->publishItem(streamJid, MOOD_PROTOCOL_URL, root);
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
				Jid AContactJid = index->data(RDR_FULL_JID).toString();
				int show = index->data(RDR_SHOW).toInt();
				QStringList features = FDiscovery != NULL ? FDiscovery->discoInfo(AStreamJid, AContactJid).features : QStringList();
				if(show != IPresence::Offline && show != IPresence::Error && !features.contains(MOOD_PROTOCOL_URL))
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
		if(!FMoodsCatalog.value(FContactsMood.value(AStreamJid.pBare()).keyname).icon.isNull())
			menuicon = FMoodsCatalog.value(FContactsMood.value(AStreamJid.pBare()).keyname).icon;
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
		userMoodDialog *dialog;
		dialog = new userMoodDialog(FMoodsCatalog, FContactsMood, AStreamJid, this);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void UserMood::setContactMood(const Jid &ASenderJid, const QString &AMoodName, const QString &AMoodText)
{
	if((FContactsMood.value(ASenderJid.pBare()).keyname != AMoodName) || FContactsMood.value(ASenderJid.pBare()).text != AMoodText)
	{
		if(!AMoodName.isEmpty())
		{
			MoodContact data;
			data.keyname = AMoodName;
			data.text = AMoodText;
			FContactsMood.insert(ASenderJid.pBare(), data);
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
		if(!FContactsMood.value(AContactJid.pBare()).keyname.isEmpty())
		{
			QString text = FContactsMood.value(AContactJid.pBare()).text;
			QString tip = QString("%1 <div style='margin-left:10px;'>%2<br>%3</div>").arg(tr("Mood:")).arg(FMoodsCatalog.value(FContactsMood.value(AContactJid.pBare()).keyname).locname).arg(text.replace("\n", "<br>"));
			AToolTips.insert(RTTO_USERMOOD, tip);
		}
	}
}

void UserMood::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserMood)



