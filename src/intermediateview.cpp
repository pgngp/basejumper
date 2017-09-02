
#include <QSqlQuery>
#include <QSqlError>
#include "intermediateview.h"
#include "math.h"
#include "search.h"

#define MARGIN				10
#define TOP_MARGIN			20

QPen IntermediateView::penGray = QPen(Qt::gray);


/**
 * Constructor
 *
 * @param parent Pointer to the parent widget
 */
IntermediateView::IntermediateView(QWidget *parent)
	: QWidget(parent)
{
	oldContigOrder = -1;
	oldContigId = -1;
	setMouseTracking(true);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

	layout = new QVBoxLayout;
	layout->addWidget(this, 0, Qt::AlignTop);

	groupBox = new QGroupBox(tr("Contig View"));
	groupBox->setLayout(layout);
	QString tmp = tr("<b>Contig View</b> displays a summarized view of a "
			"single contig.");
	groupBox->setWhatsThis(tmp);
	contig = NULL;
	contigStruct.xStart = 0;
	contigStruct.xEnd = 0;

	connect(&thread, SIGNAL(finished()), this, SLOT(showImage()));
	qRegisterMetaType<IVPainterThreadNS::ContigStruct>("IVPainterThreadNS::ContigStruct");
	connect(&thread,
			SIGNAL(contigDataGenerated(const IVPainterThreadNS::ContigStruct &)),
			this,
			SLOT(setContigData(const IVPainterThreadNS::ContigStruct &)),
			Qt::DirectConnection);
}


/**
 * Destructor
 */
IntermediateView::~IntermediateView()
{
	delete layout;
	//delete groupBox;
	contig = NULL;
}


/**
 * Returns the minimum size that the widget can have
 */
QSize IntermediateView::minimumSizeHint() const
{
    //return QSize(800, 45);
	return QSize(800, 75);
}


/**
 * Returns the recommended size of the widget
 *
 * This function is invoked when the main application window starts up
 * and intermediateview's size is initialized.
 */
QSize IntermediateView::sizeHint() const
{
	//return QSize(800, 45);
	return QSize(800, 75);
}


/**
 * Handles paint events
 *
 * This function is called whenever there is a change to the widget
 *
 * @param event QPaintEvent object
 */
