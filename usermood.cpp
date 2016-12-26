#include "usermood.h"

#define ADR_STREAM_JID Action::DR_StreamJid
#define RDR_USERMOOD 452

static const QList<int> RosterKinds = QList<int>() << RIK_CONTACT << RIK_CONTACTS_ROOT << RIK_STREAM_ROOT;

UserMood::UserMood()
{
	FMainWindowPlugin = NULL;
	FPresenceManager = NULL;
	FPEPManager = NULL;
	FDiscovery = NULL;
	FXmppStreamManager = NULL;
	FOptionsManager = NULL;
	FRosterManager = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;
	FNotifications = NULL;

#ifdef DEBUG_RESOURCES_DIR
	FileStorage::setResourcesDirs(FileStorage::resourcesDirs() << DEBUG_RESOURCES_DIR);
#endif
}

UserMood::~UserMood()
{

}

void UserMood::addMood(const QString &keyname, const QString &locname)
{
	MoodData moodData = {locname, IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(keyname)};
	FMoods.insert(keyname, moodData);
}

void UserMood::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("User Mood");
	APluginInfo->description = tr("Allows you to send and receive information about user moods");
	APluginInfo->version = "0.7";
	APluginInfo->author = "Alexey Ivanov aka krab";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins";
	APluginInfo->dependences.append(PEPMANAGER_UUID);
	APluginInfo->dependences.append(SERVICEDISCOVERY_UUID);
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(PRESENCE_UUID);
}

bool UserMood::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	AInitOrder = 50;

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

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0, NULL);
	if(plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if(FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0, NULL);
	if(plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
		if(FPresenceManager)
		{
			connect(FPresenceManager->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
					SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0, NULL);
	if(plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
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
		if (FOptionsManager)
		{
			connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
			connect(Options::instance(),SIGNAL(optionsChanged(OptionsNode)),SLOT(onOptionsChanged(OptionsNode)));
		}
	}

	connect(APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));

	return FMainWindowPlugin != NULL && FRosterManager !=NULL && FPEPManager !=NULL;
}

bool UserMood::initObjects()
{
	handlerId = FPEPManager->insertNodeHandler(MOOD_PROTOCOL_URL, this);

	IDiscoFeature feature;
	feature.active = true;
	feature.name = tr("User Mood");
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
		FRostersModel->insertRosterDataHolder(RDHO_USERMOOD,this);
	}

	if(FRostersViewPlugin)
	{
		AdvancedDelegateItem label(RLID_USERMOOD);
		label.d->kind = AdvancedDelegateItem::CustomData;
		label.d->data = RDR_USERMOOD;
		FMoodLabelId = FRostersViewPlugin->rostersView()->registerLabel(label);

		FRostersViewPlugin->rostersView()->insertLabelHolder(RLHO_USERMOOD,this);
	}

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
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

bool UserMood::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_USER_MOOD_ICON_SHOW,true);

	return true;
}

QMultiMap<int, IOptionsDialogWidget *> UserMood::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_ROSTERVIEW)
	{
		widgets.insertMulti(OWO_ROSTER_USERMOOD, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_USER_MOOD_ICON_SHOW),tr("Show contact mood icon"),AParent));
	}
	return widgets;
}


QList<int> UserMood::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_USERMOOD)
		return QList<int>() << RDR_USERMOOD;
	return QList<int>();
}

QVariant UserMood::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder == RDHO_USERMOOD)
	{
		switch (AIndex->kind())
		{
		case RIK_STREAM_ROOT:
		case RIK_CONTACT:
			{
				if (ARole == RDR_USERMOOD)
				{
					Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
					Jid senderJid = AIndex->data(RDR_PREP_BARE_JID).toString();
					return QIcon(contactMoodIcon(streamJid, senderJid));
				}
			}
			break;
		}
	}
return QVariant();
}

bool UserMood::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

QList<quint32> UserMood::rosterLabels(int AOrder, const IRosterIndex *AIndex) const
{
	QList<quint32> labels;
	if (AOrder==RLHO_USERMOOD && FMoodIconsVisible && !AIndex->data(RDR_USERMOOD).isNull())
		labels.append(FMoodLabelId);
	return labels;
}

AdvancedDelegateItem UserMood::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AIndex);
	return FRostersViewPlugin->rostersView()->registeredLabel(ALabelId);
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
						if(!choiseElem.isNull() && FMoods.contains(choiseElem.nodeName()))
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
	if (FNotifications && FContacts[streamJid].contains(senderJid.pBare()) && streamJid.pBare() != senderJid.pBare())
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
			if(!contactMoodText(streamJid, senderJid).isEmpty())
				notify.data.insert(NDR_POPUP_TEXT,QString("%1:\n%2").arg(contactMoodName(streamJid, senderJid)).arg(contactMoodText(streamJid, senderJid)));
			else
				notify.data.insert(NDR_POPUP_TEXT,QString("%1").arg(contactMoodName(streamJid, senderJid)));
			FNotifies.insert(FNotifications->appendNotification(notify),senderJid);
		}
	}
}

