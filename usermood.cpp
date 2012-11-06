#include "usermood.h"

#define ADR_STREAM_JID	Action::DR_StreamJid
#define RDR_MOOD_NAME	452

UserMood::UserMood()
{
	FMainWindowPlugin = NULL;
	FPresencePlugin = NULL;
	FPEPManager = NULL;
	FDiscovery = NULL;
	FXmppStreams = NULL;
	FOptionsManager = NULL;
	FRoster = NULL;
	FRosterPlugin = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FNotifications = NULL;
}

void UserMood::addMood(const QString &keyname, const QString &locname)
{
	MoodData moodData = {locname, IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(keyname)};
	FMoodsCatalog.insert(keyname, moodData);
}

UserMood::~UserMood()
{

}

void UserMood::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("User Mood");
	APluginInfo->description = tr("Allows you to send and receive information about user moods");
	APluginInfo->version = "0.5";
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
		if(FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0, NULL);
	if(plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if(FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
					SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IRoster").value(0, NULL);
	if(plugin)
	{
		FRoster = qobject_cast<IRoster *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0, NULL);
	if(plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
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

	connect(APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));

	return FMainWindowPlugin != NULL && FRosterPlugin !=NULL && FPEPManager !=NULL;
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
		IRostersLabel label;
		label.order = RLO_USERMOOD;
		label.value = RDR_MOOD_NAME;
		FUserMoodLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);
	}

	addMood(MOOD_NULL, tr("Without mood"));
	addMood(MOOD_AFRAID, tr("Afraid"));
	addMood(MOOD_AMAZED, tr("Amazed"));
	addMood(MOOD_ANGRY, tr("Angry"));
	addMood(MOOD_AMOROUS, tr("Amorous"));
	addMood(MOOD_ANNOYED, tr("Annoyed"));
	addMood(MOOD_ANXIOUS, tr("Anxious"));
	addMood(MOOD_AROUSED, tr("Aroused"));
	addMood(MOOD_ASHAMED, tr("Ashamed"));
	addMood(MOOD_BORED, tr("Bored"));
	addMood(MOOD_BRAVE, tr("Brave"));
	addMood(MOOD_CALM, tr("Calm"));
	addMood(MOOD_CAUTIOUS, tr("Cautious"));
	addMood(MOOD_COLD, tr("Cold"));
	addMood(MOOD_CONFIDENT, tr("Confident"));
	addMood(MOOD_CONFUSED, tr("Confused"));
	addMood(MOOD_CONTEMPLATIVE, tr("Contemplative"));
	addMood(MOOD_CONTENTED, tr("Contented"));
	addMood(MOOD_CRANKY, tr("Cranky"));
	addMood(MOOD_CRAZY, tr("Crazy"));
	addMood(MOOD_CREATIVE, tr("Creative"));
	addMood(MOOD_CURIOUS, tr("Curious"));
	addMood(MOOD_DEJECTED, tr("Dejected"));
	addMood(MOOD_DEPRESSED, tr("Depressed"));
	addMood(MOOD_DISAPPOINTED, tr("Disappointed"));
	addMood(MOOD_DISGUSTED, tr("Disgusted"));
	addMood(MOOD_DISMAYED, tr("Dismayed"));
	addMood(MOOD_DISTRACTED, tr("Distracted"));
	addMood(MOOD_EMBARRASSED, tr("Embarrassed"));
	addMood(MOOD_ENVIOUS, tr("Envious"));
	addMood(MOOD_EXCITED, tr("Excited"));
	addMood(MOOD_FLIRTATIOUS, tr("Flirtatious"));
	addMood(MOOD_FRUSTRATED, tr("Frustrated"));
	addMood(MOOD_GRUMPY, tr("Grumpy"));
	addMood(MOOD_GUILTY, tr("Guilty"));
	addMood(MOOD_HAPPY, tr("Happy"));
	addMood(MOOD_HOPEFUL, tr("Hopeful"));
	addMood(MOOD_HOT, tr("Hot"));
	addMood(MOOD_HUMBLED, tr("Humbled"));
	addMood(MOOD_HUMILIATED, tr("Humiliated"));
	addMood(MOOD_HUNGRY, tr("Hungry"));
	addMood(MOOD_HURT, tr("Hurt"));
	addMood(MOOD_IMPRESSED, tr("Impressed"));
	addMood(MOOD_IN_AWE, tr("In awe"));
	addMood(MOOD_IN_LOVE, tr("In love"));
	addMood(MOOD_INDIGNANT, tr("Indignant"));
	addMood(MOOD_INTERESTED, tr("Interested"));
	addMood(MOOD_INTOXICATED, tr("Intoxicated"));
	addMood(MOOD_INVINCIBLE, tr("Invincible"));
	addMood(MOOD_JEALOUS, tr("Jealous"));
	addMood(MOOD_LONELY, tr("Lonely"));
	addMood(MOOD_LUCKY, tr("Lucky"));
	addMood(MOOD_MEAN, tr("Mean"));
	addMood(MOOD_MOODY, tr("Moody"));
	addMood(MOOD_NERVOUS, tr("Nervous"));
	addMood(MOOD_NEUTRAL, tr("Neutral"));
	addMood(MOOD_OFFENDED, tr("Offended"));
	addMood(MOOD_OUTRAGED, tr("Outraged"));
	addMood(MOOD_PLAYFUL, tr("Playful"));
	addMood(MOOD_PROUD, tr("Proud"));
	addMood(MOOD_RELAXED, tr("Relaxed"));
	addMood(MOOD_RELIEVED, tr("Relieved"));
	addMood(MOOD_REMORSEFUL, tr("Remorseful"));
	addMood(MOOD_RESTLESS, tr("Restless"));
	addMood(MOOD_SAD, tr("Sad"));
	addMood(MOOD_SARCASTIC, tr("Sarcastic"));
	addMood(MOOD_SERIOUS, tr("Serious"));
	addMood(MOOD_SHOCKED, tr("Shocked"));
	addMood(MOOD_SHY, tr("Shy"));
	addMood(MOOD_SICK, tr("Sick"));
	addMood(MOOD_SLEEPY, tr("Sleepy"));
	addMood(MOOD_SPONTANEOUS, tr("Spontaneous"));
	addMood(MOOD_STRESSED, tr("Stressed"));
	addMood(MOOD_STRONG, tr("Strong"));
	addMood(MOOD_SURPRISED, tr("Surprised"));
	addMood(MOOD_THANKFUL, tr("Thankful"));
	addMood(MOOD_THIRSTY, tr("Thirsty"));
	addMood(MOOD_TIRED, tr("Tired"));
	addMood(MOOD_UNDEFINED, tr("Undefined"));
	addMood(MOOD_WEAK, tr("Weak"));
	addMood(MOOD_WORRIED, tr("Worried"));

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
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid senderJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		QIcon pic = contactMoodIcon(streamJid, senderJid);
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

