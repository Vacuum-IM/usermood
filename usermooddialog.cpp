#include "usermooddialog.h"

UserMoodDialog::UserMoodDialog(IUserMood *AUserMood, const QHash<QString, MoodData> &AMoodsCatalog, Jid &AStreamJid, QWidget *parent) : QDialog(parent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowTitle(tr("Set mood"));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this, MNI_USERMOOD, 0, 0, "windowIcon");

	FUserMood = AUserMood;
	FStreamJid = AStreamJid;

	QHashIterator<QString, MoodData> i(AMoodsCatalog);
	while (i.hasNext())
	{
		i.next();
		ui.cmbMood->addItem(i.value().icon, i.value().locname, i.key());
	}
	ui.cmbMood->model()->sort(Qt::AscendingOrder);
	ui.cmbMood->removeItem(ui.cmbMood->findData(MOOD_NULL));
	ui.cmbMood->insertItem(0, FUserMood->moodIcon(MOOD_NULL), FUserMood->moodName(MOOD_NULL), MOOD_NULL);
	ui.cmbMood->insertSeparator(1);

	int pos;
	pos = ui.cmbMood->findData(FUserMood->contactMoodKey(FStreamJid, FStreamJid));
	if(pos != -1)
	{
		ui.cmbMood->setCurrentIndex(pos);
		ui.pteText->setPlainText(FUserMood->contactMoodText(FStreamJid, FStreamJid));
	}
	else
		ui.cmbMood->setCurrentIndex(0);

	connect(ui.dbbButtons, SIGNAL(accepted()), SLOT(onDialogAccepted()));
	connect(ui.dbbButtons, SIGNAL(rejected()), SLOT(reject()));
}

void UserMoodDialog::onDialogAccepted()
{
	Mood data;
	data.keyname = ui.cmbMood->itemData(ui.cmbMood->currentIndex()).toString();
	data.text = ui.pteText->toPlainText();
	FUserMood->setMood(FStreamJid, data);
	accept();
}

UserMoodDialog::~UserMoodDialog()
{

}
