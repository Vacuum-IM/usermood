#include "usermood.h"

#include <QDir>
#include <QMessageBox>
#include <QApplication>
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
	APluginInfo->author = "Alexey Ivanov";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins";
	APluginInfo->dependences.append(MAINWINDOW_UUID);
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
		connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)),
										SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
		connect(FRostersViewPlugin->rostersView()->instance(), SIGNAL(indexToolTips(IRosterIndex *, int , QMultiMap<int, QString> &)),
										SLOT(onRosterIndexToolTips(IRosterIndex *, int , QMultiMap<int, QString> &)));
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
	data.name = tr("Without mood");
	FMoodsCatalog.insert(MOOD_NULL, data);
	data.name = tr("Afraid");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AFRAID);
	FMoodsCatalog.insert(MOOD_AFRAID, data);
	data.name = tr("Amazed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AMAZED);
	FMoodsCatalog.insert(MOOD_AMAZED, data);
	data.name = tr("Angry");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ANGRY);
	FMoodsCatalog.insert(MOOD_ANGRY, data);
	data.name = tr("Amorous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AMOROUS);
	FMoodsCatalog.insert(MOOD_AMOROUS, data);
	data.name = tr("Annoyed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ANNOYED);
	FMoodsCatalog.insert(MOOD_ANNOYED, data);
	data.name = tr("Anxious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ANXIOUS);
	FMoodsCatalog.insert(MOOD_ANXIOUS, data);
	data.name = tr("Aroused");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_AROUSED);
	FMoodsCatalog.insert(MOOD_AROUSED, data);
	data.name = tr("Ashamed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ASHAMED);
	FMoodsCatalog.insert(MOOD_ASHAMED, data);
	data.name = tr("Bored");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_BORED);
	FMoodsCatalog.insert(MOOD_BORED, data);
	data.name = tr("Brave");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_BRAVE);
	FMoodsCatalog.insert(MOOD_BRAVE, data);
	data.name = tr("Calm");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CALM);
	FMoodsCatalog.insert(MOOD_CALM, data);
	data.name = tr("Cautious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CAUTIOUS);
	FMoodsCatalog.insert(MOOD_CAUTIOUS, data);
	data.name = tr("Cold");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_COLD);
	FMoodsCatalog.insert(MOOD_COLD, data);
	data.name = tr("Confident");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONFIDENT);
	FMoodsCatalog.insert(MOOD_CONFIDENT, data);
	data.name = tr("Confused");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONFUSED);
	FMoodsCatalog.insert(MOOD_CONFUSED, data);
	data.name = tr("Contemplative");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONTEMPLATIVE);
	FMoodsCatalog.insert(MOOD_CONTEMPLATIVE, data);
	data.name = tr("Contented");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CONTENTED);
	FMoodsCatalog.insert(MOOD_CONTENTED, data);
	data.name = tr("Cranky");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CRANKY);
	FMoodsCatalog.insert(MOOD_CRANKY, data);
	data.name = tr("Crazy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CRAZY);
	FMoodsCatalog.insert(MOOD_CRAZY, data);
	data.name = tr("Creative");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CREATIVE);
	FMoodsCatalog.insert(MOOD_CREATIVE, data);
	data.name = tr("Curious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_CURIOUS);
	FMoodsCatalog.insert(MOOD_CURIOUS, data);
	data.name = tr("Dejected");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DEJECTED);
	FMoodsCatalog.insert(MOOD_DEJECTED, data);
	data.name = tr("Depressed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DEPRESSED);
	FMoodsCatalog.insert(MOOD_DEPRESSED, data);
	data.name = tr("Disappointed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISAPPOINTED);
	FMoodsCatalog.insert(MOOD_DISAPPOINTED, data);
	data.name = tr("Disgusted");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISGUSTED);
	FMoodsCatalog.insert(MOOD_DISGUSTED, data);
	data.name = tr("Dismayed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISMAYED);
	FMoodsCatalog.insert(MOOD_DISMAYED, data);
	data.name = tr("Distracted");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_DISTRACTED);
	FMoodsCatalog.insert(MOOD_DISTRACTED, data);
	data.name = tr("Embarrassed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_EMBARRASSED);
	FMoodsCatalog.insert(MOOD_EMBARRASSED, data);
	data.name = tr("Envious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_ENVIOUS);
	FMoodsCatalog.insert(MOOD_ENVIOUS, data);
	data.name = tr("Excited");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_EXCITED);
	FMoodsCatalog.insert(MOOD_EXCITED, data);
	data.name = tr("Flirtatious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_FLIRTATIOUS);
	FMoodsCatalog.insert(MOOD_FLIRTATIOUS, data);
	data.name = tr("Frustrated");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_FRUSTRATED);
	FMoodsCatalog.insert(MOOD_FRUSTRATED, data);
	data.name = tr("Grumpy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_GRUMPY);
	FMoodsCatalog.insert(MOOD_GRUMPY, data);
	data.name = tr("Guilty");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_GUILTY);
	FMoodsCatalog.insert(MOOD_GUILTY, data);
	data.name = tr("Happy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HAPPY);
	FMoodsCatalog.insert(MOOD_HAPPY, data);
	data.name = tr("Hopeful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HOPEFUL);
	FMoodsCatalog.insert(MOOD_HOPEFUL, data);
	data.name = tr("Hot");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HOT);
	FMoodsCatalog.insert(MOOD_HOT, data);
	data.name = tr("Humbled");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HUMBLED);
	FMoodsCatalog.insert(MOOD_HUMBLED, data);
	data.name = tr("Humiliated");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HUMILIATED);
	FMoodsCatalog.insert(MOOD_HUMILIATED, data);
	data.name = tr("Hungry");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HUNGRY);
	FMoodsCatalog.insert(MOOD_HUNGRY, data);
	data.name = tr("Hurt");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_HURT);
	FMoodsCatalog.insert(MOOD_HURT, data);
	data.name = tr("Impressed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_IMPRESSED);
	FMoodsCatalog.insert(MOOD_IMPRESSED, data);
	data.name = tr("In awe");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_IN_AWE);
	FMoodsCatalog.insert(MOOD_IN_AWE, data);
	data.name = tr("In love");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_IN_LOVE);
	FMoodsCatalog.insert(MOOD_IN_LOVE, data);
	data.name = tr("Indignant");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INDIGNANT);
	FMoodsCatalog.insert(MOOD_INDIGNANT, data);
	data.name = tr("Interested");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INTERESTED);
	FMoodsCatalog.insert(MOOD_INTERESTED, data);
	data.name = tr("Intoxicated");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INTOXICATED);
	FMoodsCatalog.insert(MOOD_INTOXICATED, data);
	data.name = tr("Invincible");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_INVINCIBLE);
	FMoodsCatalog.insert(MOOD_INVINCIBLE, data);
	data.name = tr("Jealous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_JEALOUS);
	FMoodsCatalog.insert(MOOD_JEALOUS, data);
	data.name = tr("Lonely");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_LONELY);
	FMoodsCatalog.insert(MOOD_LONELY, data);
	data.name = tr("Lucky");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_LUCKY);
	FMoodsCatalog.insert(MOOD_LUCKY, data);
	data.name = tr("Mean");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_MEAN);
	FMoodsCatalog.insert(MOOD_MEAN, data);
	data.name = tr("Moody");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_MOODY);
	FMoodsCatalog.insert(MOOD_MOODY, data);
	data.name = tr("Nervous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_NERVOUS);
	FMoodsCatalog.insert(MOOD_NERVOUS, data);
	data.name = tr("Neutral");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_NEUTRAL);
	FMoodsCatalog.insert(MOOD_NEUTRAL, data);
	data.name = tr("Offended");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_OFFENDED);
	FMoodsCatalog.insert(MOOD_OFFENDED, data);
	data.name = tr("Outraged");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_OUTRAGED);
	FMoodsCatalog.insert(MOOD_OUTRAGED, data);
	data.name = tr("Playful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_PLAYFUL);
	FMoodsCatalog.insert(MOOD_PLAYFUL, data);
	data.name = tr("Proud");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_PROUD);
	FMoodsCatalog.insert(MOOD_PROUD, data);
	data.name = tr("Relaxed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_RELAXED);
	FMoodsCatalog.insert(MOOD_RELAXED, data);
	data.name = tr("Relieved");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_RELIEVED);
	FMoodsCatalog.insert(MOOD_RELIEVED, data);
	data.name = tr("Remorseful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_REMORSEFUL);
	FMoodsCatalog.insert(MOOD_REMORSEFUL, data);
	data.name = tr("Restless");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_RESTLESS);
	FMoodsCatalog.insert(MOOD_RESTLESS, data);
	data.name = tr("Sad");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SAD);
	FMoodsCatalog.insert(MOOD_SAD, data);
	data.name = tr("Sarcastic");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SARCASTIC);
	FMoodsCatalog.insert(MOOD_SARCASTIC, data);
	data.name = tr("Serious");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SERIOUS);
	FMoodsCatalog.insert(MOOD_SERIOUS, data);
	data.name = tr("Shocked");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SHOCKED);
	FMoodsCatalog.insert(MOOD_SHOCKED, data);
	data.name = tr("Shy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SHY);
	FMoodsCatalog.insert(MOOD_SHY, data);
	data.name = tr("Sick");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SICK);
	FMoodsCatalog.insert(MOOD_SICK, data);
	data.name = tr("Sleepy");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SLEEPY);
	FMoodsCatalog.insert(MOOD_SLEEPY, data);
	data.name = tr("Spontaneous");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SPONTANEOUS);
	FMoodsCatalog.insert(MOOD_SPONTANEOUS, data);
	data.name = tr("Stressed");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_STRESSED);
	FMoodsCatalog.insert(MOOD_STRESSED, data);
	data.name = tr("Strong");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_STRONG);
	FMoodsCatalog.insert(MOOD_STRONG, data);
	data.name = tr("Surprised");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_SURPRISED);
	FMoodsCatalog.insert(MOOD_SURPRISED, data);
	data.name = tr("Thankful");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_THANKFUL);
	FMoodsCatalog.insert(MOOD_THANKFUL, data);
	data.name = tr("Thirsty");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_THIRSTY);
	FMoodsCatalog.insert(MOOD_THIRSTY, data);
	data.name = tr("Tired");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_TIRED);
	FMoodsCatalog.insert(MOOD_TIRED, data);
	data.name = tr("Undefined");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_UNDEFINED);
	FMoodsCatalog.insert(MOOD_UNDEFINED, data);
	data.name = tr("Weak");
	data.icon = IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(UMI_WEAK);
	FMoodsCatalog.insert(MOOD_WEAK, data);
	data.name = tr("Worried");
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
		QIcon pic = FMoodsCatalog.value(FContactMood.value(AIndex->data(RDR_PREP_BARE_JID).toString()).first).icon;
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
	Jid senderJid;
	QString moodName;
	QString moodText;

	QDomElement replyElem = AStanza.document().firstChildElement("message");

	if(!replyElem.isNull())
	{
		senderJid = replyElem.attribute("from");
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
							moodName = moodElem.firstChildElement().nodeName();
						}

						QDomElement textElem = moodElem.firstChildElement("text");

						if(!moodElem.isNull())
						{
							moodText = textElem.text();
						}
					}
				}
			}
		}
	}
	setContactMood(senderJid, moodName, moodText);

	return true;
}

