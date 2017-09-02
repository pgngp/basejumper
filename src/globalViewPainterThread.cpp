
#include "globalViewPainterThread.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "contig.h"
#include "math.h"
#include "database.h"

#define HORIZONTAL_MARGIN			10
#define	VERTICAL_MARGIN				20
#define	CONTIG_VERTICAL_MARGIN		12
#define	HORIZONTAL_LENGTH_MIN		40
#define	HOR_GAP_BETWEEN_CONTIGS		5
#define	VER_GAP_BETWEEN_CONTIGS		40

/**
 * Constructor
 * @return
 */
GlobalViewPainterThread::GlobalViewPainterThread()
{
	width = 0;
	height = 0;
}


/**
 * Destructor
 */
GlobalViewPainterThread::~GlobalViewPainterThread()
{
	foreach (ContigStruct *contigStruct, contigStructList)
		delete contigStruct;
}


/**
 * Returns the image on which Global View is painted
 * @return QImage
 */
QImage GlobalViewPainterThread::getImage()
{
	QMutexLocker locker(&mutex);
	return image;
}


/**
 * Returns a hash containing contig order as key and x-position as value
 * @return QHash<int, int> where contig order is key and x-position is value
 */
QHash<int, int> & GlobalViewPainterThread::getLabelXPosHash()
{
	QMutexLocker locker(&mutex);
	return contigOrderXPosHash;
}


/**
 * Returns a hash containing contig order as key and y-position as value
 * @return QHash<int, int> where contig order is key and y-position is value
 */
QHash<int, int> & GlobalViewPainterThread::getLabelYPosHash()
{
	QMutexLocker locker(&mutex);
	return contigOrderYPosHash;
}


/**
 * Implements the run method
 */
void GlobalViewPainterThread::run()
{
	image = QImage(width, height, QImage::Format_ARGB32);
	QPainter painter(&image);
	painter.fillRect(QRect(0, 0, width, height), QBrush(QColor(Qt::white)));
	QString connectionName = QString(this->metaObject()->className());

	{
		/* Create a new database connection, if it does not already exist */
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getContigDBName());
		QSqlQuery query3(db);
		if (!query3.exec("attach database 'fragDB' as 'fragDB'"))
		{
			qCritical() << "Error attaching DB in "
				<< this->metaObject()->className()
				<< ". Reason: "
				<< query3.lastError().text();
			db.close();
			return;
		}

		/* Fetch contigs from DB */
		QSqlQuery query(db);
		QString str = "select id, size, contigOrder "
				" from contig "
				" order by contigOrder asc";
		if (!query.exec(str))
		{
			qCritical() << "Error fetching contigs from DB. Reason: "
				<< query.lastError().text();
			db.close();
			return;
		}

		int contigId, contigSize, contigOrder, scaledSize;
		int fragStartPos, fragEndPos, fragPosY;
		float ratio, stretchFactor, scaleFactor;
		int lineSize;
		QSqlQuery query2(db);
		QString str2;
		QRect fragRect;
		int contigStartX, contigEndX, contigMidX, contigOffsetY;
		int fragOffsetY, fragStartX, fragEndX;
		int rightMostBoundary, fragWidth;
		ContigStruct *contigStruct;

		contigStartX = HORIZONTAL_MARGIN;
		contigEndX = 0;
		contigOffsetY = VERTICAL_MARGIN;
		lineSize = width - HORIZONTAL_MARGIN - 20;
		ratio = (float) lineSize / Contig::getTotalSize();
		rightMostBoundary = width - 20;
		contigOrderXPosHash.clear();
		contigOrderYPosHash.clear();
		foreach (contigStruct, contigStructList)
			delete contigStruct;
		contigStructList.clear();

		while (query.next())
		{
			contigId = query.value(0).toInt();
			contigSize = query.value(1).toInt();
			contigOrder = query.value(2).toInt();
			scaledSize = floor(contigSize * ratio);
			if (scaledSize < HORIZONTAL_LENGTH_MIN)
			{
				scaledSize = HORIZONTAL_LENGTH_MIN;
				stretchFactor = (float) HORIZONTAL_LENGTH_MIN / scaledSize;
			}
			else
				stretchFactor = 1.0;

			if (contigOrder > 1)
				contigStartX = contigEndX + HOR_GAP_BETWEEN_CONTIGS;
			contigEndX = contigStartX + scaledSize;
			if (contigEndX > rightMostBoundary)
			{
				contigStartX = HORIZONTAL_MARGIN;
				contigEndX = contigStartX + scaledSize;
				contigOffsetY += VER_GAP_BETWEEN_CONTIGS;
			}
			contigMidX = contigStartX + ((contigEndX - contigStartX) / 2);
			QRect contigRect(contigStartX, contigOffsetY, scaledSize, 3);
			painter.fillRect(contigRect, QBrush(QColor(Qt::darkGreen)));
			contigOrderXPosHash.insert(contigOrder, contigMidX);
			contigOrderYPosHash.insert(contigOrder, contigOffsetY-2);

			contigStruct = new ContigStruct;
			contigStruct->id = contigId;
			contigStruct->xStart = contigStartX;
			contigStruct->xEnd = contigEndX;
			contigStruct->yStart = contigOffsetY - CONTIG_VERTICAL_MARGIN;
			contigStruct->yEnd = contigOffsetY + CONTIG_VERTICAL_MARGIN;
			contigStruct->stretchFactor = stretchFactor;
			contigStructList.append(contigStruct);

			/* Draw boundary rectangles */
			QRect leftBoundaryRect(contigStartX, contigOffsetY-2, 4, 7);
			painter.fillRect(leftBoundaryRect, QBrush(QColor(Qt::darkGreen)));
			QRect rightBoundaryRect(contigEndX-4, contigOffsetY-2, 4, 7);
			painter.fillRect(rightBoundaryRect, QBrush(QColor(Qt::darkGreen)));

			/* Fetch fragments from DB */
			str2 = "select startPos, endPos, yPos "
					" from fragment "
					" where contig_id = " + QString::number(contigId);
			if (!query2.exec(str2))
			{
				qCritical() << "Error fetching fragments from DB. Reason: "
					<< query2.lastError().text();
				db.close();
				return;
			}
			fragOffsetY = contigOffsetY + 8;
			scaleFactor = ratio * stretchFactor;
			while (query2.next())
			{
				fragStartPos = query2.value(0).toInt();
				fragEndPos = query2.value(1).toInt();
				fragPosY = query2.value(2).toInt();
				fragStartX = contigStartX + floor(fragStartPos * scaleFactor);
				fragEndX = contigStartX + floor(fragEndPos * scaleFactor);
				fragWidth = fragEndX - fragStartX;
				if (fragWidth == 0) fragWidth = 1;
				fragRect = QRect(fragStartX, fragOffsetY, fragWidth, 5);

				if (fragPosY == 0)
					painter.fillRect(fragRect, QBrush(QColor("#CCCCFF")));
				else if (fragPosY == 1)
					painter.fillRect(fragRect, QBrush(QColor("#AAAAFF")));
				else if (fragPosY == 2)
					painter.fillRect(fragRect, QBrush(QColor("#6666FF")));
				else if (fragPosY == 3)
					painter.fillRect(fragRect, QBrush(QColor("#4D4DFF")));
				else if (fragPosY == 4)
					painter.fillRect(fragRect, QBrush(QColor("#3333FF")));
				else
					painter.fillRect(fragRect, QBrush(QColor("#0000CD")));
			}
		}
		emit contigDataGenerated(contigStructList);
		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
}











