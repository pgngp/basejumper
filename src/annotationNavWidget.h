/*
 * annotationNavWidget.h
 *
 *  Created on: Jun 2, 2009
 *      Author: pgupta
 */

#ifndef ANNOTATIONNAVWIDGET_H_
#define ANNOTATIONNAVWIDGET_H_
#include <QtGui>
#include <QWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QToolButton>
#include <QBoxLayout>
#include "contig.h"

class AnnotationNavWidget : public QWidget
{
	Q_OBJECT

public:
	enum AnnotType {Custom, Gene, Snp};

	AnnotationNavWidget(QWidget *parent=0);
	~AnnotationNavWidget();
	QGroupBox *getGroupBox();

	public slots:
	void setContig(Contig *);
	void setEnabled(bool);
	void setItems();

private:
	QComboBox *comboBox;
    QToolButton *firstButton;
    QToolButton *prevButton;
    QToolButton *nextButton;
    QToolButton *lastButton;
    QHBoxLayout *hBoxLayout;
    QVBoxLayout *vBoxLayout;
    QGroupBox *groupBox;
    Contig *contig;
    enum AnnotType type;
    QHash<QString, int> nameIdHash;
    int track;
    static QString desc;

	void enableButtons(bool);

	private slots:
	void trackSelected(const QString &);
	void goToFirstAnnotation();
	void goToPrevAnnotation();
	void goToNextAnnotation();
	void goToLastAnnotation();

    signals:
	void goToHPos(const int, const int);
	void goToVPos(const int);
	void messageChanged(const QString &);
};

#endif /* ANNOTATIONNAVWIDGET_H_ */