void UserMood::isSetMood(const Jid &streamJid, const QString &moodKey, const QString &moodText)
{
	QDomDocument doc("");
	QDomElement root = doc.createElement("item");
	doc.appendChild(root);

	QDomElement mood = doc.createElementNS(MOOD_PROTOCOL_URL, "mood");
	root.appendChild(mood);

	if(moodKey != MOOD_NULL)
	{
		QDomElement name = doc.createElement(moodKey);
		mood.appendChild(name);
	}
	else
	{
		QDomElement name = doc.createElement("");
		mood.appendChild(name);
	}

	if(moodKey != MOOD_NULL)
	{
		QDomElement text = doc.createElement("text");
		mood.appendChild(text);

		QDomText t1 = doc.createTextNode(moodText);
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
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresencePlugin != NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
			if(presence && presence->isOpen())
			{
				Jid contactJid = index->data(RDR_FULL_JID).toString();
				int show = index->data(RDR_SHOW).toInt();
				QStringList features = FDiscovery != NULL ? FDiscovery->discoInfo(streamJid, contactJid).features : QStringList();
				if(show != IPresence::Offline && show != IPresence::Error && !features.contains(MOOD_PROTOCOL_URL))
				{
					Action *action = createSetMoodAction(streamJid, MOOD_PROTOCOL_URL, AMenu);
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
		if(!FMoodsCatalog.value(FContactMood.value(AStreamJid.pBare()).first).icon.isNull())
			menuicon = FMoodsCatalog.value(FContactMood.value(AStreamJid.pBare()).first).icon;
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
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		userMoodDialog *dialog;
		dialog = new userMoodDialog(FMoodsCatalog, FContactMood, streamJid, this);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void UserMood::setContactMood(const Jid &senderJid, const QString &AMoodName, const QString &AMoodText)
{
	if((FContactMood.value(senderJid.pBare()).first != AMoodName) || FContactMood.value(senderJid.pBare()).second != AMoodText)
	{
		if(!AMoodName.isEmpty())
			FContactMood.insert(senderJid.pBare(), QPair<QString, QString>(AMoodName, AMoodText));
		else
			FContactMood.remove(senderJid.pBare());
	}
	updateDataHolder(senderJid);
}

void UserMood::updateDataHolder(const Jid &senderJid)
{
	if(FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, rosterDataTypes())
		findData.insert(RDR_TYPE, type);
		if(!senderJid.isEmpty())
			findData.insert(RDR_PREP_BARE_JID, senderJid.pBare());
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
		QString contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if(!FContactMood.value(contactJid).first.isEmpty())
		{
			QString text = FContactMood.value(contactJid).second;
			QString tip = QString("%1 <div style='margin-left:10px;'>%2<br>%3</div>").arg(tr("Mood:")).arg(FMoodsCatalog.value(FContactMood.value(contactJid).first).name).arg(text.replace("\n", "<br>"));
			AToolTips.insert(RTTO_USERMOOD, tip);
		}
	}
}

void UserMood::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserMood)



