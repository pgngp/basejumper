
#include "geneOverlapExportDialog.h"

/**
 * Constructor
 */
GeneOverlapExportDialog::GeneOverlapExportDialog(
		QWidget *parent,
		Qt::WindowFlags f)
	: QDialog(parent, f)
{
	/* Create description label */
	descLabel = new QLabel("Select the scope of genes:", this, Qt::Widget);

	/* Create radio buttons */
	genomeRadioButton = new QRadioButton("Genome", this);
	genomeRadioButton->setChecked(true);
	contigRadioButton = new QRadioButton("Contig", this);
	baseViewRadioButton = new QRadioButton("Base View", this);

	/* Create push buttons */
	exportButton = new QPushButton(
			QIcon(":images/table_go.png"),
			tr("Export"),
			this);
	exportButton->setAutoDefault(true);
	exportButton->setDefault(true);
	connect(exportButton, SIGNAL(clicked()), this, SLOT(exportTriggered()));

	cancelButton = new QPushButton(
			QIcon(":images/cancel.png"),
			tr("Cancel"),
			this);
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelTriggered()));

	/* Create layouts */
	choicesLayout = new QVBoxLayout;
	choicesLayout->addWidget(genomeRadioButton, 0, Qt::AlignTop);
	choicesLayout->addWidget(contigRadioButton, 0, Qt::AlignTop);
	choicesLayout->addWidget(baseViewRadioButton, 0, Qt::AlignTop);

	buttonsLayout = new QHBoxLayout;
	buttonsLayout->addWidget(exportButton, 0, Qt::AlignRight);
	buttonsLayout->addWidget(cancelButton, 0, Qt::AlignRight);

	parentLayout = new QVBoxLayout;
	parentLayout->addWidget(descLabel, 0, Qt::AlignTop);
	parentLayout->addLayout(choicesLayout, 0);
	parentLayout->addLayout(buttonsLayout, 0);

	this->setWindowTitle("Select gene scope");
	this->setLayout(parentLayout);
	this->setModal(true);
}


/**
 * Destructor
 */
GeneOverlapExportDialog::~GeneOverlapExportDialog()
{
	delete descLabel;
	delete genomeRadioButton;
	delete contigRadioButton;
	delete baseViewRadioButton;
	delete exportButton;
	delete cancelButton;
	delete choicesLayout;
	delete buttonsLayout;
	delete parentLayout;
}


/*
 * Sends a signal to GeneOverlapExporter with the scope
 * of the genes to be exported.
 */
void GeneOverlapExportDialog::exportTriggered()
{
	if (genomeRadioButton->isChecked())
		emit scope(1);
	else if (contigRadioButton->isChecked())
		emit scope(2);
	else if (baseViewRadioButton->isChecked())
		emit scope(3);
	this->close();
}


/*
 * Sends a 'null' scope to GeneOverlapExporter object
 */
void GeneOverlapExportDialog::cancelTriggered()
{
	emit scope(0);
	this->close();
}