bool UserMood::processPEPEvent(const Jid &streamJid, const Stanza &AStanza)
{
	QDomElement replyElem = AStanza.document().firstChildElement("message");
	if(!replyElem.isNull())
	{
		Mood data;
		Jid senderJid = replyElem.attribute("from");

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
						if(!choiseElem.isNull() && FMoodsCatalog.contains(choiseElem.nodeName()))
						{
							data.keyname = choiseElem.nodeName();
						}
						QDomElement textElem = moodElem.firstChildElement("text");
						if(!textElem.isNull())
						{
							data.text = textElem.text();
						}
					}
					else
						return false;
				}
			}
		}
		setContactMood(streamJid, senderJid, data);
	}
	else
		return false;

	return true;
}

void UserMood::setMood(const Jid &streamJid, const Mood &mood)
{
	QDomDocument docElem("");
	QDomElement rootElem = docElem.createElement("item");
	docElem.appendChild(rootElem);

	QDomElement moodElem = docElem.createElementNS(MOOD_PROTOCOL_URL, "mood");
	rootElem.appendChild(moodElem);
	if(mood.keyname != MOOD_NULL)
	{
		QDomElement nameElem = docElem.createElement(mood.keyname);
		moodElem.appendChild(nameElem);
		QDomElement textElem = docElem.createElement("text");
		moodElem.appendChild(textElem);
		QDomText deskElem = docElem.createTextNode(mood.text);
		textElem.appendChild(deskElem);
	}
	else
	{
		QDomElement nameElem = docElem.createElement("");
		moodElem.appendChild(nameElem);
	}
	FPEPManager->publishItem(streamJid, MOOD_PROTOCOL_URL, rootElem);
}

