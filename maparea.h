#ifndef MAPAREA_H
#define MAPAREA_H

#include <QtGui>
#include <QBrush>
#include <QPen>
#include <QWidget>
#include "parser.h"
#include "mainwindow.h"
#include <QHash>
#include "annotation.h"
#include "gene.h"
#include "contigList.h"

using namespace std;

class MapArea : public QWidget
{
    Q_OBJECT

public:
    MapArea(MainWindow *mainWindow, QWidget *parent = 0);
    ~MapArea();
    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    QScrollBar* getHScrollBar() const;
    QScrollBar* getVScrollBar() const;
    int getMaxHScrollbarValue() const;
    void setCurrentContigIndex(const int);
    void insertContigFile(const int &, const int &);
    int getSnpThreshold() const;
    QGroupBox* getGroupBox();

public slots:
	void zoomSliderMoved(int);
	void hScrollbarAction(int);
	void initialize();
	void goToPos(const int, const int);
	//void goToPos(const int, const int, const int);
	void annotationLoaded();
	void bookmark();
	void loadBookmark(const QString &);
	void getContigOrderIdHash();
	void exportSelection();
	void setSnpThreshold(const int);
	void setMaxVScrollbarValue(const int);
	//void highlight(const QRect &);
	void resetHighlighting();
	void setVScrollbarValue(const int);
	void showSearchResults(QMap<int, QMap<int, int> *> *);

protected:
    void paintEvent(QPaintEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    //void mousePressEvent(QMouseEvent *event);
    //void mouseReleaseEvent(QMouseEvent *event);
    void resizeEvent(QResizeEvent */*event*/);

private:
    void drawContig(QPainter &, const int &);
    void drawFragments(QPainter &, const int &);
    void drawGenes(QPainter &, QList<Annotation *> &, int, const QString &);
    void drawSnps(QPainter &, QList<Annotation *> &, int, const QString &);
    void drawCustomTrack(QPainter &, QList<Annotation *> &, int, const QString &);
    int convertPointToBases(const QPoint &);
    int convertYPos(const QPoint &p);
    void highlightSearchResults(QPainter &, const int);

    MainWindow *mainWindow;	/* Holds a pointer to the main window */
    QScrollBar *vScrollBar;	/* Holds a pointer to the vertical scroll bar widget */
    QScrollBar *hScrollBar;	/* Holds a pointer to the horizontal scroll bar widget */

    float pointSize;		/* Holds the font size at the current zoom level */
    float padding;			/* Holds the padding space around a base character */
    float dblPadding;		/* Holds twice the padding space */
    int numBases;			/* Holds the number of bases that should be displayed at the current zoom level */
    int halfNumBases;		/* Variable that holds the value of (numBases / 2) */
    int contigStartPos;		/* Used to keep track of the beginning of the image */
    int contigEndPos;		/* Used to keep track of the end of the image */
    int rateOfChange;		/* Holds the factor by which number of bases increases/decreases when zoom slider is moved */
	int currentContigIndex;	/* Holds the index of the current contig */
	int oldContigIndex;		/* Holds the index of the old contig */
	int midPos;				/* Keeps track of the mid-position */
	Contig *contig;
	QHash<int, int> contigOrderIdHash;	/* Holds contig order as key and ID as value */
	int snpThreshold;
	int adjustedWidth;
	QPixmap highlightPixmap;			/* Pixmap containing highlighting */
	int highlightStartPos;
	int highlightEndPos;
	QVBoxLayout *layout;
	QGroupBox *groupBox;
	int fragAreaMinX;
	int fragAreaMaxX;
	int fragAreaMinY;
	int fragAreaMaxY;
	int numFragsDisplayed;
	int fragOffset;
	QPoint mousePressPos;
	bool isSearchHighlightEnabled;
	QMap<int, QMap<int, int> *> *searchResultsMap;
	ContigList *contigList;

    static QPen penBlue;	/* Blue colored pen */
    static QPen penGreen;	/* Green colored pen */
    static QPen penRed;		/* Red colored pen */
    static QPen penMagenta;	/* Magenta colored pen */
    static QPen penBlack;	/* Black colored pen */
    static QPen penGray;	/* Gray colored pen */
    static QPen penDarkYellow;	/* Dark yellow colored pen */
    static QPen penIndigo;	/* Indigo colored pen */
    static QFont font;		/* Holds the font in which the bases will be displayed */
    static QBrush brush;	/* Holds the paint brush that will be used to paint the background of SNPs */
    static QBrush lightGrayBrush;	/* Holds light gray color */
    static QBrush brushIndigo;		/* Indigo colored brush */

    signals:
    void contigChanged(Contig *);
    void fileChanged(const QString &);
    void messageChanged(const QString &);
    void zoomedIn(bool);
    void viewChanged(const Contig *);
    void searchMessageEmitted(const QString &);
    void seqHighlighted(bool);
    void zoom(const int);
};


#endif
