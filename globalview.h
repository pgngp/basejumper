
#ifndef GLOBALVIEW_H_
#define GLOBALVIEW_H_

#include <QtGui>
#include <QWidget>
#include <QMap>
#include <QHash>
#include "contig.h"
#include "globalViewPainterThread.h"

class GlobalView : public QWidget
{
	Q_OBJECT

public:
	GlobalView(QWidget *parent = 0);
	~GlobalView();
    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    public slots:
    void clean();
    void updateView(const Contig *);
	void createContigPixmap();
	void createHighlightingPixmap(const QRect &);
	QGroupBox* getGroupBox();
	void setContig(Contig *);
	void showSearchResults(QMap<int, QMap<int, int> *> *);
	void resetHighlighting();
	void setContigData(const QList<ContigStruct *> &);

protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent *);

private:
	int lineSize;
	QFont font;
	QPixmap contigPixmap;
	QPixmap highlightingPixmap;
	QPixmap searchResultsPixmap;
	QVBoxLayout *layout;
	QGroupBox *groupBox;
	QScrollArea *scrollArea;
	Contig *contig;
	GlobalViewPainterThread thread;
	QList<ContigStruct *> contigStructList;
	static QPen penGray;

	void getContigs();
	void getFrags();
	void drawContigs(QPainter &);
	void drawFrags(QPainter &);
	int getContigId(const QPoint &);
	int getNucleotidePos(const int, const QPoint &);
	void setVScrollbarMax(const int);

	private slots:
	void showImage();

	signals:
	void globalViewClicked(const int, const int);
	void contigClicked(int);
    void messageChanged(const QString &);
    void viewChanged();
    void vScrollbarMaxChanged(const int);
    void setVisibleVScrollbar(bool);
};

#endif /* GLOBALVIEW_H_ */
