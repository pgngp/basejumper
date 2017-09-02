
#ifndef READSNAVWIDGET_H_
#define READSNAVWIDGET_H_

#include <QtGui>
#include <QWidget>
#include "contig.h"

class ReadsNavWidget : public QWidget
{
	Q_OBJECT

public:
	ReadsNavWidget(QWidget *parent = 0);
	~ReadsNavWidget();
	QGroupBox *getGroupBox();

	public slots:
	void setContig(Contig *);
	void setEnabled(bool);

private:
    QToolButton *firstButton;
    QToolButton *prevButton;
    QToolButton *nextButton;
    QToolButton *lastButton;
    QHBoxLayout *hBoxLayout;
    QGroupBox *groupBox;
    Contig *contig;

    private slots:
    void goToFirstRead();
    void goToPrevRead();
    void goToNextRead();
    void goToLastRead();

    signals:
	void goToHPos(const int, const int);
	void goToVPos(const int);
	void messageChanged(const QString &);
};


#endif /* READSNAVWIDGET_H_ */
