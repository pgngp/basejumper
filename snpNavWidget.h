
#ifndef SNPNAVWIDGET_H_
#define SNPNAVWIDGET_H_

#include <QtGui>
#include <QWidget>
#include "contig.h"

class SnpNavWidget : public QWidget
{
	Q_OBJECT

public:
	SnpNavWidget(QWidget *parent = 0);
	~SnpNavWidget();
	QGroupBox *getGroupBox();

	public slots:
	void setContig(Contig *);
	void setThreshold(const int);
	void setEnabled(bool);

private:
	QToolButton *firstButton;
	QToolButton *prevButton;
	QToolButton *nextButton;
	QToolButton *lastButton;
	QHBoxLayout *hBoxLayout;
	QGroupBox *groupBox;
	Contig *contig;
	int threshold;

	private slots:
	void goToFirstSnp();
	void goToPrevSnp();
	void goToNextSnp();
	void goToLastSnp();

    signals:
	void goToHPos(const int, const int);
	void goToVPos(const int);
	void messageChanged(const QString &);

};

#endif /* SNPNAVWIDGET_H_ */
