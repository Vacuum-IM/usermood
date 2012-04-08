#include "usermooddialog.h"
#include <QDebug>

userMoodDialog::userMoodDialog(const QMap<QString, MoodData> &AMoodsCatalog, QMap<QString, QPair<QString, QString> > &AContactMood, Jid &streamJid, UserMood *AUserMood, QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowTitle(tr("Set mood"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_USERMOOD, 0, 0, "windowIcon");

	FUserMood = AUserMood;
	FStreamJid = streamJid;

	QMap<QString, MoodData>::const_iterator it = AMoodsCatalog.constBegin();
	for(; it != AMoodsCatalog.constEnd(); ++it)
	{
		ui.cmbMood->addItem(it->icon, it->name, it.key());
	}

	ui.cmbMood->model()->sort(Qt::AscendingOrder);
	ui.cmbMood->removeItem(ui.cmbMood->findData(MOOD_NULL));
	ui.cmbMood->insertItem(0, AMoodsCatalog.value(MOOD_NULL).icon, AMoodsCatalog.value(MOOD_NULL).name, MOOD_NULL);
	ui.cmbMood->insertSeparator(1);

	int pos;
	QString jid = streamJid.pBare();
	pos = ui.cmbMood->findData(AContactMood.value(jid).first);

	if(pos != -1)
	{
		ui.cmbMood->setCurrentIndex(pos);
		ui.pteText->setPlainText(AContactMood.value(jid).second);
	}
	else
		ui.cmbMood->setCurrentIndex(0);

	connect(ui.dbbButtons, SIGNAL(accepted()), SLOT(onDialogAccepted()));
	connect(ui.dbbButtons, SIGNAL(rejected()), SLOT(reject()));

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

