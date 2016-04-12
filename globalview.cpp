
#include <iostream>
#include "globalview.h"
#include <QSqlQuery>
#include <QSqlError>
#include "math.h"
#include "search.h"
#include <cassert>

#define POINT_SIZE			8
#define	FONT_FAMILY			"Sans Serif"
#define	GV_VSCROLLBAR_SINGLE_STEP	40
#define	HORIZONTAL_MARGIN	10

QPen GlobalView::penGray = QPen(Qt::gray);


/**
 * Constructor
 *
 * @param parent : Pointer to the parent widget
 */
GlobalView::GlobalView(QWidget *parent)
	: QWidget(parent)
{
	clean();

	lineSize = 0;
	font = QFont(FONT_FAMILY);
	font.setPointSize(POINT_SIZE);
	contig = NULL;

	setMouseTracking(true);
	setBackgroundRole(QPalette::Base);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	scrollArea = new QScrollArea;
	scrollArea->setWidget(this);
	scrollArea->setWidgetResizable(true);
	scrollArea->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	scrollArea->verticalScrollBar()->setTracking(false);
	scrollArea->verticalScrollBar()->setSingleStep(GV_VSCROLLBAR_SINGLE_STEP);
	scrollArea->verticalScrollBar()->setVisible(false);
	scrollArea->horizontalScrollBar()->setVisible(false);
	scrollArea->setStyleSheet("border-style: hidden");
	scrollArea->setBackgroundRole(QPalette::Base);
	scrollArea->setAutoFillBackground(true);

    layout = new QVBoxLayout;
    layout->addWidget(scrollArea, 0, Qt::AlignTop);

	groupBox = new QGroupBox(tr("Global View"));
	groupBox->setLayout(layout);
	QString tmp = tr("<b>Global View</b> displays a summarized view of "
			"all the currently loaded contigs.");
	groupBox->setWhatsThis(tmp);

	connect(&thread, SIGNAL(finished()), this, SLOT(showImage()));
	qRegisterMetaType<QList<ContigStruct *> >("QList<ContigStruct *>");
	connect(&thread, SIGNAL(contigDataGenerated(const QList<ContigStruct *> &)),
			this, SLOT(setContigData(const QList<ContigStruct *> &)),
			Qt::DirectConnection);
}


/**
 * Destructor
 */
GlobalView::~GlobalView()
{
	//delete scrollArea;
	delete layout;
	//delete groupBox;
	contig = NULL;
}


/**
 * Returns the recommended minimum size of the widget
 */
QSize GlobalView::minimumSizeHint() const
{
    return QSize(700, 90);
}


/**
 * Returns the recommended size of the widget.
 *
 * This function is invoked when the main application window starts up
 * and Global View's size is initialized.
 */
QSize GlobalView::sizeHint() const
{
	return QSize(700, 90);
}


/**
 * Handles paint events
 */
void GlobalView::paintEvent(QPaintEvent */*event*/)
{
	if (Contig::getNumContigs() == 0)
	{
		QPainter painter(this);
		painter.fillRect(0, 0, width(), height(), Qt::transparent);
		painter.setPen(penGray);
		painter.drawRect(0, 0, width() - 1, height() - 1);
		return;
	}

	QRect sourceRect(0, 0, width(), height());
	QRect targetRect(0, 0, width(), height());
	QPainter painter(this);
	painter.drawPixmap(targetRect, highlightingPixmap, sourceRect);
	painter.setPen(penGray);
	painter.drawRect(0, 0, width() - 1, height() - 1);
}


/**
 * Resets data members
 */
void GlobalView::clean()
{

	update();
}


/**
 * Handles mouse move events
 */
void GlobalView::mouseMoveEvent(QMouseEvent *event)
{
	/* If no contigs are loaded, then exit */
    if (Contig::getNumContigs() == 0)
    	return;

    foreach (ContigStruct *contigStruct, contigStructList)
    {
		if (event->y() >= contigStruct->yStart
				&& event->y() <= contigStruct->yEnd
				&& event->x() >= contigStruct->xStart
				&& event->x() <= contigStruct->xEnd)
		{
			this->setCursor(Qt::PointingHandCursor);
			emit messageChanged(tr("Click to display that contig in the Contig view"));
			return;
		}
    }

    this->setCursor(Qt::ArrowCursor);
	emit messageChanged("");
}


