
#ifndef ZOOMWIDGET_H_
#define ZOOMWIDGET_H_

#include <QtGui>
#include <QWidget>
#include "contig.h"

class ZoomWidget : public QWidget
{
    Q_OBJECT

public:
    ZoomWidget(QWidget *parent = 0);
    ~ZoomWidget();
    QGroupBox *getGroupBox();

    public slots:
    void setIntervals(int);
    void reset();
    void setEnabled(bool);
    void zoom(const int);
    void setContig(Contig *);

private:
	QVBoxLayout *vBoxLayout;
    QToolButton *zoomInButton;
    QToolButton *zoomOutButton;
    QSlider *slider;
    QGroupBox *groupBox;
    Contig *contig;

    static int previousLevel;

    private slots:
    void zoomIn();
    void zoomOut();

    signals:
    void sliderMoved(int);
};

#endif /* ZOOMWIDGET_H_ */
