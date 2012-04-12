#ifndef USERMOODDIALOG_H
#define USERMOODDIALOG_H

#include <QDialog>
#include <utils/jid.h>

#include "usermood.h"
#include "ui_usermooddialog.h"

class UserMood;

class MoodData;
class MoodContact;

class userMoodDialog : public QDialog
{
	Q_OBJECT

public:
	userMoodDialog(const QMap<QString, MoodData> &AMoodsCatalog, QMap<QString, MoodContact> &AContactsMood, Jid &AStreamJid, UserMood *AUserMood, QWidget *parent = 0);
	~userMoodDialog();

protected slots:
	void onDialogAccepted();

private:
	Ui::userMoodDialog ui;
	UserMood *FUserMood;

	Jid FStreamJid;
};

#endif // USERMOODDIALOG_H
