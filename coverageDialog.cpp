/*
 * gene.cpp
 *
 *  Created on: May 6, 2009
 *      Author: John Obenauer, Pankaj Gupta
 */

#include "coverageDialog.h"
#include <QSqlQuery>
#include <QSqlError>

/*
 * Constructor
 */
CoverageDialog::CoverageDialog(QWidget *parent, Qt::WindowFlags flags)
	: QDialog(parent, flags)
{
	tableView = new QTableView(this);

	vBoxLayout = new QVBoxLayout;
	vBoxLayout->addWidget(tableView, 0, Qt::AlignLeft);
}


/*
 * Destructor
 */
CoverageDialog::~CoverageDialog()
{
	delete tableView;
	delete vBoxLayout;
}

