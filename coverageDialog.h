/*
 * coverageDialog.h
 *
 *  Created on: May 6, 2009
 *      Author: John Obenauer, Pankaj Gupta
 */

#ifndef COVERAGEDIALOG_H_
#define COVERAGEDIALOG_H_

#include <QWidget>
#include <QDialog>
#include <QBoxLayout>
#include <QTableView>

class CoverageDialog : public QDialog
{
	Q_OBJECT

public:
	CoverageDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
	~CoverageDialog();

private:
	QVBoxLayout *vBoxLayout;
	QTableView *tableView;
};

#endif /* COVERAGEDIALOG_H_ */
