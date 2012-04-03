#include "usermooddialog.h"

userMoodDialog::userMoodDialog(const QMap<QString, MoodData> &AMoodsCatalog, QMap<QString, QPair<QString, QString> > &AContactMood, Jid &streamJid, UserMood *AUserMood, QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose,true);
    setWindowTitle(tr("Set mood"));

	FUserMood = AUserMood;
	FStreamJid = streamJid;

    QMap<QString, MoodData>::const_iterator it = AMoodsCatalog.constBegin();
    for (;it != AMoodsCatalog.constEnd(); ++it)
    {
		ui.cmbMood->addItem(it->name,it.key());
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
 
    QString moodKey = ui.cmbMood->itemData(ui.cmbMood->currentIndex()).toString(); 
    QString moodText = ui.pteText->toPlainText();
    FUserMood->isSetMood(FStreamJid, moodKey, moodText);

    accept();
}


userMoodDialog::~userMoodDialog()
{

}
