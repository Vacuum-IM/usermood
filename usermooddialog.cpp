#include "usermooddialog.h"
#include "ui_usermooddialog.h"
#include "usermood.h"
#include <usermood.h>

userMoodDialog::userMoodDialog(const QMap<QString, QString> &AMoodsCatalog, QMap<QString, QPair<QString, QString> > &AContactMood, Jid &streamJid, UserMood *AUserMood, QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose,true);
    setWindowTitle(tr("Set mood"));

	FUserMood = AUserMood;
	FStreamJid = streamJid;

    QMap<QString, QString>::const_iterator it = AMoodsCatalog.begin();
    for (;it != AMoodsCatalog.end(); ++it)
    {
        ui.cmbMood->addItem(it.value(),it.key());
    }
	
	int pos;
	QString jid = streamJid.pBare();
	pos = ui.cmbMood->findData(AContactMood.value(jid).first);

	if (pos != -1)
	{
		ui.cmbMood->setCurrentIndex(pos);
		ui.pteText->setPlainText(AContactMood.value(jid).second);
	}

    connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));

}

void userMoodDialog::onDialogAccepted()
{
 
    QString moodName = ui.cmbMood->currentText();
    QString moodText = ui.pteText->toPlainText();
	FUserMood->isSetMood(FStreamJid, moodName, moodText);

    accept();
}


userMoodDialog::~userMoodDialog()
{

}