void IntermediateView::paintEvent(QPaintEvent */*event*/)
{
	/* Return if there are no contigs to display */
    if (contig == NULL || contig->order <= 0)
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
 * Handles mouse move events
 */
void IntermediateView::mouseMoveEvent(QMouseEvent *event)
{
	/* If no contigs are loaded, then exit */
    if (contig == NULL)
    	return;

	if (event->x() < contigStruct.xStart
			|| event->x() > contigStruct.xEnd
			|| event->y() < 0
			|| event->y() >= height())
	{
		this->setCursor(Qt::ArrowCursor);
		emit messageChanged("");
		return;
	}

	this->setCursor(Qt::PointingHandCursor);
	emit messageChanged(tr("Click to display that region in the Contig view"));
}


/**
 * Handles mouse press events
 */
void IntermediateView::mousePressEvent(QMouseEvent *event)
{
	/* If no contigs are loaded, then exit */
    if (contig == NULL)
    	return;

	if (contig->order <= 0
			|| event->button() != Qt::LeftButton
			|| event->x() < contigStruct.xStart
			|| event->x() > contigStruct.xEnd)
		return;

	int pos = (int) floor((float)
			(event->x() - contigStruct.xStart) * contig->size / lineSize);
	emit intermediateViewClicked(contig->id, pos);
}


/**
 * Resets member variables
 */
void IntermediateView::reset()
{
	oldContigOrder = -1;
	oldContigId = -1;
	contig = NULL;
	contigStruct.xStart = 0;
	contigStruct.xEnd = 0;
	update();
}


/**
 * Creates a pixmap that displays the highlighted area
 *
 * The highlighted area corresponds to the contig sequence that is currently
 * being displayed in the Base View.
 *
 * @param rect : The rectangle that indicates the region displayed in
 * the Base View
 */
void IntermediateView::createHighlightingPixmap(const QRect &rect)
{
	highlightingPixmap = QPixmap(this->width(), this->height());
	highlightingPixmap.fill(Qt::transparent);
	QPainter painter(&highlightingPixmap);
	painter.setPen(QColor("red"));
	painter.drawRect(rect);
}


/**
 * Creates a pixmap that displays the contig and fragments
 */
void IntermediateView::createContigPixmap()
{
	if (contig == NULL)
		return;
	thread.setWidth(width());
	thread.setHeight(height());
	thread.start();
	thread.wait();

	if (Search::getIsSeqHighlighted() == true)
    	highlightSearchResults();

	return;
}


/**
 * Updates the intermediate view.
 *
 * Displays the given contig and highlights the area of the contig which
 * is currently shown in the Base View.
 *
 * @param c: pointer to the contig object which is to be displayed
 */
void IntermediateView::updateView(const Contig *c)
{
	int scaledX1, scaledX2, width, height, y;
	qreal ratio;

	if (c == NULL)
	{
		update();
		return;
	}

	ratio = (qreal) lineSize / c->size;
	scaledX1 = ((int) ceil(c->startPos * ratio));
	scaledX2 = ((int) floor(c->endPos * ratio));
	width = scaledX2 - scaledX1;
	scaledX1 += MARGIN;
	if (width < 1) width = 1;
	height = 20;
	y = TOP_MARGIN - 15;
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
 * Returns a pointer to the group box that encloses this widget
 */
QGroupBox* IntermediateView::getGroupBox()
{
	return groupBox;
}


/**
 * Sets the contig to the local contig
 *
 * @param c : A pointer to the contig that the local contig
 * should point to.
 */
void IntermediateView::setContig(Contig *c)
{
	if (c == NULL)
		return;
	else if (c->order != oldContigOrder || c->id != oldContigId)
	{
		oldContigOrder = c->order;
		oldContigId = c->id;
		contig = c;
		thread.setContig(contig);
		createContigPixmap();
	}
}


/**
 * Handles resizing events
 */
void IntermediateView::resizeEvent(QResizeEvent */*event*/)
{
	createContigPixmap();
	updateView(contig);
}


/**
 * Highlights search results
 *
 * @param map : Contains contig ID as key and QMap<int, int>
 * as value. The value is basically a map of start and end positions.
 */
void IntermediateView::showSearchResults(
		QMap<int, QMap<int, int> *> *map)
{
	searchResultsMap = map;
	highlightSearchResults();
}


/*
 * Displays search results by highlighting them
 */
void IntermediateView::highlightSearchResults()
{
	searchResultsPixmap = QPixmap(this->width(), this->height());
	searchResultsPixmap.fill(Qt::transparent);
	QPainter painter(&searchResultsPixmap);
	painter.setPen(QColor("#8B4513"));

	QMap<int, int> *map = NULL;
	QRect rect;
	int scaledX1, scaledX2, width, height, y, start;
	qreal ratio;

	map = searchResultsMap->value(contig->id);
	if (map == NULL)
		return;

	ratio = (qreal) lineSize / contig->size;
	foreach (start, map->keys())
	{
		scaledX1 = ((int) ceil(start * ratio));
		scaledX2 = ((int) floor(map->value(start) * ratio));
		width = scaledX2 - scaledX1;
		scaledX1 += MARGIN;
		if (width < 1) width = 1;
		height = 20;
		y = TOP_MARGIN - 15;
		rect = QRect(scaledX1, y, width, height);
		painter.drawRect(rect);
	}
}


/**
 * Removes highlighting of search results
 */
void IntermediateView::resetHighlighting()
{
	updateView(contig);
}


/*
 * Displays image containing all the contigs and fragments
 */
void IntermediateView::showImage()
{
	contigPixmap = QPixmap::fromImage(thread.getImage());
	QPoint labelPos = thread.getLabelPos();
	QPainter painter(&contigPixmap);
	painter.setPen(QColor(Qt::darkGreen));
	painter.drawText(
			labelPos.x(),
			labelPos.y(),
			QString::number(contig->order));
	updateView(contig);
}


/*
 * Sets the contig structure data
 */
void IntermediateView::setContigData(
		const IVPainterThreadNS::ContigStruct &contigStruct)
{
	this->contigStruct = contigStruct;
	lineSize = contigStruct.xEnd - contigStruct.xStart;
}

