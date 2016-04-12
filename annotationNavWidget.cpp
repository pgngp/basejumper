
#include "annotationNavWidget.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "database.h"


QString AnnotationNavWidget::desc = "Select track...";

/**
 * Constructor
 */
AnnotationNavWidget::AnnotationNavWidget(QWidget *parent)
	: QWidget(parent)
{
	comboBox = new QComboBox(this);
	comboBox->addItem(desc, QVariant(-1));
	comboBox->setDisabled(true);
	connect(comboBox, SIGNAL(activated(const QString &)),
			this, SLOT(trackSelected(const QString &)));

    firstButton = new QToolButton;
    firstButton->setIcon(QIcon(":/images/resultset_first.png"));
    firstButton->setText(tr("First Annotation"));
    firstButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    firstButton->setStatusTip(tr("First Annotation"));
    firstButton->setToolTip(tr("First Annotation"));
    firstButton->setDisabled(true);
    firstButton->setAutoRaise(true);
    connect(firstButton, SIGNAL(clicked()), this, SLOT(goToFirstAnnotation()));

    prevButton = new QToolButton;
    prevButton->setIcon(QIcon(":/images/resultset_previous.png"));
    prevButton->setText(tr("Previous Annotation"));
    prevButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    prevButton->setStatusTip(tr("Previous Annotation"));
    prevButton->setToolTip(tr("Previous Annotation"));
    prevButton->setDisabled(true);
    prevButton->setAutoRaise(true);
    connect(prevButton, SIGNAL(clicked()), this, SLOT(goToPrevAnnotation()));

    nextButton = new QToolButton;
    nextButton->setIcon(QIcon(":/images/resultset_next.png"));
    nextButton->setText(tr("Next Annotation"));
    nextButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    nextButton->setStatusTip(tr("Next Annotation"));
    nextButton->setToolTip(tr("Next Annotation"));
    nextButton->setDisabled(true);
    nextButton->setAutoRaise(true);
    connect(nextButton, SIGNAL(clicked()), this, SLOT(goToNextAnnotation()));

    lastButton = new QToolButton;
    lastButton->setIcon(QIcon(":/images/resultset_last.png"));
    lastButton->setText(tr("Last Annotation"));
    lastButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    lastButton->setStatusTip(tr("Last Annotation"));
    lastButton->setToolTip(tr("Last Annotation"));
    lastButton->setDisabled(true);
    lastButton->setAutoRaise(true);
    connect(lastButton, SIGNAL(clicked()), this, SLOT(goToLastAnnotation()));

    hBoxLayout = new QHBoxLayout;
    hBoxLayout->setSizeConstraint(QLayout::SetFixedSize);
    hBoxLayout->addWidget(firstButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(prevButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(nextButton, 0, Qt::AlignLeft);
    hBoxLayout->addWidget(lastButton, 0, Qt::AlignLeft);
    hBoxLayout->setContentsMargins(2, 5, 2, 5);

    vBoxLayout = new QVBoxLayout;
    vBoxLayout->addWidget(comboBox, 0, Qt::AlignTop);
    vBoxLayout->addLayout(hBoxLayout, 0);

    groupBox = new QGroupBox(tr("Annotations"));
    groupBox->setLayout(vBoxLayout);
    groupBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	QString tmp = tr("<b>Annotations</b> provides navigational "
			"buttons to allow the user "
			"to jump between the different annotations (of the same "
			"track) in the contig.");
	groupBox->setWhatsThis(tmp);

    contig = NULL;
    type = Custom;
    track = 0;
}


/**
 * Destructor
 */
AnnotationNavWidget::~AnnotationNavWidget()
{
	delete comboBox;
	delete firstButton;
	delete prevButton;
	delete nextButton;
	delete lastButton;
	delete hBoxLayout;
	delete vBoxLayout;
	delete groupBox;
	contig = NULL;
}


/*
 * Go to the first annotation
 */
void AnnotationNavWidget::goToFirstAnnotation()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getAnnotationDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos "
			" from annotation "
			" where contigId = " + QString::number(contig->id) +
			" and annotationTypeId = " + QString::number(track) +
			" order by startPos asc "
			" limit 1";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'first annotation' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(0);
	}
	db.close();
}