/**
 * Handles mouse click events
 */
void GlobalView::mousePressEvent(QMouseEvent *event)
{
	/* If no contigs are loaded, then exit */
    if (Contig::getNumContigs() == 0)
    	return;

	int id = getContigId(event->pos());
	if (id <= 0)
		return;

	int pos = getNucleotidePos(id, event->pos());
	if (pos <= 0)
		return;

	emit globalViewClicked(id, pos);

	return;
}


/*
 * Returns the contig ID that corresponds to the given position
 */
int GlobalView::getContigId(const QPoint &p)
{
	ContigStruct *contigStruct;
	for (int i = 0; i < Contig::getNumContigs(); ++i)
	{
		contigStruct = contigStructList.at(i);
		if (p.x() >= contigStruct->xStart
				&& p.x() <= contigStruct->xEnd
				&& p.y() >= contigStruct->yStart
				&& p.y() <= contigStruct->yEnd)
			return contigStruct->id;
	}

	return -1;
}


/*
 * Returns the nucleotide position corresponding to the given
 * mouse pointer position and contig ID
 */
int GlobalView::getNucleotidePos(const int contigId, const QPoint &p)
{
	ContigStruct *contigStruct;
	for (int i = 0; i < Contig::getNumContigs(); ++i)
	{
		contigStruct = contigStructList.at(i);
		if (contigId == contigStruct->id)
		{
			float inverseRatio = Contig::totalSize / lineSize;
			int pos = floor((p.x() - contigStruct->xStart) * inverseRatio
					* contigStruct->stretchFactor);
			return pos;
		}
	}

	return -1;
}


/**
 * Creates a pixmap highlighting the rectangular region that is displayed
 * in the Base View.
 *
 * @param rect : The rectangular region that is to be highlighted
 */
void GlobalView::createHighlightingPixmap(const QRect &rect)
{
	highlightingPixmap = QPixmap(this->width(), this->height());
	highlightingPixmap.fill(Qt::transparent);
	QPainter painter(&highlightingPixmap);
	painter.setPen(QColor("red"));
	painter.drawRect(rect);
}


/**
 * Creates a pixmap containing the contig and reads
 */
void GlobalView::createContigPixmap()
{
	lineSize = width() - HORIZONTAL_MARGIN - 20;
	thread.setWidth(width());
	thread.setHeight(height());
	thread.start();
	return;
}


/**
 * Updates the view.
 *
 * Shows the region that is being displayed in the Base View.
 *
 * @param contig : A pointer to the contig for which the view is to be
 * updated.
 */
void GlobalView::updateView(const Contig *contig)
{
	int scaledX1, scaledX2, width, height, y;
	qreal ratio, stretchFactor;
	ContigStruct *contigStruct;

	if (contig == NULL || Contig::getNumContigs() == 0)
	{
		update();
		return;
	}
	else if (contigStructList.size() == 0)
		return;

	ratio = (qreal) lineSize / contig->totalSize;
	contigStruct = NULL;
	foreach (contigStruct, contigStructList)
	{
		if (contig->id == contigStruct->id)
			break;
	}
	assert (contigStruct != NULL);

	stretchFactor = contigStruct->stretchFactor;
	scaledX1 = ((int) ceil(contig->startPos * ratio * stretchFactor));
	scaledX2 = ((int) floor(contig->endPos * ratio * stretchFactor));
	width = scaledX2 - scaledX1;
	scaledX1 += contigStruct->xStart;
	if (width < 1) width = 1;
	height = 30;
	y = contigStruct->yStart - 3;
	QRect rect(scaledX1, y, width, height);
	createHighlightingPixmap(rect);

	QRect sourceRect(0, 0, this->width(), this->height());
	QRect targetRect(0, 0, this->width(), this->height());
	QPainter p(&highlightingPixmap);
	p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
	if (Search::getIsSeqHighlighted() == true)
		p.drawPixmap(targetRect, searchResultsPixmap, sourceRect);
	p.drawPixmap(targetRect, contigPixmap, sourceRect);

	update();
}


