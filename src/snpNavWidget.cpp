
#include <QSqlQuery>
#include <QSqlError>
#include "snpNavWidget.h"
#include "database.h"


/**
 * Constructor
 */
SnpNavWidget::SnpNavWidget(QWidget *parent)
	: QWidget(parent)
{
    firstButton = new QToolButton;
    firstButton->setIcon(QIcon(":/images/resultset_first.png"));
    firstButton->setText(tr("First SNP"));
    firstButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    firstButton->setStatusTip(tr("First SNP"));
    firstButton->setToolTip(tr("First SNP"));
    firstButton->setDisabled(true);
    firstButton->setAutoRaise(true);
    connect(firstButton, SIGNAL(clicked()), this, SLOT(goToFirstSnp()));

    prevButton = new QToolButton;
    prevButton->setIcon(QIcon(":/images/resultset_previous.png"));
    prevButton->setText(tr("Previous SNP"));
    prevButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    prevButton->setStatusTip(tr("Previous SNP"));
    prevButton->setToolTip(tr("Previous SNP"));
    prevButton->setDisabled(true);
    prevButton->setAutoRaise(true);
    connect(prevButton, SIGNAL(clicked()), this, SLOT(goToPrevSnp()));

    nextButton = new QToolButton;
    nextButton->setIcon(QIcon(":/images/resultset_next.png"));
    nextButton->setText(tr("Next SNP"));
    nextButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    nextButton->setStatusTip(tr("Next SNP"));
    nextButton->setToolTip(tr("Next SNP"));
    nextButton->setDisabled(true);
    nextButton->setAutoRaise(true);
    connect(nextButton, SIGNAL(clicked()), this, SLOT(goToNextSnp()));

    lastButton = new QToolButton;
    lastButton->setIcon(QIcon(":/images/resultset_last.png"));
    lastButton->setText(tr("Last SNP"));
    lastButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    lastButton->setStatusTip(tr("Last SNP"));
    lastButton->setToolTip(tr("Last SNP"));
    lastButton->setDisabled(true);
    lastButton->setAutoRaise(true);
    connect(lastButton, SIGNAL(clicked()), this, SLOT(goToLastSnp()));

    hBoxLayout = new QHBoxLayout;
    hBoxLayout->setSizeConstraint(QLayout::SetFixedSize);
    hBoxLayout->addWidget(firstButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(prevButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(nextButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(lastButton, 0, Qt::AlignLeft);

    groupBox = new QGroupBox(tr("Polymorphisms"));
    groupBox->setLayout(hBoxLayout);
    groupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	QString tmp = tr("<b>Polymorphisms</b> provides navigational "
			"buttons to allow the user "
			"to jump between the different SNPs in the contig.");
	groupBox->setWhatsThis(tmp);

    contig = NULL;
    threshold = 0;

}


/**
 * Destructor
 */
SnpNavWidget::~SnpNavWidget()
{
	delete firstButton;
	delete prevButton;
	delete nextButton;
	delete lastButton;
	delete hBoxLayout;
	delete groupBox;
	contig = NULL;
}


/*
 * Show the first SNP.
 *
 * For this we need to find a SNP with the least distance.
 */
void SnpNavWidget::goToFirstSnp()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getSnpDBName());
	QSqlQuery query(db);
	QString str;

	str = "select pos "
			" from snp_pos "
			" where contig_id = " + QString::number(contig->id) +
			" and variationPercent >= " + QString::number(threshold) +
			" order by pos asc "
			" limit 1 ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'first SNP' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, query.value(0).toInt());
		emit goToVPos(0);
	}

	db.close();
}


/*
 * Show the previous SNP
 *
 * For this we need to find a SNP with greatest distance
 * that is less than the contig's current start position.
 */
void SnpNavWidget::goToPrevSnp()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getSnpDBName());
	QSqlQuery query(db);
	QString str;

	str = "select pos "
			" from snp_pos "
			" where contig_id = " + QString::number(contig->id) +
			" and pos < " + QString::number(Contig::startPos) +
			" and variationPercent >= " + QString::number(threshold) +
			" order by pos desc "
			" limit 1 ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'previous SNP' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, query.value(0).toInt());
		emit goToVPos(0);
	}
	else
		emit messageChanged(tr("No more SNPs"));

	db.close();
}


/*
 * Show the next SNP
 *
 * For this we need to find a SNP with the least distance
 * that is greater than the contig's current start position.
 */
void SnpNavWidget::goToNextSnp()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getSnpDBName());
	QSqlQuery query(db);
	QString str;

	str = "select pos "
			" from snp_pos "
			" where contig_id = " + QString::number(contig->id) +
			" and pos > " + QString::number(Contig::startPos) +
			" and variationPercent >= " + QString::number(threshold) +
			" order by pos asc "
			" limit 1 ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'next SNP' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, query.value(0).toInt());
		emit goToVPos(0);
	}
	else
		emit messageChanged(tr("No more SNPs"));

	db.close();
}


/*
 * Show the last SNP.
 *
 * For this we find the SNP with the greatest distance.
 */
void SnpNavWidget::goToLastSnp()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getSnpDBName());
	QSqlQuery query(db);
	QString str;

	str = "select pos "
			" from snp_pos "
			" where contig_id = " + QString::number(contig->id) +
			" and variationPercent >= " + QString::number(threshold) +
			" order by pos desc "
			" limit 1 ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'last SNP' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, query.value(0).toInt());
		emit goToVPos(0);
	}

	db.close();
}


/**
 * Sets the current contig
 */
void SnpNavWidget::setContig(Contig *c)
{
	contig = c;
}


/**
 * Sets SNP threshold value
 */
void SnpNavWidget::setThreshold(const int val)
{
	threshold = val;
}


/**
 * Enables or disables child widgets
 */
void SnpNavWidget::setEnabled(bool enable)
{
	firstButton->setEnabled(enable);
	prevButton->setEnabled(enable);
	nextButton->setEnabled(enable);
	lastButton->setEnabled(enable);
}


/**
 * Returns a pointer to the group box
 */
QGroupBox* SnpNavWidget::getGroupBox()
{
	return groupBox;
}

