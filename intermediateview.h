
#ifndef INTERMEDIATEVIEW_H_
#define INTERMEDIATEVIEW_H_

#include <QtGui>
#include <QWidget>
#include "contig.h"
#include "intermediateViewPainterThread.h"

using namespace std;

class IntermediateView : public QWidget
{
    Q_OBJECT

public:
	IntermediateView(QWidget *parent = 0);
	~IntermediateView();
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    QGroupBox* getGroupBox();

    public slots:
    void reset();
    void updateView(const Contig *);
    void createContigPixmap();
    void createHighlightingPixmap(const QRect &);
    void setContig(Contig *);
    void showSearchResults(QMap<int, QMap<int, int> *> *);
    void resetHighlighting();

protected:
    void paintEvent(QPaintEvent *event);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void resizeEvent(QResizeEvent *);

private:
	int oldContigOrder;
	int oldContigId;
	int lineSize;
	QPixmap contigPixmap;
	QPixmap highlightingPixmap;
	QPixmap searchResultsPixmap;
	QVBoxLayout *layout;
	QGroupBox *groupBox;
	Contig *contig;
	QMap<int, QMap<int, int> *> *searchResultsMap;
	IntermediateViewPainterThread thread;
	static QPen penGray;
	IVPainterThreadNS::ContigStruct contigStruct;

	void highlightSearchResults();

	private slots:
	void showImage();
	void setContigData(const IVPainterThreadNS::ContigStruct &);

	signals:
	void messageChanged(const QString &);
	void intermediateViewClicked(const int, const int);
};
#endif /* INTERMEDIATEVIEW_H_ */
