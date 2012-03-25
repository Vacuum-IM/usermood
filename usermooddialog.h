#ifndef USERMOODDIALOG_H
#define USERMOODDIALOG_H

#include <QDialog>
#include <QPair>
#include <utils/jid.h>

#include "usermood.h"
#include "ui_usermooddialog.h"

class UserMood;

class userMoodDialog : public QDialog
{
    Q_OBJECT
    
public:
    userMoodDialog(const QMap<QString, QString> &AMoodsCatalog, QMap<QString, QPair<QString, QString> > &AContactMood, Jid &streamJid, UserMood *AUserMood, QWidget *parent = 0);
    ~userMoodDialog();

protected slots:
    void onDialogAccepted();
    
private:
    Ui::userMoodDialog ui;
	UserMood *FUserMood;

	Jid FStreamJid;
};

#endif // USERMOODDIALOG_H
