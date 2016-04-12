
#include "intermediateViewPainterThread.h"
#include "fragment.h"
#include "math.h"

#define HORIZONTAL_MARGIN	10
#define VERTICAL_MARGIN		20
#define POINT_SIZE			8
#define	TWICE_POINT_SIZE	16
#define	THRICE_POINT_SIZE	24
#define	FONT_FAMILY			"Sans Serif"
#define	PADDING				2
#define	DBL_PADDING			4
#define MAX_DEPTH			20


/**
 * Constructor
 * @return
 */
IntermediateViewPainterThread::IntermediateViewPainterThread()
{
	width = 0;
	height = 0;
	labelPos.rx() = 0;
	labelPos.ry() = 0;
	contig = NULL;
}


/**
 * Destructor
 * @return
 */
IntermediateViewPainterThread::~IntermediateViewPainterThread()
{

}


/**
 * Sets the contig to the given contig
 * @param contig : Pointer to the contig
 */
void IntermediateViewPainterThread::setContig(Contig *contig)
{
	if (contig == NULL)
		return;

	QMutexLocker locker(&mutex);
	this->contig = contig;
}


/**
 * Implements the run method
 */
void IntermediateViewPainterThread::run()
{
	if (contig == NULL)
		return;

	int contigStartX, contigMidX, contigEndX, contigOffsetY;
	int lineSize, fragYPos, numFrags, x1, x2, y1, y2;
	int maxDepth;
	float ratio, maxYPos_logValue, maxYPos_log10Value;
	Fragment *frag;
	QBrush brush;
	enum ScaleType {Linear, LogBaseE, LogBase10};
	ScaleType yPosScale;

	/* Initialization */
	contigStartX = HORIZONTAL_MARGIN;
	contigEndX = width - HORIZONTAL_MARGIN - 20;
	contigMidX = (contigEndX - contigStartX) / 2;
	contigOffsetY = VERTICAL_MARGIN;
	labelPos.rx() = contigMidX;
	labelPos.ry() = contigOffsetY - 2;
	lineSize = width - (2 * HORIZONTAL_MARGIN) - 20;
	ratio = (float) lineSize / contig->size;
	numFrags = contig->fragList->size();
	maxDepth = (int) floor((float) (height - contigOffsetY) / 2) - 2;
	maxYPos_logValue = log(contig->maxFragRows);
	maxYPos_log10Value = log10(contig->maxFragRows);
	image = QImage(width, height, QImage::Format_ARGB32);

	/* Initialize painter */
	QPainter painter(&image);
	painter.fillRect(QRect(0, 0, width, height), QBrush(QColor(Qt::white)));

	/* Draw contig */
	QRect rect(
			QPoint(contigStartX, contigOffsetY),
			QPoint(contigEndX, contigOffsetY+2));
	painter.fillRect(rect, QBrush(QColor(Qt::darkGreen)));
	QRect leftBoundaryRect(contigStartX, contigOffsetY-2, 4, 7);
	painter.fillRect(leftBoundaryRect, QBrush(QColor(Qt::darkGreen)));
	QRect rightBoundaryRect(contigEndX-3, contigOffsetY-2, 4, 7);
	painter.fillRect(rightBoundaryRect, QBrush(QColor(Qt::darkGreen)));

	/* Determine whether fragment y-position should be log-e or log-10 scale */
	if (contig->maxFragRows >= maxDepth
			&& maxYPos_logValue >= maxDepth
			&& maxYPos_log10Value >= maxDepth)
		yPosScale = LogBase10;
	else if (contig->maxFragRows >= maxDepth
			&& maxYPos_logValue >= maxDepth)
		yPosScale = LogBaseE;
	else
		yPosScale = Linear;

	/* Draw fragments */
	for (int i = 0; i < numFrags; ++i)
	{
		frag = contig->fragList->at(i);
		x1 = contigStartX + (frag->startPos * ratio);
		x2 = contigStartX + (frag->endPos * ratio);

		/* Scale y-position */
		if (yPosScale == LogBase10)
			fragYPos = (int) floor(log10(frag->yPos));
		else if (yPosScale == LogBaseE)
			fragYPos = (int) floor(log(frag->yPos));
		else
			fragYPos = frag->yPos;

		/* Initialize vertical position and brush */
		if (fragYPos >= maxDepth)
		{
			y1 = contigOffsetY + (maxDepth * 2) + 8;
			brush = QBrush(Qt::cyan);
		}
		else
		{
			y1 = contigOffsetY + (fragYPos * 2) + 8;
			brush = QBrush(Qt::blue);
		}
		y2 = y1 + 2;

		/* Paint fragment */
		painter.fillRect(QRect(QPoint(x1, y1), QPoint(x2, y2)), brush);
	}

	contigStruct.xStart = contigStartX;
	contigStruct.xEnd = contigEndX;
	emit contigDataGenerated(contigStruct);
}


/**
 * Returns the image on which Intermediate View is painted
 * @return QImage
 */
QImage IntermediateViewPainterThread::getImage()
{
	QMutexLocker locker(&mutex);
	return image;
}


/**
 * Returns the x-position
 * @return QHash<int, int> where contig order is key and x-position is value
 */
QPoint & IntermediateViewPainterThread::getLabelPos()
{
	QMutexLocker locker(&mutex);
	return labelPos;
}

