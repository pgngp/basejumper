
#include "parserThread.h"
#include "contig.h"
#include "fragment.h"
#include "math.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "database.h"

#define	BYTE_TO_MBYTE	1048576
#define	MAX_CONTIG_PARTITION	500

extern QQueue<Contig *> contigQueue;
extern QQueue<Fragment *> fragQueue;
extern QMutex contigMutex;
extern QMutex fragMutex;
extern QWaitCondition contigQueueNotFull;
extern QWaitCondition contigQueueNotEmpty;
extern QWaitCondition fragQueueNotFull;
extern QWaitCondition fragQueueNotEmpty;

const int fragQueueSizeMax = 50000;
extern bool moreContigs;

extern QList<quint16> partitionList;
extern int contigPartitions;
extern QMutex partitionListMutex;
extern const int maxPartitionSize;


/**
 * Constructor
 * @return
 */
ParserThread::ParserThread()
{
	isOrderFileLoaded = false;
}


/**
 * Destructor
 * @return
 */
ParserThread::~ParserThread()
{

}


/**
 * Implements the run method
 */
void ParserThread::run()
{
	//qDebug() << "ParserThread begin***";
	char refName[50], fragName[50], complement;
	short readFlag, contigFlag;
	int fragLength, startPos, qualStart, qualEnd, alignStart, alignEnd;
	int refSize, contigIndex, fragIndex, fragIndex2, fragCount, maxYPos;
	int contigNum, fragNum, afNum, fileNum, lineCount, lineSize;
	int numContigs, numFrags, numFragsContig, tmpParsedSize;
	int fragsInCurrentContig, i;
	int totalFragSize, avgFragSize;
	long totalContigSize;
	QString fileName, filesSizeStr, message, orderFile;
	QHash<QByteArray, int> fragNumMappings;
	QByteArray line;
	bool parsedASLine;
	Fragment *frag;
	Contig *contig;
	File *fileObject;
	quint64 parsedSize, totalFileSize;
	//QHash<QByteArray, int> fragNameOccurenceHash;
	QString fileBaseName;
	QList<Fragment *> fragList;
	const char *ASLineFormat, *COLineFormat, *AFLineFormat, *RDLineFormat, *QALineFormat;
	QByteArray emptyByteArray;
	int filesSize;

	/* Initialization */
	contigNum = 0;
	fragNum = 0;
	afNum = 0;
	fileNum = 1;
	isOrderFileLoaded = false;
	lineCount = 0;
	tmpParsedSize = 0;
	parsedSize = Q_UINT64_C(0);
	totalFileSize = Q_UINT64_C(0);
    QFileInfo fileInfo(files.at(0));
    orderFile = fileInfo.absolutePath() + "/order.txt";
    totalContigSize = 0;
    totalFragSize = 0;
    avgFragSize = 0;
    ASLineFormat = "AS %d %d";
    COLineFormat = "CO %s %d %d %*d %*c";
    AFLineFormat = "AF %s %c %d";
    RDLineFormat = "RD %s %d %*d %*d";
    QALineFormat = "QA %d %d %d %d";
    emptyByteArray = "";
    const int parsedSizeInterval = 5000;
    filesSize = files.size();
    moreContigs = true;

	/* Emit signal to indicate total size of files */
	foreach (QString str, files)
		totalFileSize += QFileInfo(str).size();
	if (QFile::exists(orderFile))
		totalFileSize += QFileInfo(orderFile).size();
	emit totalSize((int) ((qreal) totalFileSize / BYTE_TO_MBYTE));

	/* Remove existing rows from DB tables */
	//Database::deleteAll();

	loadedContigsSet.clear();
	Contig::setNumContigs(0);

	/* Emit signal to indicate file parsing */
	emit messageChanged("Parsing contig files...");
	emit cleanWidgets();
	emit parsingStarted();
	//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	filesSizeStr = QString::number(filesSize);
	for (i = 0; i < filesSize; ++i)
	{
		/* File I/O */
		fileName = files.at(i);
		QFile file(fileName);
		QTextStream in(&file);
		in.setIntegerBase(10);
		in.setCodec("UTF-8");
		fileBaseName = QFileInfo(fileName).baseName();

		contigFlag = 0;
		readFlag = 0;
		fragCount = 0;
		numContigs = 0;
		numFrags = 0;
		numFragsContig = 0;
		contigIndex = 0;
		fragIndex = -1;
		fragIndex2 = -1;
		parsedASLine = false;

		/* Emit signal to indicate file parsing */
		message = "Parsing contig file "
			+ QString::number(i+1)
			+ " of " + filesSizeStr + "..."
			+ "\n(File: " + fileBaseName + ")";
		emit messageChanged(message);
		qDebug() << message;
		//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

		/* Try opening the file */
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qCritical() << tr("Cannot read file %1:\n%2.")
					.arg(file.fileName())
					.arg(file.errorString());
			return;
		}

		/* Get each line from the input file */
		while(!in.atEnd())
		{
			line.clear();
			line = in.readLine().toAscii();
			lineSize = line.size();
			parsedSize += lineSize;
			tmpParsedSize += lineSize;

			/* Get counts of contigs and fragments */
			if (!parsedASLine
					&& sscanf(line, ASLineFormat, &numContigs, &numFrags) == 2)
			{
				parsedASLine = true;
			}
			/* Get name and size of each reference contig */
			else if (sscanf(line,
							COLineFormat,
							refName,
							&refSize,
							&numFragsContig) == 3)
			{
				contigNum++;
				contigIndex++;
				fragsInCurrentContig = 0;
				maxYPos = 0;
				fragList.clear();

				contig = new Contig;
				fileObject = new File(fileNum, fileBaseName, fileName);

				/* Store id, name, size, and number of reads for this contig */
				contig->id = contigNum;
				contig->name = refName;
				loadedContigsSet.insert(contig->name);
				contig->size = refSize;
				//contig->seq.clear();
				contig->seq.reserve(contig->size + 1);
				contig->seq = "";
				contig->numberReads = numFragsContig;
				contig->order = contigNum;
				contig->readStartIndex = fragNum + 1;
				contig->readEndIndex = contig->readStartIndex + numFragsContig - 1;
				contig->zoomLevels = (int) log((double) contig->size);
				contig->fileId = fileNum;
				contig->file = fileObject;


//				partitionListMutex.lock();
//				contigPartitions = ceil((float) contig->size / maxPartitionSize);
//				for (int i = 0; i < contigPartitions; ++i)
//					partitionList.append(0);
//				partitionListMutex.unlock();

				/* For now, populate Contig::orderMap with parse order.
				 * This could change if an order file is used later. */
				Contig::orderMap[contig->order] = contig->id;

				/* Read contig sequence */
				while (true)
				{
					line = in.readLine().toAscii();
					if (line.isEmpty()) break;
					contig->seq += line;
					lineSize = line.size();
					parsedSize += lineSize;
					tmpParsedSize += lineSize;
					if (tmpParsedSize >= parsedSizeInterval)
					{
						tmpParsedSize = 0;
						emit parsingProgress(parsedSize / BYTE_TO_MBYTE);
						//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
					}
				}
				contigFlag = 0;
				totalContigSize += contig->size;
			}
			/* Get name, complement status, and mapped position of each read */
			else if (sscanf(line, AFLineFormat, fragName, &complement, &startPos) == 3)
			{
				/* Skip contig sequence in fragment list */
				if (strcmp(refName, fragName) == 0)
				{
					/* Subtract reference from list of fragments for each contig;
					 * Not sure if this is necessary after first contig */
					--numFrags;
					--numFragsContig;
					--contig->numberReads;
					--contig->readEndIndex;
				}
				else
				{
					afNum++;
					fragIndex2++;
					frag = new Fragment();
					fragList.append(frag);
					frag->id = afNum;
					frag->name = fragName;
					frag->complement = complement;
					frag->startPos = startPos;
					frag->contigNumber = contigNum;
					frag->yPos = -1;
					frag->numMappings = 1;
				}
			}
			/* Collect sequence lines for current fragment */
			else if (!readFlag && sscanf(line, RDLineFormat, fragName, &fragLength) == 2)
			{
				if (strcmp(refName, fragName) != 0)
				{
					fragCount++;
					fragNum++;
					fragIndex++;
					fragsInCurrentContig++;
					frag = fragList.at(fragIndex);

					/* This program assumes the fragments are in the same order
					 * in the AF list and the RD list; if not, show an error
					 * message */
					if (strcmp(frag->name, fragName) != 0)
					{
						qCritical() << tr("Reads in AF section are in a different order than in RD section.\n");
						return;
					}

					/* Store fragment length in array */
					frag->size = fragLength;
					totalFragSize += frag->size;
					frag->seq.reserve(frag->size);
					frag->endPos = frag->startPos + frag->size - 1;

					/* Read fragment sequence */
					while (true)
					{
						line = in.readLine().toAscii();
						if (line.isEmpty()) break;
						frag->seq += line;
					}
					parsedSize += frag->size;
					tmpParsedSize += frag->size;

					fragMutex.lock();
					if (fragQueue.size() >= fragQueueSizeMax)
						fragQueueNotFull.wait(&fragMutex);
					fragQueue.enqueue(frag);
					fragQueueNotEmpty.wakeAll();
					fragMutex.unlock();

					/* If all fragments belonging to the current contig
					 * have been parsed */
					if (contig->numberReads == fragsInCurrentContig)
					{
						//qDebug() << "Last leg of parsing contig " << contig->id;
						/* Calculate contig coverage */
						contig->coverage = ((float) totalFragSize) / contig->size;

						/* Calculate average fragment size */
						avgFragSize = (int) ceil(((float) totalFragSize) / contig->numberReads);

						//contigSaverThread.addContig(contig);
						contigMutex.lock();
						contigQueue.append(contig);
						contigQueueNotEmpty.wakeAll();
						contigMutex.unlock();

						fragIndex = -1;
						fragIndex2 = -1;
					}
				}
			}
			/* Get bases that are high-quality and were mapped successfully */
			else if (sscanf(line, QALineFormat, &qualStart, &qualEnd, &alignStart, &alignEnd) == 4)
			{
				frag->qualStart = qualStart;
				frag->qualEnd = qualEnd;
				frag->alignStart = alignStart;
				frag->alignEnd = alignEnd;
			}
			/* When any blank line is detected, stop sequence collection */
			else if (line.isEmpty())
			{

			}

			if (tmpParsedSize >= parsedSizeInterval)
			{
				tmpParsedSize = 0;
				emit parsingProgress(parsedSize / BYTE_TO_MBYTE);
				//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}
		} /* end while */
		file.close();
		fileNum++;
	} /* end for */

	contigMutex.lock();
	moreContigs = false;
	contigQueueNotEmpty.wakeAll();
	contigMutex.unlock();

	fragMutex.lock();
	fragQueue.append(NULL);
	fragQueueNotEmpty.wakeAll();
	fragMutex.unlock();

    Contig::setTotalSize(totalContigSize);

   //emit parsingFinished();

	/* If number of contigs is 0, then display a message and return false */
	if (contigNum == 0)
	{
		qCritical() << tr("No contigs found in selected file(s).");
		return;
	}
	Contig::setNumContigs(contigNum);

	//qDebug() << "ParserThread end***";
}

