#include <QtGui>
#include "bookmarkdialog.h"
#include "maparea.h"
#include <QSqlQuery>
#include <QSqlError>

#define MAX_CHARS			30
#define	TEXT_BOX_MAX_WIDTH	300


/**
 * Constructor
 */
BookmarkDialog::BookmarkDialog(QWidget *parent)
	: QDialog(parent)
{
	this->setParent(parent);
	this->setWindowTitle(tr("Name the bookmark"));

	/* Name label */
	nameLabel = new QLabel;
	nameLabel->setText(tr("Bookmark name: "));

	/* Name line edit box */
	nameLineEdit = new QLineEdit;
	nameLineEdit->setMaxLength(MAX_CHARS);
	nameLineEdit->setMinimumWidth(TEXT_BOX_MAX_WIDTH);
	connect(nameLineEdit,
			SIGNAL(textChanged(const QString &)),
			this,
			SLOT(enableSaveButton(const QString &)));

	/* Save button */
	saveButton = new QPushButton;
	saveButton->setText(tr("Save"));
	saveButton->setDisabled(true);
	saveButton->setIcon(QIcon(":/images/disk.png"));
	connect(saveButton, SIGNAL(clicked()), this, SLOT(saveBookmark()));

	/* Cancel button */
	cancelButton = new QPushButton;
	cancelButton->setText(tr("Cancel"));
	cancelButton->setIcon(QIcon(":/images/cancel.png"));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

	/* Name horizontal box layout */
	nameHBoxLayout = new QHBoxLayout;
	nameHBoxLayout->addWidget(nameLabel);
	nameHBoxLayout->addWidget(nameLineEdit, 1, Qt::AlignLeft);

	/* Buttons horizontal box layout */
	buttonsHBoxLayout = new QHBoxLayout;
	buttonsHBoxLayout->addWidget(saveButton, 1, Qt::AlignRight);
	buttonsHBoxLayout->addWidget(cancelButton);

	/* Vertical layout */
	vBoxLayout = new QVBoxLayout;
	vBoxLayout->addLayout(nameHBoxLayout);
	vBoxLayout->addSpacing(1);
	vBoxLayout->addLayout(buttonsHBoxLayout);

	this->setLayout(vBoxLayout);
}


/**
 * Destructor
 */
BookmarkDialog::~BookmarkDialog()
{
	delete nameLabel;
	delete nameLineEdit;
	delete saveButton;
	delete cancelButton;
	delete nameHBoxLayout;
	delete buttonsHBoxLayout;
	delete vBoxLayout;
}


/**
 * Saves the selected region as a bookmark
 */
void BookmarkDialog::saveBookmark()
{
	QString name, queryStr;
	int startPos, contigId;
	QSqlQuery query;
	MapArea *mapArea;

	/* Get values */
	mapArea = reinterpret_cast<MapArea *>(this->parentWidget());
	name = nameLineEdit->text();
	contigId = Contig::getCurrentId();
	startPos = Contig::getStartPos();

	/* Store in DB */
	queryStr = "insert into bookmark "
			" (name, filepath, contigName, startPos) "
			" values "
			" ('" + name + "', "
			" (select file.filepath "
			" from file join contig "
			" on file.id = contig.fileId "
			" where contig.id = " + QString::number(contigId) + "), "
			" (select name from contig where id = " + QString::number(contigId) + "), "
			+ QString::number(startPos) + ")";
	if (!query.exec(queryStr))
	{
		QMessageBox::critical(
			this->parentWidget(),
			tr("Basejumper"),
			tr("Error inserting into 'bookmark' table.\nReason: "
					+ query.lastError().text().toAscii()));
		return;
	}

	this->close();
	emit bookmarkAdded(name);
}


/**
 * Enables or disables the 'saveButton' depending on whether
 * text has been entered into the 'nameLineEdit' box
 */
void BookmarkDialog::enableSaveButton(const QString &text)
{
	if (text.isEmpty())
		saveButton->setDisabled(true);
	else
		saveButton->setEnabled(true);
}

