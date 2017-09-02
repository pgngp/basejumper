
#include <QSqlQuery>
#include <QSqlError>
#include "readsNavWidget.h"
#include "database.h"

/**
 * Constructor
 */
ReadsNavWidget::ReadsNavWidget(QWidget *parent)
	: QWidget(parent)
{
    firstButton = new QToolButton;
    firstButton->setIcon(QIcon(":/images/resultset_first.png"));
    firstButton->setText(tr("First read"));
    firstButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    firstButton->setStatusTip(tr("First read"));
    firstButton->setToolTip(tr("First read"));
    firstButton->setDisabled(true);
    firstButton->setAutoRaise(true);
    connect(firstButton, SIGNAL(clicked()), this, SLOT(goToFirstRead()));

    prevButton = new QToolButton;
    prevButton->setIcon(QIcon(":/images/resultset_previous.png"));
    prevButton->setText(tr("Previous read"));
    prevButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    prevButton->setStatusTip(tr("Previous read"));
    prevButton->setToolTip(tr("Previous read"));
    prevButton->setDisabled(true);
    prevButton->setAutoRaise(true);
    connect(prevButton, SIGNAL(clicked()), this, SLOT(goToPrevRead()));

    nextButton = new QToolButton;
    nextButton->setIcon(QIcon(":/images/resultset_next.png"));
    nextButton->setText(tr("Next read"));
    nextButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    nextButton->setStatusTip(tr("Next read"));
    nextButton->setToolTip(tr("Next read"));
    nextButton->setDisabled(true);
    nextButton->setAutoRaise(true);
    connect(nextButton, SIGNAL(clicked()), this, SLOT(goToNextRead()));

    lastButton = new QToolButton;
    lastButton->setIcon(QIcon(":/images/resultset_last.png"));
    lastButton->setText(tr("Last read"));
    lastButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    lastButton->setStatusTip(tr("Last read"));
    lastButton->setToolTip(tr("Last read"));
    lastButton->setDisabled(true);
    lastButton->setAutoRaise(true);
    connect(lastButton, SIGNAL(clicked()), this, SLOT(goToLastRead()));

    hBoxLayout = new QHBoxLayout;
    hBoxLayout->setSizeConstraint(QLayout::SetFixedSize);
    hBoxLayout->addWidget(firstButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(prevButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(nextButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(lastButton, 0, Qt::AlignLeft);
    //hBoxLayout->setContentsMargins(2, 5, 2, 5);

    groupBox = new QGroupBox(tr("Reads"));
    groupBox->setLayout(hBoxLayout);
    groupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	QString tmp = tr("<b>Reads</b> provides navigational "
			"buttons to allow the user "
			"to jump between the different fragments in the contig.");
	groupBox->setWhatsThis(tmp);

    contig = NULL;
}


/**
 * Destructor
 */
ReadsNavWidget::~ReadsNavWidget()
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
 * Go to first read
 */
void ReadsNavWidget::goToFirstRead()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getFragDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos, yPos "
			" from fragment "
			" where contig_id = " + QString::number(contig->id) +
			" order by startPos asc "
			" limit 1 ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'first read' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(query.value(1).toInt() * 2);
	}

	db.close();
}


/*
 * Go to previous read
 */
void ReadsNavWidget::goToPrevRead()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getFragDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos, yPos "
			" from fragment "
			" where contig_id = " + QString::number(contig->id) + ""
			" and startPos < " + QString::number(Contig::startPos) + ""
			" order by startPos desc "
			" limit 1 ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'previous read' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(query.value(1).toInt() * 2);
	}
	else
		emit messageChanged(tr("No more reads"));

	db.close();
}


/*
 * Go to next read
 */
void ReadsNavWidget::goToNextRead()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getFragDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos, yPos "
			" from fragment "
			" where contig_id = " + QString::number(contig->id) +
			" and startPos > " + QString::number(Contig::startPos+1) +
			" order by startPos asc "
			" limit 1 ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'next read' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(query.value(1).toInt() * 2);
	}
	else
		emit messageChanged(tr("No more reads"));

	db.close();
}


/*
 * Go to last read
 */
void ReadsNavWidget::goToLastRead()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getFragDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos, yPos "
			" from fragment "
			" where contig_id = " + QString::number(contig->id) +
			" order by startPos desc "
			" limit 1";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this,
				tr("Basejumper"),
				tr("Error fetching 'last read' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(query.value(1).toInt() * 2);
	}

	db.close();
}


/**
 * Sets the current contig
 */
void ReadsNavWidget::setContig(Contig *c)
{
	contig = c;
}


/**
 * Returns a pointer to the group box
 */
QGroupBox* ReadsNavWidget::getGroupBox()
{
	return groupBox;
}


/**
 * Enables or disables child widgets
 */
void ReadsNavWidget::setEnabled(bool enable)
{
	firstButton->setEnabled(enable);
	prevButton->setEnabled(enable);
	nextButton->setEnabled(enable);
	lastButton->setEnabled(enable);
}