/*
 * Go to the previous annotation
 */
void AnnotationNavWidget::goToPrevAnnotation()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getAnnotationDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos "
			" from annotation "
			" where contigId = " + QString::number(contig->id) +
			" and annotationTypeId = " + QString::number(track) +
			" and startPos < " + QString::number(Contig::startPos) +
			" order by startPos desc "
			" limit 1";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'previous annotation' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(0);
	}
	db.close();
}


/*
 * Go to the next annotation
 */
void AnnotationNavWidget::goToNextAnnotation()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getAnnotationDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos "
			" from annotation "
			" where contigId = " + QString::number(contig->id) +
			" and annotationTypeId = " + QString::number(track) +
			" and startPos > " + QString::number(Contig::startPos+1) +
			" order by startPos asc "
			" limit 1";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'next annotation' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(0);
	}
	db.close();
}


/*
 * Go to the last annotation
 */
void AnnotationNavWidget::goToLastAnnotation()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getAnnotationDBName());
	QSqlQuery query(db);
	QString str;

	str = "select startPos "
			" from annotation "
			" where contigId = " + QString::number(contig->id) +
			" and annotationTypeId = " + QString::number(track) +
			" order by startPos desc "
			" limit 1";
	if (!query.exec(str))
	{
		QMessageBox::critical(
				this->parentWidget(),
				tr("Basejumper"),
				tr("Error fetching 'last annotation' from DB.\nReason: "
						+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	if (query.next())
	{
		emit goToHPos(contig->id, (query.value(0).toInt()-1));
		emit goToVPos(0);
	}
	db.close();
}


/**
 * Sets the current contig
 */
void AnnotationNavWidget::setContig(Contig *c)
{
	contig = c;
}


/**
 * Returns a pointer to the group box
 */
QGroupBox* AnnotationNavWidget::getGroupBox()
{
	return groupBox;
}


/*
 * Set the type of track selected and enable navigation buttons
 */
void AnnotationNavWidget::trackSelected(const QString &text)
{
	/* If the first entry was selected, disable navigation buttons */
	if (text.compare(desc, Qt::CaseInsensitive) == 0)
	{
		enableButtons(false);
		return;
	}

	track = nameIdHash.value(text);
	enableButtons(true);
}


/**
 * Enables/disables the widget
 */
void AnnotationNavWidget::setEnabled(bool b)
{
	if (b)
	{
		comboBox->setEnabled(true);
		if (comboBox->currentIndex() > 0)
			enableButtons(true);
	}
	else
	{
		comboBox->setDisabled(true);
		enableButtons(false);
	}
}


/*
 * Enable/disable navigation buttons
 */
void AnnotationNavWidget::enableButtons(bool b)
{
	firstButton->setEnabled(b);
	prevButton->setEnabled(b);
	nextButton->setEnabled(b);
	lastButton->setEnabled(b);
}


/**
 * Populates the combo box.
 */
void AnnotationNavWidget::setItems()
{
	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getAnnotationDBName());
	QSqlQuery query(db);
	QString str, name;
	int id;

	str = "select id, alias "
			" from annotationType "
			" order by annotOrder asc ";
	if (!query.exec(str))
	{
		QMessageBox::critical(
			this->parentWidget(),
			tr("Basejumper"),
			tr("Error fetching from 'annotationType' table.\nReason: "
					+ query.lastError().text().toAscii()));
		db.close();
		return;
	}

	nameIdHash.clear();
	while (query.next())
	{
		id = query.value(0).toInt();
		name = query.value(1).toString();

		nameIdHash.insert(name, id);
		comboBox->addItem(name, QVariant(id));
	}
	db.close();
}