/**
 * Returns a pointer to the group box that encloses Global View
 *
 * @return : A pointer to the group box that encloses Global View.
 */
QGroupBox* GlobalView::getGroupBox()
{
	return groupBox;
}


/*
 * Sets maximum value of vertical scrollbar
 */
void GlobalView::setVScrollbarMax(const int lines)
{
	scrollArea->verticalScrollBar()->setMaximum(
			scrollArea->verticalScrollBar()->singleStep() * lines);
}


/**
 * Sets the contig pointer to point to the current contig.
 *
 * @param c : A pointer to the current contig
 */
void GlobalView::setContig(Contig *c)
{
	if (c == NULL)
		return;
	contig = c;
	update();
}


/**
 * Handles resize events
 */
void GlobalView::resizeEvent(QResizeEvent */*event*/)
{
	/* If there are no contigs, return */
	if (Contig::getNumContigs() == 0)
		return;

	createContigPixmap();
	updateView(contig);
}


/**
 * Highlights search results
 *
 * @param map : Pointer to a QMap<int, QMap<int, int>*> that stores
 * contig ID as the key and <startPos, endPos> as value. The startPos and endPos
 * are the start and end positions of each highlighted area.
 */
void GlobalView::showSearchResults(
		QMap<int, QMap<int, int> *> *searchResultsMap)
{
	searchResultsPixmap = QPixmap(this->width(), this->height());
	searchResultsPixmap.fill(Qt::transparent);
	QPainter painter(&searchResultsPixmap);
	painter.setPen(QColor("#8B4513"));

	QMap<int, int> *map = NULL;
	QRect rect;
	int scaledX1, scaledX2, width, height, y, contigId, start;
	qreal ratio, stretchFactor, factor;
	ContigStruct *contigStruct;

	ratio = (qreal) lineSize / Contig::totalSize;
	foreach (contigId, searchResultsMap->keys())
	{
		map = searchResultsMap->value(contigId);
		if (map == NULL)
			continue;

		foreach (contigStruct, contigStructList)
		{
			if (contigId == contigStruct->id)
				break;
		}
		stretchFactor = contigStruct->stretchFactor;
		factor = ratio * stretchFactor;
		foreach (start, map->keys())
		{
			scaledX1 = ((int) ceil(start * factor));
			scaledX2 = ((int) floor(map->value(start) * factor));
			width = scaledX2 - scaledX1;
			scaledX1 += contigStruct->xStart;
			if (width < 1) width = 1;
			height = 20;
			y = contigStruct->yStart + 2;
			rect = QRect(scaledX1, y, width, height);
			painter.drawRect(rect);
		}
		map = NULL;
	}
}

/**
 * Resets highlighting, that is, removes all highlightings
 */
void GlobalView::resetHighlighting()
{
	updateView(contig);
}


/*
 * Displays image containing all the contigs and fragments
 */
void GlobalView::showImage()
{
	contigPixmap = QPixmap::fromImage(thread.getImage());
	QHash<int, int> contigOrderXPosHash = thread.getLabelXPosHash();
	QHash<int, int> contigOrderYPosHash = thread.getLabelYPosHash();
	QPainter painter(&contigPixmap);
	painter.setPen(QColor(Qt::darkGreen));
	foreach (int key, contigOrderXPosHash.keys())
	{
		painter.drawText(
				contigOrderXPosHash.value(key),
				contigOrderYPosHash.value(key),
				QString::number(key));
	}
	updateView(contig);
}


/**
 * Sets the list of contig structures containing data relevant to Global View
 * @param list : Contains list of contig structures that contain data relevant
 * to Global View
 */
void GlobalView::setContigData(const QList<ContigStruct *> &list)
{
	contigStructList = list;
}