void UserMood::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
		FNotifications->removeNotification(ANotifyId);
}

void UserMood::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
		FNotifies.remove(ANotifyId);
}

void UserMood::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		IRosterIndex *index = AIndexes.first();
		if(index->kind() == RIK_STREAM_ROOT)
		{
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresenceManager != NULL ? FPresenceManager->findPresence(streamJid) : NULL;
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

void UserMood::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips)
{
	if ((ALabelId==AdvancedDelegateItem::DisplayId && RosterKinds.contains(AIndex->kind())) || ALabelId == FMoodLabelId)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		if(!contactMoodKey(streamJid, contactJid).isEmpty())
		{
			QString tooltip_full = QString("<b>%1</b> %2<br>%3</div>")
					.arg(tr("Mood:")).arg(contactMoodName(streamJid, contactJid)).arg(contactMoodText(streamJid, contactJid));
			QString tooltip_short = QString("<b>%1</b> %2</div>")
					.arg(tr("Mood:")).arg(contactMoodName(streamJid, contactJid));
			AToolTips.insert(RTTO_USERMOOD, contactMoodText(streamJid, contactJid).isEmpty() ? tooltip_short : tooltip_full);
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
		dialog = new UserMoodDialog(this, FMoods, streamJid);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void UserMood::setContactMood(const Jid &streamJid, const Jid &senderJid, const Mood &mood)
{
	if((contactMoodKey(streamJid, senderJid) != mood.keyname) ||
			contactMoodText(streamJid, senderJid) != mood.text)
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;
		if((roster!=NULL && !roster->findItem(senderJid).isNull()) || streamJid.pBare() == senderJid.pBare())
		{
			if(!mood.keyname.isEmpty())
			{
				FContacts[streamJid].insert(senderJid.pBare(), mood);
				onShowNotification(streamJid, senderJid);
			}
			else
				FContacts[streamJid].remove(senderJid.pBare());
		}
	}
	updateDataHolder(streamJid, senderJid);
}

void UserMood::updateDataHolder(const Jid &streamJid, const Jid &senderJid)
{
	if(FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		if (!streamJid.isEmpty())
			findData.insert(RDR_STREAM_JID,streamJid.pFull());
		if (!senderJid.isEmpty())
			findData.insert(RDR_PREP_BARE_JID,senderJid.pBare());
		findData.insert(RDR_KIND,RIK_STREAM_ROOT);
		findData.insert(RDR_KIND,RIK_CONTACT);
		findData.insert(RDR_KIND,RIK_CONTACTS_ROOT);

		foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
			emit rosterDataChanged(index, RDR_USERMOOD);
	}
}

void UserMood::onStreamClosed(IXmppStream *AXmppStream)
{
	FContacts.remove(AXmppStream->streamJid());
}

void UserMood::onContactStateChanged(const Jid &streamJid, const Jid &contactJid, bool AStateOnline)
{
	if (!AStateOnline && FContacts[streamJid].contains(contactJid.pBare()))
		{
			FContacts[streamJid].remove(contactJid.pBare());
			updateDataHolder(streamJid, contactJid);
		}
}

void UserMood::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ROSTER_USER_MOOD_ICON_SHOW));
}

void UserMood::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_USER_MOOD_ICON_SHOW)
	{
		FMoodIconsVisible = ANode.value().toBool();
		emit rosterLabelChanged(FMoodLabelId,NULL);
	}
}

QIcon UserMood::moodIcon(const QString &keyname) const
{
	return FMoods.value(keyname).icon;
}

QString UserMood::moodName(const QString &keyname) const
{
	return FMoods.value(keyname).locname;
}

QString UserMood::contactMoodKey(const Jid &streamJid, const Jid &contactJid) const
{
	return FContacts[streamJid].value(contactJid.pBare()).keyname;
}

QIcon UserMood::contactMoodIcon(const Jid &streamJid, const Jid &contactJid) const
{
	return FMoods.value(FContacts[streamJid].value(contactJid.pBare()).keyname).icon;
}

QString UserMood::contactMoodName(const Jid &streamJid, const Jid &contactJid) const
{
	return FMoods.value(FContacts[streamJid].value(contactJid.pBare()).keyname).locname;
}

QString UserMood::contactMoodText(const Jid &streamJid, const Jid &contactJid) const
{
	QString text = FContacts[streamJid].value(contactJid.pBare()).text;
	return text.replace("\n", "<br>");
}

void UserMood::onApplicationQuit()
{
	FPEPManager->removeNodeHandler(handlerId);
}
