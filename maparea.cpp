#include "maparea.h"
#include "bookmarkdialog.h"
#include <algorithm>
#include "math.h"
#include "time.h"
#include "ctype.h"
#include "limits.h"
#include "assert.h"
#include <stdlib.h>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "file.h"
#include "search.h"
#include "annotationList.h"
#include <iostream>

#define POINT_SIZE			8
#define	PADDING				2
#define	DBL_PADDING			4
#define LINE_HEIGHT 		10
#define TOTAL_LINE_HEIGHT	15
#define TWICE_HEIGHT		20
#define THRICE_HEIGHT		30
#define NUM_BASES			100
#define SCROLLBAR_WIDTH		16
#define	V_SCROLLBAR_MIN		0
#define V_SCROLLBAR_MAX		500
#define	H_SCROLLBAR_MIN		0
#define	POINT_SIZE_MIN		1.00
#define	FIRST_CONTIG_ORDER	1
#define FONT_FAMILY			"Sans Serif"
#define SCROLL_STEP			120
#define	SEARCH_HIGHLIGHT_HT	15


/*
 * Static member definitions
 */

/* Initialize the different color pens */
QPen MapArea::penBlue = QPen(Qt::blue);
QPen MapArea::penGreen = QPen(Qt::darkGreen);
QPen MapArea::penRed = QPen(Qt::red);
QPen MapArea::penMagenta = QPen(Qt::magenta);
QPen MapArea::penBlack = QPen(Qt::black);
QPen MapArea::penGray = QPen(Qt::gray);
QPen MapArea::penDarkYellow = QPen(Qt::darkYellow);
QPen MapArea::penIndigo = QPen(QColor("#8B36D9"));

/* Initialize the font */
QFont MapArea::font = QFont(FONT_FAMILY);

/* Initialize the brush */
QBrush MapArea::brush = QBrush(Qt::yellow);
QBrush MapArea::lightGrayBrush = QBrush(QColor(225, 225, 225));
QBrush MapArea::brushIndigo = QBrush(QColor("#8B36D9"));


/**
 * Constructor
 */
