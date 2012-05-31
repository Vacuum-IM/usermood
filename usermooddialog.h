#ifndef USERMOODDIALOG_H
#define USERMOODDIALOG_H

#include <QDialog>

#include <definitions/resources.h>
#include <utils/iconstorage.h>
#include <utils/jid.h>

#include "iusermood.h"
#include "ui_usermooddialog.h"

class userMoodDialog : public QDialog
{
	Q_OBJECT

public:
    userMoodDialog(IUserMood *AUserMood, const QMap<QString, MoodData> &AMoodsCatalog, QMap<QString, MoodContact> &AContactsMood, Jid &AStreamJid, QWidget *AParent = 0);
	~userMoodDialog();

protected slots:
	void onDialogAccepted();

private:
	Ui::userMoodDialog ui;
    IUserMood *FUserMood;

	Jid FStreamJid;
};

#endif // USERMOODDIALOG_H
