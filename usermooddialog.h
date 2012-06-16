#ifndef USERMOODDIALOG_H
#define USERMOODDIALOG_H

#include <QDialog>

#include <definitions/resources.h>
#include <utils/iconstorage.h>
#include <utils/jid.h>

#include "iusermood.h"
#include "ui_usermooddialog.h"

class UserMoodDialog : public QDialog
{
	Q_OBJECT

public:
	UserMoodDialog(IUserMood *AUserMood, const QMap<QString, MoodData> &AMoodsCatalog, Jid &AStreamJid, QWidget *AParent = 0);
	~UserMoodDialog();

protected slots:
	void onDialogAccepted();

private:
	Ui::UserMoodDialog ui;
	IUserMood *FUserMood;

	Jid FStreamJid;
};

#endif // USERMOODDIALOG_H