MapArea::MapArea(MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
{
	this->mainWindow = mainWindow;

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setMouseTracking(true);
    setAcceptDrops(true);

    /* Set up the horizontal and vertical scroll bars */
    vScrollBar = new QScrollBar(Qt::Vertical, this);
    vScrollBar->setMinimum(V_SCROLLBAR_MIN);
    connect(vScrollBar, SIGNAL(valueChanged(int)), this, SLOT(update()));
    hScrollBar = new QScrollBar(Qt::Horizontal, this);
    hScrollBar->setMinimum(H_SCROLLBAR_MIN);
    connect(hScrollBar, SIGNAL(actionTriggered(int)),
    		this, SLOT(hScrollbarAction(int)));

    padding = (float) PADDING;
    dblPadding = (float) DBL_PADDING;

    layout = new QVBoxLayout;
    layout->addWidget(this, 0, Qt::AlignTop);
    groupBox = new QGroupBox(tr("Base View"));
    groupBox->setLayout(layout);
    QString tmp = tr("<b>Base View</b> displays a detailed view of a"
    			" single contig along with fragments and annotation"
    			" aligned with the contig.");
    groupBox->setWhatsThis(tmp);
    groupBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    contigList = new ContigList;
    connect(this, SIGNAL(zoomedIn(bool)),
    		contigList, SLOT(setLoadSequenceFlag(bool)),
    		Qt::DirectConnection);

    contig = NULL;
    fragAreaMinX = 0;
    fragAreaMaxX = width();
    fragAreaMinY = 0;
    fragAreaMaxY = height();
    numFragsDisplayed = 0;
    fragOffset = 0;

    initialize();
}


/**
 * Destructor
 */
MapArea::~MapArea()
{
	delete vScrollBar;
	delete hScrollBar;
	delete contigList;
	delete layout;
	//delete groupBox;
	searchResultsMap = NULL;
}


/**
 * Initializes/Resets data members
 */
void MapArea::initialize()
{
	adjustedWidth = width() - (2 * SCROLLBAR_WIDTH);
    //numBases = (int) floor((double) width() / POINT_SIZE);
	numBases = (int) floor((float) adjustedWidth / POINT_SIZE);
    halfNumBases = (int) floor((float) numBases / 2);

    contigStartPos = 0;
    Contig::startPos = contigStartPos;

    midPos = halfNumBases;
    Contig::midPos = midPos;

    contigEndPos = numBases;
    Contig::endPos = contigEndPos;

    rateOfChange = 1;
    pointSize = POINT_SIZE;

    currentContigIndex = 0;
    Contig::setCurrentId(currentContigIndex);

    oldContigIndex = currentContigIndex;
    searchResultsMap = NULL;

    contigList->reset();
}


/**
 * Returns the minimum size the widget can have
 */
QSize MapArea::minimumSizeHint() const
{
	return QSize(800, 200);
}


/**
 * Returns the suggested size that the widget should have
 */
QSize MapArea::sizeHint() const
{
	return QSize(800, 800);
}


/**
 * Changes contig display resolution whenever the zoom slider is moved
 */
void MapArea::zoomSliderMoved(int position)
{
	int tmp;

	/* Initialize */
	rateOfChange = (int) ceil(exp((float)(position - 1)));

 	/* Assign current contig index if it has not been assigned yet */
	if (currentContigIndex == 0)
	{
		currentContigIndex = contigOrderIdHash.value(FIRST_CONTIG_ORDER);
		assert(currentContigIndex != 0);
		Contig::setCurrentId(currentContigIndex);
		contig = contigList->getContigUsingId(currentContigIndex);
		vScrollBar->setMaximum(contig->maxFragRows * 2);
		hScrollBar->setMaximum(contig->size - 1);
	}

	/* Update number of bases to be displayed per window */
	tmp = (int) (floor((float) adjustedWidth / POINT_SIZE)) * rateOfChange;
	if (tmp < 0 || tmp > INT_MAX)
		tmp = INT_MAX;
	numBases = tmp;
	if (numBases > contig->size)
		numBases = contig->size;
	//qDebug() << "contig size = " << contig->size << ", numBases = " << numBases;
    halfNumBases = (int) floor((float) numBases / 2);

	/* Update point size */
    pointSize = ((float) adjustedWidth / numBases);

    /* Update zoom-level flag */
    if (pointSize < POINT_SIZE_MIN)
    	zoomedIn(false);
    else
    	zoomedIn(true);

    goToPos(currentContigIndex, (midPos - halfNumBases));
}


/**
 * Handles keyboard key press events
 */
void MapArea::keyPressEvent(QKeyEvent *event)
{
	switch (event->key())
	{
	/* Right arrow key */
	case Qt::Key_Right:
		hScrollbarAction(QAbstractSlider::SliderPageStepAdd);
		break;
	/* Left arrow key */
	case Qt::Key_Left:
		hScrollbarAction(QAbstractSlider::SliderPageStepSub);
		break;
	/* Up arrow key */
	case Qt::Key_Up:
		vScrollBar->setValue(max(0, vScrollBar->value() - 1));
		break;
	/* Down arrow key */
	case Qt::Key_Down:
		vScrollBar->setValue(min(V_SCROLLBAR_MAX, vScrollBar->value() + 1));
		break;
	default:
		QWidget::keyPressEvent(event);
	}

}


/**
 * This function is invoked every time the widget changes
 */
void MapArea::paintEvent(QPaintEvent */*event*/)
{
	//return;
    int offset, vScrollbarOffset;

    /* If there are no contigs, return */
    if (Contig::getNumContigs() == 0)
    {
    	hScrollBar->hide();
    	vScrollBar->hide();
    	QPainter painter(this);
    	painter.setPen(penGray);
    	painter.drawRect(0, 0, width()-1, height()-1);
    	painter.fillRect(1, 1, width() - 2, height() - 2, Qt::white);
    	return;
    }

    /* Initialize the painter/canvas */
    QPainter painter(this);
    painter.setPen(penGray);
	font.setPointSizeF(pointSize);
	painter.setFont(font);
    painter.drawRect(0, 0, width()-1, height()-1);
    painter.setRenderHint(QPainter::TextAntialiasing, false);

	offset = 0;

	AnnotationList *annotList;
	for (int j = 0; j < contig->annotationLists.size(); ++j)
	{
		annotList = contig->annotationLists.at(j);
		if (annotList->getType() == AnnotationList::Snp)
		{
			offset += THRICE_HEIGHT;
			drawSnps(
					painter,
					annotList->getList(),
					offset,
					annotList->getAlias());
		}
		else if (annotList->getType() == AnnotationList::Gene)
		{
			offset += THRICE_HEIGHT;
			drawGenes(
					painter,
					annotList->getList(),
					offset,
					annotList->getAlias());
			offset += contig->maxGeneRows * LINE_HEIGHT;
		}
		else
		{
			offset += THRICE_HEIGHT;
			drawCustomTrack(
					painter,
					annotList->getList(),
					offset,
					annotList->getAlias());
		}
	}
	annotList = NULL;

	offset += THRICE_HEIGHT;
	vScrollbarOffset = offset;
	drawContig(painter, offset);
	offset += THRICE_HEIGHT;
	drawFragments(painter, offset);

    /* Align scrollbars with the main window */
	hScrollBar->setGeometry(
			1, height() - SCROLLBAR_WIDTH,
			width() - SCROLLBAR_WIDTH, SCROLLBAR_WIDTH);
	vScrollBar->setGeometry(
			width() - SCROLLBAR_WIDTH,
			vScrollbarOffset,
			SCROLLBAR_WIDTH,
			height() - vScrollbarOffset);
	hScrollBar->show();
	vScrollBar->show();

	fragAreaMinY = offset;
	fragAreaMaxY = height() - SCROLLBAR_WIDTH;
	fragAreaMaxX = width() - SCROLLBAR_WIDTH;
}


/*
 * Draws the contig sequence
 */
void MapArea::drawContig(QPainter &painter, const int &yPos)
{
	char ch;
	QString str;
    QPoint point(0, (yPos - 3));
    int occupiedLen;

    /* Draw contig header */
	painter.fillRect(
			1,
			yPos - THRICE_HEIGHT + 1,
			width() - 2,
			THRICE_HEIGHT - 1,
			lightGrayBrush);
	str = "CONTIG: ";
	painter.setPen(penBlack);
	painter.setFont(QFont(FONT_FAMILY, POINT_SIZE));
	painter.drawText(POINT_SIZE, yPos - TWICE_HEIGHT + 1, str);
	occupiedLen = str.length();
	str = QString(contig->name) + ""
			" (Viewing: " + QString("%L2").arg(contigStartPos+1) + ""
			" - " + QString("%L2").arg(contigEndPos+1) + ""
			" of " + QString("%L2").arg(contig->size) + ";"
			" Size: " + QString("%L2").arg(contigEndPos-contigStartPos+1) + ")";
	painter.setPen(penBlue);
	painter.drawText(occupiedLen * POINT_SIZE, yPos - TWICE_HEIGHT + 1, str);

	QPen pen(Qt::black);
	pen.setStyle(Qt::DotLine);
	painter.setPen(pen);
	painter.drawLine(0, yPos, width(), yPos);

	/* If font size is less than a threshold value, then draw a line to
	 * represent the contig */
    if (pointSize < POINT_SIZE_MIN)
    {
    	/* Highlight search results in contig */
        if (Search::getIsSeqHighlighted() == true)
        	highlightSearchResults(painter, yPos);

    	/* Set the x-coordinate */
    	point.rx() += floor(pointSize + DBL_PADDING);

    	QPoint point1(point.x(), (point.y() - 5));
    	QPoint point2(
    			point.x() + floor((contigEndPos - contigStartPos) * pointSize) + pointSize + DBL_PADDING,
    			(point.y() - 2));
    	painter.setPen(penGreen);
    	QRect rect(point1, point2);
    	painter.fillRect(rect, QBrush(QColor(Qt::darkGreen)));

    	QPoint pointHigh(point1.x(), point1.y() + 4);
    	QPoint pointLow(point1.x(), point1.y() - 2);
    	if (contigStartPos <= 0)
    		painter.drawLine(pointHigh, pointLow);

    	pointHigh.rx() = point2.x();
    	pointLow.rx() = point2.x();
    	//qDebug() << "contigEndPos = " << contigEndPos << ", contig size = " << contig->size;
    	if (contigEndPos >= (contig->size - 1))
    		painter.drawLine(pointHigh, pointLow);

    	return;
    }

    assert(contigStartPos >= 0);
    assert(contigEndPos <= contig->size);

	/* Highlight search results in contig */
    if (Search::getIsSeqHighlighted() == true)
    	highlightSearchResults(painter, yPos);

    /* Paint each base of the contig */
    painter.setFont(font);
	for (int k = contigStartPos; k < contigEndPos; ++k)
	{
		/* Set the x-coordinate */
		point.rx() += floor(pointSize + DBL_PADDING);

		/* If this is a SNP, then paint a background */
		if (contig->snpPosHash[k] == 1)
		{
			QRect rect(
					point.x() - padding,
					point.y() - pointSize - padding,
					pointSize + dblPadding,
					pointSize + dblPadding);
			painter.fillRect(rect, brush);
			painter.setPen(penGray);
			painter.drawRect(rect);
		}

		/* Get the base */
		ch = contig->seq.at(k);

		/* Set pen-color depending on the base */
		if (ch == 'A' || ch == 'a')
			painter.setPen(penBlue);
		else if (ch == 'C' || ch == 'c')
			painter.setPen(penGreen);
		else if (ch == 'G' || ch == 'g')
			painter.setPen(penRed);
		else if (ch == 'T' || ch == 't')
			painter.setPen(penMagenta);
		else
			painter.setPen(penBlack);

		/* Paint the base */
		painter.drawText(point, QString(ch));
	}
}


/*
 * Draws the fragment sequences
 */
void MapArea::drawFragments(QPainter &painter, const int &yPos)
{
	Fragment *frag;
	int fragStartPos = 0;
	int fragEndPos = 0;
    char ch;
    int fragsSize, yFrameStart, yFrameEnd, vSliderValue_half, offset;
    QPoint point(0, 0);

    /* Initialization */
    fragsSize = contig->fragList->size();
    vSliderValue_half = (int) floor((float) vScrollBar->value() / 2);
    yFrameStart = vSliderValue_half;
    yFrameEnd = vSliderValue_half + (int) floor((float) height() / TOTAL_LINE_HEIGHT);
    offset = yPos;
    fragOffset = yPos;
    numFragsDisplayed = 0;

    /* Draw header */
	painter.setPen(penBlack);
	painter.setFont(QFont(FONT_FAMILY, POINT_SIZE));
	painter.drawText(POINT_SIZE, (yPos - TWICE_HEIGHT + 2), "READS:");

	/* We need to start from 1 instead of 0 because the first
	 * fragment is a "dummy" object */
	for (int j = 0; j < fragsSize; ++j)
	{
		frag = contig->fragList->at(j);

		/* Skip if the fragment cannot be displayed in this window range */
		if (frag->yPos < yFrameStart
				|| frag->yPos > yFrameEnd
				|| contigStartPos > frag->endPos
				|| contigEndPos < frag->startPos)
			continue;

		numFragsDisplayed++;

		point.rx() = 0;
		point.ry() = (frag->yPos - yFrameStart) * TOTAL_LINE_HEIGHT + fragOffset;


		/* Assign fragment start position */
		if (contigStartPos >= frag->startPos)
			fragStartPos = contigStartPos - frag->startPos + 1;
		else
		{
			fragStartPos = 0;
			point.rx() = ((frag->startPos - contigStartPos) * (pointSize + DBL_PADDING)) - (pointSize + DBL_PADDING);
		}

		/* Assign fragment end position */
		if (contigEndPos < frag->endPos)
			fragEndPos = contigEndPos - frag->startPos;
		else
			fragEndPos = frag->size;

		/* If font size < threshold value, draw line to represent read */
		if (pointSize < POINT_SIZE_MIN)
		{
			/* Start Point */
			QPoint point1(pointSize + DBL_PADDING, point.y());
			if (contigStartPos >= frag->startPos)
				point1.rx() += 0;
			else
				point1.rx() += ((frag->startPos - contigStartPos) * pointSize);

			/* End Point */
			QPoint point2(pointSize + DBL_PADDING, point.y());
			if (contigEndPos >= frag->endPos)
				point2.rx() += ((frag->endPos - contigStartPos) * pointSize);
			else
				point2.rx() += ((contigEndPos - contigStartPos) * pointSize);

			/* Draw line */
			painter.setPen(penBlue);
			painter.drawLine(point1, point2);

			/* Draw ticks */
			QPoint pointHigh(point1.x(), point1.y() + 2);
			QPoint pointLow(point1.x(), point1.y() - 2);
			if (frag->startPos >= contigStartPos && frag->startPos <= contigEndPos)
				painter.drawLine(pointHigh, pointLow);
			pointHigh.rx() = point2.x();
			pointLow.rx() = point2.x();
			if (contigStartPos <= frag->endPos && contigEndPos >= frag->endPos)
				painter.drawLine(pointHigh, pointLow);

			continue;
		}

		/* Paint the fragment name if the beginning of the fragment
		 * lies within the window range. */
		painter.setFont(font);
		if (frag->startPos > contigStartPos
				&& frag->startPos <= contigEndPos)
		{
			QString tmp = QString(frag->name) + " "
					"[" + QString::number(frag->numMappings) + "]";
			int fragNameSize = tmp.size();
			QPoint tmpPoint(point.x() - pointSize - (pointSize * fragNameSize),
					point.y());
			painter.setPen(penBlack);
			painter.drawText(tmpPoint, tmp);
		}

		/* Paint each base of the fragment */
		assert(fragStartPos >= 0);
		assert(fragEndPos <= frag->size);
		for (int k = fragStartPos; k < fragEndPos; ++k)
		{
			point.rx() += pointSize + DBL_PADDING;
			ch = frag->seq.at(k);


			/* If this is a SNP, then draw a colored rectangle around it */
			if (tolower(contig->seq.at(frag->startPos + k - 1)) != tolower(ch)
					&& contig->snpPosHash.value(frag->startPos + k - 1) == 1)
			{
				QRect rect(
						point.x() - padding,
						point.y() - pointSize - padding,
						pointSize + dblPadding,
						pointSize + dblPadding);
				painter.fillRect(rect, brush);
				painter.setPen(penGray);
				painter.drawRect(rect);
			}

			/* Paint the bases */
			if (ch == 'A' || ch == 'a')
				painter.setPen(penBlue);
			else if (ch == 'C' || ch == 'c')
				painter.setPen(penGreen);
			else if (ch == 'G' || ch == 'g')
				painter.setPen(penRed);
			else if (ch == 'T' || ch == 't')
				painter.setPen(penMagenta);
			else
				painter.setPen(penBlack);
			painter.drawText(point, QString(ch));
		}
	}
}


/**
 * Draws genes that belong to the current displayable window
 */
void MapArea::drawGenes(
		QPainter &painter,
		QList<Annotation *> &annotationList,
		int yPos,
		const QString &title)
{
	Gene *annot;
	float start, end, geneStartPos, geneEndPos, factor, y, structureStart, structureEnd;
	QString trackName, geneNamesStr, arrow;
	int occupiedLen, count, listSize;
	QVector<GeneStructure *> *geneStructures;
	GeneStructure *substructure;

	/* Initialization */
	start = (float) contigStartPos;
	end = (float) contigEndPos;
	factor = pointSize + DBL_PADDING;
	y = 0.0;
	trackName = title + ": ";
	occupiedLen = 0;
	count = 0;
	geneNamesStr = "";
	listSize = annotationList.size();

	/* Draw header */
	painter.setFont(QFont(FONT_FAMILY, POINT_SIZE));
	painter.setPen(penBlack);
	painter.drawText(POINT_SIZE, (yPos - TWICE_HEIGHT), trackName);

	QPen pen(Qt::black);
	pen.setStyle(Qt::DotLine);
	painter.setPen(pen);
	painter.drawLine(0,
			yPos + (contig->maxGeneRows * LINE_HEIGHT),
			width(),
			yPos + (contig->maxGeneRows * LINE_HEIGHT));

	occupiedLen = trackName.length();

	painter.setPen(penIndigo);

	for (int i = 0; i < listSize; ++i)
	{
		annot = static_cast<Gene *>(annotationList.at(i));
		geneStartPos = annot->getStartPos();
		geneEndPos = annot->getEndPos();

		/* Filter out structures which cannot be displayed in the
		 * current window */
		if (geneStartPos > end || geneEndPos < start)
			continue;

		/* Append gene names to a string so that the string can be
		 * displayed later on */
		if (count < 10)
		{
			geneNamesStr += annot->getName() + "; ";
			count++;
		}
		else if (count == 10)
		{
			geneNamesStr += "...";
			count++;
		}

		y = annot->yPos * LINE_HEIGHT + (yPos - 10);
		geneStructures = annot->getSubstructures();

		/* If gene structure information is not available */
		if (geneStructures->size() == 0)
		{
			/* If font size < threshold value */
			if (pointSize < POINT_SIZE_MIN)
			{
				QPoint point1(0, y);
				QPoint point2(0, y+1);

				/* Calculate the gene start position */
				if (geneStartPos <= start)
					point1.rx() = factor;
				else
					point1.rx() = ((geneStartPos - start) * pointSize) + factor;

				/* Calculate the gene end position */
				if (geneEndPos < end)
					point2.rx() = ((geneEndPos - start) * pointSize) + factor;
				else
					point2.rx() = ((end - start) * pointSize) + factor;

				painter.drawRect(QRect(point1, point2));
			}
			else
			{
				/* Calculate the gene start position */
				if (geneStartPos <= start)
					geneStartPos = factor;
				else
					geneStartPos = ((geneStartPos - start) * factor);

				/* Calculate the gene end position */
				if (geneEndPos < end)
					geneEndPos = ((geneEndPos - start) * factor);
				else
					geneEndPos = ((end - start) * factor);

				/* Draw gene */
				if (geneEndPos >= geneStartPos)
				{
					QRect rect(QPoint(geneStartPos, y), QPoint(geneEndPos, y+1));
					painter.drawRect(rect);

				}
			}
			continue;
		}

		/* Set direction of transcription */
		if (annot->direction == Gene::Upstream)
			arrow = "<";
		else
			arrow = ">";

		/* If gene structure information is available */
		foreach (substructure, *geneStructures)
		{
			/* If font size < threshold value */
			if (pointSize < POINT_SIZE_MIN)
			{
				painter.setFont(QFont(FONT_FAMILY, 5));

				/* Calculate the gene start position */
				if (substructure->start <= start)
					structureStart = factor;
				else
					structureStart = ((substructure->start - start) * pointSize) + factor;

				/* Calculate the gene end position */
				if (substructure->end < end)
					structureEnd = ((substructure->end - start) * pointSize) + factor;
				else
					structureEnd = ((end - start) * pointSize) + factor;
			}
			else
			{
				painter.setFont(QFont(FONT_FAMILY, POINT_SIZE));

				/* Calculate the gene start position */
				if (substructure->start <= start)
					structureStart = factor;
				else
					structureStart = ((substructure->start - start) * factor);

				/* Calculate the gene end position */
				if (substructure->end < end)
					structureEnd = ((substructure->end - start) * factor);
				else
					structureEnd = ((end - start) * factor);
			}

			/* Draw substructure */
			if (structureEnd >= structureStart)
			{
				if (substructure->type == GeneStructure::EXON)
				{
					QRect rect(
							QPoint(structureStart, y-1),
							QPoint(structureEnd, y+2));
					painter.fillRect(rect, brushIndigo);
				}
				else if (substructure->type == GeneStructure::INTRON)
				{
					do
					{
						painter.drawText(QPoint(structureStart, y+2), arrow);
						structureStart += POINT_SIZE + DBL_PADDING;
					} while (structureStart <= structureEnd);
				}
				else if (substructure->type == GeneStructure::UTR5P)
				{
					painter.drawLine(
							QPoint(structureStart, y),
							QPoint(structureEnd, y));
				}
				else if (substructure->type == GeneStructure::UTR3P)
				{
					painter.drawLine(
							QPoint(structureStart, y),
							QPoint(structureEnd, y));
				}
			}
		}
	}
	annot = NULL;

	/* Display gene names */
	painter.setPen(penBlue);
	painter.setFont(QFont(FONT_FAMILY, POINT_SIZE));
	painter.drawText(
			occupiedLen * POINT_SIZE,
			(yPos - TWICE_HEIGHT),
			geneNamesStr);
}


/**
 * Draws SNPS
 */
void MapArea::drawSnps(
		QPainter &painter,
		QList<Annotation *> &annotationList,
		int yPos,
		const QString &title)
{
	Annotation *annot;
	float start, end, snpStartPos, snpEndPos, factor, y;
	QString trackName;


	/* Initialize */
	start = (float) contigStartPos;
	end = (float) contigEndPos;
	factor = pointSize + DBL_PADDING;
	y = (float) (yPos - 15);
	trackName = title + ": ";

	/* Draw header */
	painter.setFont(QFont(FONT_FAMILY, POINT_SIZE));
	painter.setPen(penBlack);
	painter.drawText(POINT_SIZE, (yPos - TWICE_HEIGHT), trackName);

	QPen pen(Qt::black);
	pen.setStyle(Qt::DotLine);
	painter.setPen(pen);
	painter.drawLine(0, yPos, width(), yPos);
	painter.setPen(penGray);

	foreach (annot, annotationList)
	{
		snpStartPos = (float) annot->getStartPos();
		snpEndPos = (float) annot->getEndPos();

		/* Filter out SNPs which cannot be displayed in the current window */
		if (snpStartPos > contigEndPos || snpEndPos < contigStartPos)
			continue;

		/* Draw SNPs */
		if (snpEndPos >= snpStartPos)
		{

			/* If font size < threshold value, draw line to represent SNPs */
			if (pointSize < POINT_SIZE_MIN)
			{
				QPoint point1(0, y);
				QPoint point2(0, y);

				/* Calculate the gene start position */
				if (snpStartPos <= start)
					point1.rx() = factor;
				else
					point1.rx() = ((snpStartPos - start) * pointSize) + factor;

				/* Calculate the gene end position */
				if (snpEndPos < end)
					point2.rx() = ((snpEndPos - start) * pointSize) + factor;
				else
					point2.rx() = ((end - start) * pointSize) + factor;

				painter.setPen(penDarkYellow);
				painter.drawLine(point1, point2);

				QPoint pointHigh(point1.x(), point1.y() + 2);
				QPoint pointLow(point1.x(), point1.y() - 2);
				if (snpStartPos >= start && snpStartPos <= end)
					painter.drawLine(pointHigh, pointLow);

				pointHigh.rx() = point2.x();
				pointLow.rx() = point2.x();
				if (start <= snpEndPos && end >= snpEndPos)
					painter.drawLine(pointHigh, pointLow);

				continue;
			}

			/* Calculate the SNP start position */
			if (snpStartPos <= start)
				snpStartPos = factor;
			else
				snpStartPos = ((snpStartPos - start) * factor);

			/* Calculate the gene end position */
			if (snpEndPos < end)
				snpEndPos = ((snpEndPos - start) * factor);
			else
				snpEndPos = ((end - start) * factor);

			QRect rect(snpStartPos, y, (snpEndPos - snpStartPos), factor);
			painter.fillRect(rect, brush);
			painter.drawRect(rect);
			for (float i = snpStartPos; i < snpEndPos; i += factor)
				painter.drawLine(QLine(i, y, i, y + factor));
		}
	}
}


/**
 * Draws custom-tracks
 */
void MapArea::drawCustomTrack(
		QPainter &painter,
		QList<Annotation *> &annotationList,
		int yPos,
		const QString &title)
{
	Annotation *annot;
	float start, end, annotStartPos, annotEndPos, factor, y;
	QString trackName;

	/* Initialize */
	start = (float) contigStartPos;
	end = (float) contigEndPos;
	factor = pointSize + DBL_PADDING;
	y = (float) (yPos - 15);
	trackName = title + ": ";
	painter.setPen(penGray);

	/* Draw header */
	painter.setFont(QFont(FONT_FAMILY, POINT_SIZE));
	painter.setPen(penBlack);
	painter.drawText(POINT_SIZE, (yPos - TWICE_HEIGHT), trackName);

	QPen pen(Qt::black);
	pen.setStyle(Qt::DotLine);
	painter.setPen(pen);
	painter.drawLine(0, yPos, width(), yPos);
	painter.setPen(penGray);

	foreach (annot, annotationList)
	{
		annotStartPos = annot->getStartPos();
		annotEndPos = annot->getEndPos();

		/* Filter out SNPs which cannot be displayed in the current window */
		if (annotStartPos > contigEndPos || annotEndPos < contigStartPos)
			continue;

		/* If font size < threshold value, draw line to represent custom tracks */
		if (pointSize < POINT_SIZE_MIN)
		{
			QPoint point1(0, y);
			QPoint point2(0, y);

			/* Calculate the gene start position */
			if (annotStartPos <= start)
				point1.rx() = factor;
			else
				point1.rx() = ((annotStartPos - start) * pointSize) + factor;

			/* Calculate the gene end position */
			if (annotEndPos < end)
				point2.rx() = ((annotEndPos - start) * pointSize) + factor;
			else
				point2.rx() = ((end - start) * pointSize) + factor;

			painter.setPen(penDarkYellow);
			painter.drawLine(point1, point2);

			QPoint pointHigh(point1.x(), point1.y() + 2);
			QPoint pointLow(point1.x(), point1.y() - 2);
			if (annotStartPos >= start && annotStartPos <= end)
				painter.drawLine(pointHigh, pointLow);

			pointHigh.rx() = point2.x();
			pointLow.rx() = point2.x();
			if (start <= annotEndPos && end >= annotEndPos)
				painter.drawLine(pointHigh, pointLow);

			continue;
		}

		/* Calculate the SNP start position */
		if (annotStartPos <= start)
			annotStartPos = factor;
		else
			annotStartPos = ((annotStartPos - start) * factor);

		/* Calculate the gene end position */
		if (annotEndPos < end)
			annotEndPos = ((annotEndPos - start) * factor);
		else
			annotEndPos = ((end - start) * factor);

		/* Draw gene */
		if (annotEndPos >= annotStartPos)
		{
			/* If font size < threshold value, draw line to represent custom tracks */
			if (pointSize < POINT_SIZE_MIN)
			{
				continue;
			}

			QRect rect(annotStartPos, y, (annotEndPos - annotStartPos), factor);
			painter.fillRect(rect, brush);
			painter.drawRect(rect);
			for (float i = annotStartPos; i < annotEndPos; i += factor)
				painter.drawLine(QLine(i, y, i, y + factor));
		}
	}
}


/**
 * Handles the actions performed by the horizontal scroll bar
 *
 * @param action : Action that was performed by the horizontal scrollbar
 */
void MapArea::hScrollbarAction(int action)
{
	int pos;

	/* When the right-arrow button of the scrollbar is clicked */
	if (action == QAbstractSlider::SliderSingleStepAdd)
		pos = contigStartPos + rateOfChange;
	/* When the left-arrow button of the scrollbar is clicked */
	else if (action == QAbstractSlider::SliderSingleStepSub)
		pos = contigStartPos - rateOfChange;
	/* When the right-arrow key of the keyboard is pressed */
	else if (action == QAbstractSlider::SliderPageStepAdd)
		pos = contigStartPos + numBases;
	/* When the left-arrow key of the keyboard is pressed */
	else if (action == QAbstractSlider::SliderPageStepSub)
		pos = contigStartPos - numBases;
	/* When the horizontal scrollbar is manually moved */
	else if (action == QAbstractSlider::SliderMove)
		pos = hScrollBar->sliderPosition();
	goToPos(currentContigIndex, pos);
}


/**
 * Returns a pointer to the horizontal scrollbar
 */
QScrollBar* MapArea::getHScrollBar() const
{
	return hScrollBar;
}


/**
 * Returns a pointer to the vertical scrollbar
 */
QScrollBar* MapArea::getVScrollBar() const
{
	return vScrollBar;
}


/**
 * Sets the maximum value of the vertical scroll bar
 *
 * @param val : Value which the maximum of the vertical
 * scrollbar should be set to
 */
void MapArea::setMaxVScrollbarValue(const int val)
{
	vScrollBar->setMaximum(val);
}


/**
 * Sets the index of the current contig to the given index
 *
 * @param index : Index which the index of the current contig should be set to
 */
void MapArea::setCurrentContigIndex(const int index)
{
	currentContigIndex = index;
	Contig::setCurrentId(currentContigIndex);
}

/**
 * This function takes in a contig ID and position value
 * and moves the Base View to that position.
 *
 * @param id : ID of the contig to which the view should be changed to
 * @param pos : Position within the contig to which the view should be
 * changed to
 */
void MapArea::goToPos(const int id, const int pos)
{
	if (id <= 0)
		return;

	/* Set contig ID */
	currentContigIndex = id;
	Contig::setCurrentId(currentContigIndex);

	/* Set contig start position */
	contigStartPos = pos;
	if (contigStartPos >= contig->size)
		contigStartPos = contig->size - 1;
	else if (contigStartPos < 0)
		contigStartPos = 0;
	Contig::startPos = contigStartPos;

	/* Set contig mid position */
	midPos = contigStartPos + halfNumBases;
	Contig::midPos = midPos;

	/* If the current-contig-index != old-contig-index, then
	 * the contig boundary has been crossed */
	if (currentContigIndex != oldContigIndex)
	{
		contig = contigList->getContigUsingId(currentContigIndex);
		oldContigIndex = currentContigIndex;
//		emit fileChanged(tr("%1[*] - %2")
//				.arg(File::getName(contig->fileId))
//				.arg(tr("Basejumper")));
		if (contig == NULL)
			qDebug() << "contig is null";
		emit fileChanged(tr("%1[*] - %2")
				.arg(contig->file->getName())
				.arg(tr("Basejumper")));
		emit contigChanged(contig);
		vScrollBar->setMaximum(contig->maxFragRows * 2);
		hScrollBar->setMaximum(contig->size - 1);
	}

	/* Update the horizontal scrollbar value/position */
	hScrollBar->setValue(contigStartPos);

	/* Set contig end position */
	contigEndPos = contigStartPos + numBases - 1;
	if (contigEndPos >= contig->size)
		contigEndPos = contig->size - 1;
	Contig::endPos = contigEndPos;

	emit viewChanged(contig);
	update();
}


/**
 * This function takes in a contig ID, position, and string length
 * and moves the view to that position.
 *
 * @param id : ID of the contig to which the view should be changed to
 * @param pos : Position within the contig to which the view should be
 * @param length : Length of the sequence to be highlighted
 */
//void MapArea::goToPos(const int id, const int pos, const int length)
//{
//	Search::setIsSeqHighlighted(true);
//	goToPos(id, pos);
//}


/**
 * This function is invoked when the annotation files are loaded.
 */
void MapArea::annotationLoaded()
{
	contig = contigList->getContigUsingId(currentContigIndex);
	contigChanged(contig);
	goToPos(currentContigIndex, 0);
}


/**
 * Bookmark the selected region
 */
void MapArea::bookmark()
{
	BookmarkDialog bookmarkDialog(this);
	connect(&bookmarkDialog, SIGNAL(bookmarkAdded(const QString &)),
			mainWindow, SLOT(addBookmarkAction(const QString &)));
	bookmarkDialog.exec();
}


/**
 * Loads the given bookmark
 *
 * @param bookmarkName : Name of the bookmark to be loaded
 */
void MapArea::loadBookmark(const QString &bookmarkName)
{
	QString queryStr, filePath, contigName;
	QStringList fileList;
	QSqlQuery query;
	int contigId, pos;

	/* Get bookmark info using the bookmark name */
	queryStr = "select filepath, contigName, startPos "
			" from bookmark "
			" where name = '" + bookmarkName + "'";
	if (!query.exec(queryStr))
	{
		QMessageBox::critical(
			this,
			tr("Basejumper"),
			tr("Error fetching bookmarks from DB.\nReason: "
					+ query.lastError().text().toAscii()));
		return;
	}
	if (query.next())
	{
		filePath = query.value(0).toString();
		contigName = query.value(1).toString();
		pos = query.value(2).toInt();
	}

	/* Check if this contig is already present in the DB */
	queryStr = "select id "
			" from contig "
			" where lower(name) = lower('" + contigName + "')";
	if (!query.exec(queryStr))
	{
		QMessageBox::critical(
			this,
			tr("Basejumper"),
			tr("Error fetching bookmarks from DB.\nReason: "
					+ query.lastError().text().toAscii()));
		return;
	}
	if (query.next())
		contigId = query.value(0).toInt();
	else
		contigId = 0;

	/* If contig is present in DB */
	if (contigId > 0)
		goToPos(contigId, pos);
	/* If contig is not present in DB */
	else
	{
		fileList.append(filePath);
		mainWindow->open(fileList);
		goToPos(1, pos);
	}
}


/**
 * Create a hash containing contig order as key and contig ID
 * as value.
 */
void MapArea::getContigOrderIdHash()
{
	qDebug() << "getContigOrderIdHash";
	contigList->mapOrderAndId();
	QString connectionName = QString(this->metaObject()->className());

	{
		QSqlDatabase db =
			Database::createConnection(
					connectionName,
					Database::getContigDBName());
		QSqlQuery query(db);
		QString str;

		str = "select id, contigOrder "
				" from contig ";
		if (!query.exec(str))
		{
			QMessageBox::critical(
				this,
				tr("Basejumper"),
				tr("Error fetching contig ID and order from DB.\nReason: "
						+ query.lastError().text().toAscii()));
			db.close();
			return;
		}
		contigOrderIdHash.clear();
		while (query.next())
		{
			contigOrderIdHash[query.value(1).toInt()] = query.value(0).toInt();
			qDebug() << "order = " << query.value(1).toInt() << ", id = " << query.value(0).toInt();
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
}


/**
 * Export the selected region to an ACE file
 */
void MapArea::exportSelection()
{
	/*
	- Open an output file to write to
	- store the contig(s) into a contig array
	- store the fragments into a frag array
	- Write contigs and frags to the output file
	- save the output file
	- close the output file
	*/
	int numOfContigs, numOfFrags, contigSize, numOfContigReads;
	QString contigName, contigSeq, fragName;

	/* Initialize */
	numOfContigs = 0;
	numOfFrags = 0;
	contigSize = 0;
	numOfContigReads = 0;
	contigName = "XYZ";
	contigSeq = "ACGTTAC";
	fragName = "frag1";

	QFile file("out.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "AS " << numOfContigs << " " << numOfFrags << endl;
    out << endl;

    out << "CO " << contigName << " " << contigSize << " " << numOfContigReads << endl;
    out << contigSeq << endl;
    out << endl;

    out << "BQ" << endl;
    out << endl;

    file.close();
}


/**
 * Sets SNP threshold value and reload contig
 *
 * @param val : Value to which the threshold should be set
 */
void MapArea::setSnpThreshold(const int val)
{
	snpThreshold = val;
	contigList->setSnpThreshold(val);

	if (contig != NULL)
	{
		contig = contigList->getContigUsingId(contig->id);
		goToPos(contig->id, Contig::startPos);
	}
}


/**
 * Returns SNP threshold value
 *
 * @return The SNP threshold value
 */
int MapArea::getSnpThreshold() const
{
	return snpThreshold;
}


/**
 * Highlights the given rectangle on a paint device
 *
 * @param rect : Rectangle that is to be highlighted
 */
//void MapArea::highlight(const QRect &rect)
//{
//	highlightPixmap = QPixmap(rect.width(), rect.height());
//	highlightPixmap.fill(Qt::transparent);
//	QPainter painter(&highlightPixmap);
//	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
//	painter.fillRect(rect, QBrush(QColor("#DBB399"), Qt::SolidPattern));
//	emit seqHighlighted(true);
//}


/*
 * Highlights search results
 */
void MapArea::highlightSearchResults(QPainter &painter, const int offset)
{
	QMap<int, int> *map;
	int start, end;
	QPoint p1, p2;
	QRect rect;
	float factor;
	QPen pen;

	/* If search results map is null, then return */
	if (searchResultsMap == NULL)
		return;

	map = searchResultsMap->value(contig->id);
	factor = pointSize + DBL_PADDING;

	/* If there are no search results for the current contig, then return */
	if (map == NULL)
		return;

	/* Initialize brush */
	if (pointSize < POINT_SIZE_MIN)
		pen = QPen(QColor("#8B4513"));
	else
		pen = QPen(QColor("#8B4513"));

	/* Highlight each search result */
	foreach (start, map->keys())
	{
		end = map->value(start);

		/* If the range does not lie in the displayable area,
		 * go to the next iteration */
		if (start > contigEndPos || end < contigStartPos)
			continue;

		/* Create the rectangle to be highlighted */
		p1 = QPoint(0, offset - SEARCH_HIGHLIGHT_HT);
		p2 = QPoint(0, offset - 1);

		/* If we are at or near global level */
		if (pointSize < POINT_SIZE_MIN)
		{
			if (start >= contigStartPos && end <= contigEndPos)
			{
				p1.rx() +=  factor + (start - contigStartPos + 1) * pointSize;
				p2.rx() += p1.x() + ceil((end - start + 1) * pointSize);
			}
			else if (start >= contigStartPos && end > contigEndPos)
			{
				p1.rx() += factor + (start - contigStartPos + 1) * pointSize;
				p2.rx() += p1.x() + ceil((contigEndPos - start + 1) * pointSize);
			}
			else if (start < contigStartPos && end <= contigEndPos)
			{
				p1.rx() = -1;
				p2.rx() += factor + ceil((end - contigStartPos + 1) * pointSize);
			}
			else if (start < contigStartPos && end > contigEndPos)
			{
				p1.rx() = -1;
				p2.rx() += factor + ceil((contigEndPos - contigStartPos + 1) * pointSize);
			}
		}
		/* If we are at or near base level */
		else
		{
			if (start > contigStartPos && end <= contigEndPos)
			{
				p1.rx() += (start - contigStartPos + 1) * factor - PADDING;
				p2.rx() += p1.x() + ceil((end - start + 1) * factor) - PADDING;
			}
			else if (start > contigStartPos && end > contigEndPos)
			{
				p1.rx() += (start - contigStartPos + 1) * factor - PADDING;
				p2.rx() += p1.x() + ceil((contigEndPos - start + 1) * factor);
			}
			else if (start <= contigStartPos && end <= contigEndPos)
			{
				if ( start == contigStartPos )
				{
					p1.rx() = pointSize;
					p2.rx() += p1.x() + ceil((end - contigStartPos + 1) * factor);
				} else {
					p1.rx() = -1;
					// the + 2 is to include the last base pair in the rectangle
					p2.rx() += p1.x() + ceil((end - contigStartPos + 2) * factor) - PADDING;
				}
			}
			else if (start <= contigStartPos && end > contigEndPos)
			{
				p1.rx() = start == contigStartPos ? pointSize + DBL_PADDING : -1;
				p2.rx() += p1.x() + ceil((contigEndPos - contigStartPos + 1) * factor);
			}
		}
		rect.setTopLeft(p1);
		rect.setBottomRight(p2);
		painter.setPen(pen);
		painter.drawRect(rect);
	}
}


/**
 * Resets highlighting, that is, all highlightings are removed
 */
void MapArea::resetHighlighting()
{
	emit seqHighlighted(false);
	update();
}


/**
 * Highlights search results
 *
 * @param map : Pointer to a QMap<int, QMap<int, int>*> that stores
 * contig ID as the key and <startPos, endPos> as value. The startPos and endPos
 * are the start and end positions of each highlighted area.
 */
void MapArea::showSearchResults(QMap<int, QMap<int, int> *> *map)
{
	searchResultsMap = map;

	foreach (int contigId, searchResultsMap->keys())
	{
		if (searchResultsMap->value(contigId) == NULL)
			continue;
		//goToPos(contigId, searchResultsMap->value(contigId)->keys().at(0));
		emit viewChanged(contig);
		return;
	}
}


/**
 * Sets the value of the vertical scrollbar
 *
 * @param val : Value which the vertical scrollbar should be set to
 */
void MapArea::setVScrollbarValue(const int val)
{
	vScrollBar->setValue(val);
}


/**
 * Returns a pointer to the group box that encloses this widget
 */
QGroupBox* MapArea::getGroupBox()
{
	return groupBox;
}


/**
 * Handles mouse move event
 */
void MapArea::mouseMoveEvent(QMouseEvent *event)
{
	if (Contig::getNumContigs() == 0
			|| event->x() < fragAreaMinX
			|| event->x() > fragAreaMaxX
			|| event->y() < fragAreaMinY
			|| event->y() > fragAreaMaxY
			|| !QSqlDatabase::database().isOpen())
	{
		this->setCursor(Qt::ArrowCursor);
		return;
	}

	QString displayStr = "";
	QSqlQuery fragQuery;
	int sum = contigStartPos + convertPointToBases(event->pos());
	int yPos = convertYPos(event->pos());
	//qDebug() << "sum = " << sum << ", x() = " << event->x() << ", pointSize = " << pointSize;
	QString str = "select name "
			" from fragment "
			" where contig_id = " + QString::number(currentContigIndex) +
			" and yPos = " + QString::number(yPos) +
			" and startPos <= " + QString::number(sum) +
			" and endPos >= " + QString::number(sum) +
			" order by startPos asc";
	if (!fragQuery.exec(str))
	{
		QMessageBox::critical(
			this,
			tr("Basejumper"),
			tr("Error fetching fragments from DB.\nReason: "
					+ fragQuery.lastError().text().toAscii()));
		return;
	}
	while (fragQuery.next())
		displayStr += fragQuery.value(0).toString() + "; ";

	if (displayStr != "")
	{
		this->setCursor(Qt::PointingHandCursor);
		QToolTip::showText(event->globalPos(),displayStr, this);
	}
	else
		this->setCursor(Qt::ArrowCursor);
}


/**
 * Handles mouse double-click events
 */
void MapArea::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (Contig::getNumContigs() == 0)
		return;

	goToPos(currentContigIndex,
			(contigStartPos + convertPointToBases(event->pos()) - halfNumBases));
}


/*
 * Returns the number of bases that can fit upto the given point
 */
int MapArea::convertPointToBases(const QPoint &p)
{
	if (pointSize > POINT_SIZE_MIN)
		return (int)  ceil((float) p.x() / (pointSize + DBL_PADDING));
	else
	{
		int x = (int) floor((float) (p.x() - pointSize - DBL_PADDING) / pointSize);
		return x;
	}
}


/*
 * Returns the fragment's y-position equivalent of the given point
 */
int MapArea::convertYPos(const QPoint &p)
{
	//int y = ((int) floor((qreal)(p.y() - fragOffset) / TOTAL_LINE_HEIGHT)) + ((int) floor((double) vSliderValue / 2));
	//int y = (int) ceil(((qreal)(p.y() - fragOffset) / TOTAL_LINE_HEIGHT) + ((qreal) vSliderValue / 2));
	int y = (int) ceil(((float)(p.y() - fragOffset) / TOTAL_LINE_HEIGHT) + ((float) vScrollBar->value() / 2));
	return y;
}


/**
 * Handles mouse wheel events
 */
void MapArea::wheelEvent(QWheelEvent *event)
{
	if (Contig::getNumContigs() == 0)
		return;

	int x = event->delta() / SCROLL_STEP;
	emit zoom(x); /* Negate the value of x to reverse scrolling */
}


/**
 * Handles mouse drag events
 */
void MapArea::dragMoveEvent(QDragMoveEvent *event)
{
	//event->accept();
	qDebug() << "drag event";
}


/**
 * Handles mouse drop events
 */
void MapArea::dropEvent(QDropEvent *event)
{
	event->accept();
	qDebug() << "drop event";
}


///*
// * Event handler function for mouse press event
// */
//void MapArea::mousePressEvent(QMouseEvent *event)
//{
////	qDebug() << "mouse press event";
////	mousePressPos = event->pos();
////	this->setCursor(Qt::ClosedHandCursor);
//	if (event->button() == Qt::LeftButton)
//	{
//
//	}
//
//}


///*
// * Event handler function for mouse release event
// */
//void MapArea::mouseReleaseEvent(QMouseEvent *event)
//{
//	if (abs(event->x() - mousePressPos.x()) > 20)
//	{
//		qDebug() << "mouse release event";
//		QPoint p(0, event->y());
//		p.rx() = event->x() - mousePressPos.x();
//		goToPos(currentContigIndex, contigStartPos + convertPointToBases(p));
//	}
//	this->setCursor(Qt::ArrowCursor);
//}


/**
 * Handles widget resize events
 */
void MapArea::resizeEvent(QResizeEvent */*event*/)
{
	if (contig == NULL)
		return;

	adjustedWidth = width() - (2 * SCROLLBAR_WIDTH);

	/* Update number of bases to be displayed per window */
	int tmp = (int) (floor((float) adjustedWidth / POINT_SIZE)) * rateOfChange;

	if (tmp < 0 || tmp > INT_MAX)
		tmp = INT_MAX;
	numBases = tmp;
	if (numBases > contig->size)
		numBases = contig->size;
    halfNumBases = (int) floor((float) numBases / 2);

	/* Update point size */
    pointSize = ((float) adjustedWidth / numBases);

    goToPos(currentContigIndex, contigStartPos);
}










