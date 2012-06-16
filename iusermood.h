#ifndef IUSERMOOD_H
#define IUSERMOOD_H

#include <QString>
#include <QIcon>

#include "definitions.h"
#include <utils/iconstorage.h>
#include <utils/jid.h>

#define USERMOOD_UUID "{df730f89-9cb1-472a-b61b-aea95594fde1}"
#define PEP_USERMOOD 4010

class MoodData
{
public:
	MoodData() {}
	MoodData(const QString &locname) : locname(locname) {}
	MoodData(const QString &icon, const QString &locname) : locname(locname), icon(IconStorage::staticStorage(RSR_STORAGE_MOODICONS)->getIcon(icon)) {}
	QString locname;
	QIcon icon;
};

class MoodContact
{
public:
	QString keyname;
	QString text;
};

class IUserMood
{
public:
	virtual QObject *instance() = 0;
	virtual void setMood(const Jid &AStreamJid, const QString &AMoodKey, const QString &AMoodText) = 0;
	virtual QIcon moodIcon(const QString &keyname) const = 0;
	virtual QString moodName(const QString &keyname) const = 0;
	virtual QString contactMoodKey(const Jid &contactJid) const = 0;
	virtual QIcon contactMoodIcon(const Jid &contactJid) const = 0;
	virtual QString contactMoodName(const Jid &contactJid) const = 0;
	virtual QString contactMoodText(const Jid &contactJid) const = 0;
//signals:
};

Q_DECLARE_INTERFACE(IUserMood,"Vacuum.ExternalPlugin.IUserMood/0.1")


#endif // IUSERMOOD_H