void UserMood::onShowNotification(const Jid &streamJid, const Jid &senderJid)
{
	if (FNotifications && FMoodsContacts[streamJid].contains(senderJid.pBare()) && streamJid.pBare() != senderJid.pBare())
	{
		INotification notify;
		notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_USERMOOD);
		if ((notify.kinds & INotification::PopupWindow) > 0)
		{
			notify.typeId = NNT_USERMOOD;
			notify.data.insert(NDR_ICON,contactMoodIcon(streamJid, senderJid));
			notify.data.insert(NDR_STREAM_JID,streamJid.full());
			notify.data.insert(NDR_CONTACT_JID,senderJid.full());
			notify.data.insert(NDR_TOOLTIP,QString("%1 %2").arg(FNotifications->contactName(streamJid, senderJid)).arg(tr("changed mood")));
			notify.data.insert(NDR_POPUP_CAPTION,tr("Mood changed"));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(streamJid, senderJid));
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(senderJid));
			notify.data.insert(NDR_POPUP_HTML,QString("%1 %2").arg(contactMoodName(streamJid, senderJid)).arg(contactMoodText(streamJid, senderJid)));
			FNotifies.insert(FNotifications->appendNotification(notify),senderJid);
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
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresencePlugin != NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
			if(presence && presence->isOpen())
			{
				int show = index->data(RDR_SHOW).toInt();
				if(show != IPresence::Offline && show != IPresence::Error && FPEPManager->isSupported(streamJid))
				{
					Action *action = createSetMoodAction(streamJid, MOOD_PROTOCOL_URL, AMenu);
					AMenu->addAction(action, AG_RVCM_USERMOOD, false);
				}
			}
		}
	}
}

Action *UserMood::createSetMoodAction(const Jid &streamJid, const QString &AFeature, QObject *AParent) const
{
	if(AFeature == MOOD_PROTOCOL_URL)
	{
		Action *action = new Action(AParent);
		action->setText(tr("Mood"));
		QIcon menuicon;
		if(!contactMoodIcon(streamJid, streamJid).isNull())
			menuicon = contactMoodIcon(streamJid, streamJid);
		else
			menuicon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_USERMOOD);
		action->setIcon(menuicon);
		action->setData(ADR_STREAM_JID, streamJid.full());
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
		UserMoodDialog *dialog;
		dialog = new UserMoodDialog(this, FMoodsCatalog, streamJid);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void UserMood::setContactMood(const Jid &streamJid, const Jid &senderJid, const Mood &mood)
{
	if((contactMoodKey(streamJid, senderJid) != mood.keyname) ||
			contactMoodText(streamJid, senderJid) != mood.text)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
		foreach(IRosterItem ritem, ritems)
		{
			if ((ritem.isValid && ritem.itemJid.pBare().contains(senderJid.pBare())) ||
					streamJid.pBare() == senderJid.pBare())
			{
				if(!mood.keyname.isEmpty())
				{
					FMoodsContacts[streamJid].insert(senderJid.pBare(), mood);
					onShowNotification(streamJid, senderJid);
				}
				else
				FMoodsContacts[streamJid].remove(senderJid.pBare());
			}
		}
	}
	updateDataHolder(streamJid, senderJid);
}

