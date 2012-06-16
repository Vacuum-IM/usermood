#include "usermooddialog.h"

UserMoodDialog::UserMoodDialog(IUserMood *AUserMood, const QMap<QString, MoodData> &AMoodsCatalog, Jid &AStreamJid, QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowTitle(tr("Set mood"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_USERMOOD, 0, 0, "windowIcon");

	FUserMood = AUserMood;
	FStreamJid = AStreamJid;

	QMap<QString, MoodData>::const_iterator it = AMoodsCatalog.constBegin();
	for(; it != AMoodsCatalog.constEnd(); ++it)
	{
		ui.cmbMood->addItem(it->icon, it->locname, it.key());
	}
	ui.cmbMood->model()->sort(Qt::AscendingOrder);
	ui.cmbMood->removeItem(ui.cmbMood->findData(MOOD_NULL));
	ui.cmbMood->insertItem(0, FUserMood->moodIcon(MOOD_NULL), FUserMood->moodName(MOOD_NULL), MOOD_NULL);
	ui.cmbMood->insertSeparator(1);

	int pos;
	pos = ui.cmbMood->findData(FUserMood->contactMoodKey(AStreamJid));
	if(pos != -1)
	{
		ui.cmbMood->setCurrentIndex(pos);
		ui.pteText->setPlainText(FUserMood->contactMoodText(AStreamJid));
	}
	else
		ui.cmbMood->setCurrentIndex(0);

	connect(ui.dbbButtons, SIGNAL(accepted()), SLOT(onDialogAccepted()));
	connect(ui.dbbButtons, SIGNAL(rejected()), SLOT(reject()));
}

void UserMoodDialog::onDialogAccepted()
{
	QString AMoodKey = ui.cmbMood->itemData(ui.cmbMood->currentIndex()).toString();
	QString AMoodText = ui.pteText->toPlainText();
	FUserMood->setMood(FStreamJid, AMoodKey, AMoodText);
	accept();
}

UserMoodDialog::~UserMoodDialog()
{

}
