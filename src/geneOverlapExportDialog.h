
#ifndef GENEOVERLAPEXPORTDIALOG_H_
#define GENEOVERLAPEXPORTDIALOG_H_
#include <QDialog>
#include <QLabel>
#include <QRadioButton>
#include <QPushButton>
#include <QBoxLayout>

class GeneOverlapExportDialog : public QDialog
{
	Q_OBJECT

public:
	GeneOverlapExportDialog(QWidget *parent=0, Qt::WindowFlags f=0);
	~GeneOverlapExportDialog();

private:
	QLabel *descLabel;
	QRadioButton *genomeRadioButton;
	QRadioButton *contigRadioButton;
	QRadioButton *baseViewRadioButton;
	QPushButton *exportButton;
	QPushButton *cancelButton;
	QVBoxLayout *choicesLayout;
	QHBoxLayout *buttonsLayout;
	QVBoxLayout *parentLayout;

	private slots:
	void exportTriggered();
	void cancelTriggered();

	signals:
	void scope(int);
};

#endif /* GENEOVERLAPEXPORTDIALOG_H_ */