void UserMood::updateDataHolder(const Jid &streamJid, const Jid &senderJid)
{
	if(FRostersViewPlugin && FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		foreach(int type, rosterDataTypes())
			findData.insert(RDR_TYPE, type);
		findData.insert(RDR_PREP_BARE_JID, senderJid.pBare());

		foreach (IRosterIndex *index, FRostersModel->streamRoot(streamJid)->findChilds(findData, true))
		{
			if(FMoodsContacts[streamJid].contains(senderJid.pBare()))
				FRostersViewPlugin->rostersView()->insertLabel(FUserMoodLabelId,index);
			else
				FRostersViewPlugin->rostersView()->removeLabel(FUserMoodLabelId,index);
			emit rosterDataChanged(index, RDR_MOOD_NAME);
		}
	}
}

void UserMood::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FRostersViewPlugin)
	{
		IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
		IRosterIndex *index = model!=NULL ? model->streamRoot(AXmppStream->streamJid()) : NULL;
		if (index!=NULL)
			FRostersViewPlugin->rostersView()->insertLabel(FUserMoodLabelId,index);
	}
}

void UserMood::onStreamClosed(IXmppStream *AXmppStream)
{
	Jid streamJid = AXmppStream->streamJid();
	FMoodsContacts.remove(streamJid);
	if(FRostersViewPlugin && FRostersModel)
	{
		IRosterIndex *index = FRostersModel->streamRoot(streamJid);
		FRostersViewPlugin->rostersView()->removeLabel(FUserMoodLabelId,index);
		emit rosterDataChanged(index, RDR_MOOD_NAME);
	}
}

void UserMood::onContactStateChanged(const Jid &streamJid, const Jid &contactJid, bool AStateOnline)
{
	if (!AStateOnline)
	{
		if (FMoodsContacts[streamJid].contains(contactJid.pBare()))
		{
			FMoodsContacts[streamJid].remove(contactJid.pBare());
			updateDataHolder(streamJid, contactJid);
		}
	}
}

void UserMood::onRosterIndexToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int, QString> &AToolTips)
{
	if(ALabelId == RLID_DISPLAY || ALabelId == FUserMoodLabelId)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if(!contactMoodKey(streamJid, contactJid).isEmpty())
		{
			QString tooltip_full = QString("%1 <div style='margin-left:10px;'>%2<br>%3</div>")
					.arg(tr("Mood:")).arg(contactMoodName(streamJid, contactJid)).arg(contactMoodText(streamJid, contactJid));
			QString tooltip_short = QString("%1 <div style='margin-left:10px;'>%2</div>")
					.arg(tr("Mood:")).arg(contactMoodName(streamJid, contactJid));
			AToolTips.insert(RTTO_USERMOOD, contactMoodText(streamJid, contactJid).isEmpty() ? tooltip_short : tooltip_full);
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

QString UserMood::contactMoodKey(const Jid &streamJid, const Jid &contactJid) const
{
	return FMoodsContacts[streamJid].value(contactJid.pBare()).keyname;
}

QIcon UserMood::contactMoodIcon(const Jid &streamJid, const Jid &contactJid) const
{
	return FMoodsCatalog.value(FMoodsContacts[streamJid].value(contactJid.pBare()).keyname).icon;
}

QString UserMood::contactMoodName(const Jid &streamJid, const Jid &contactJid) const
{
	return FMoodsCatalog.value(FMoodsContacts[streamJid].value(contactJid.pBare()).keyname).locname;
}

QString UserMood::contactMoodText(const Jid &streamJid, const Jid &contactJid) const
{
	QString text = FMoodsContacts[streamJid].value(contactJid.pBare()).text;
	return text.replace("\n", "<br>");
}

void UserMood::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(handlerId);
}

Q_EXPORT_PLUGIN2(plg_pepmanager, UserMood)
