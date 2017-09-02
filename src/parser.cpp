
#include "parser.h"
#include <QtGui>
#include "math.h"
#include <QtAlgorithms>
#include "assert.h"
#include <algorithm>
#include "limits.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVarLengthArray>
#include <exception>
#include <QFileInfo>
#include "database.h"
#include "geneStructure.h"
#include "parserThread.h"

#define	BYTE_TO_MBYTE	1048576
#define	MAX_CONTIG_PARTITION	500
#define	FRAG_DESC_GAP	20

using namespace std;

/* Global variables */
QWaitCondition contigSaved;
QWaitCondition snpSaved;
QWaitCondition fragSaved;
QWaitCondition dbAvailable;
QWaitCondition fragQueueEmpty;
QMutex mutex;
QMutex destroyerMutex;
QReadWriteLock readWriteLock;
int numContigsSaved = 0;
int numContigsFragsSaved = 0;
int numContigsSnpsSaved = 0;
int numContigsDestroyed = 0;
bool volatile isDbAvailable = true;

QQueue<Contig *> contigQueue;
QQueue<Fragment *> fragQueue;
QMutex contigMutex;
QMutex fragMutex;
QWaitCondition contigQueueNotFull;
QWaitCondition contigQueueNotEmpty;
QWaitCondition fragQueueNotFull;
QWaitCondition fragQueueNotEmpty;

const int fragQueueSizeMax = 50000;
bool moreContigs = true;

QList<quint16> partitionList;
int contigPartitions;
const int maxPartitionSize = 100;
QMutex partitionListMutex;




/**
 * Constructor
 */
Parser::Parser()
{
	isOrderFileLoaded = false;

	connect(&parserThread, SIGNAL(totalSize(int)),
			this, SLOT(totalSizeSignaled(int)));
	connect(&parserThread, SIGNAL(messageChanged(const QString &)),
			this, SLOT(messageChangeSignaled(const QString &)));
	connect(&parserThread, SIGNAL(cleanWidgets()),
			this, SLOT(cleanWidgetsSignaled()));
	connect(&parserThread, SIGNAL(parsingStarted()),
			this, SLOT(parsingStartedSignaled()));
	connect(&parserThread, SIGNAL(parsingProgress(int)),
			this, SLOT(parsingProgressSignaled(int)));
	connect(&parserThread, SIGNAL(finished()),
			this, SLOT(signalParsingFinished()));
	connect(&contigSaverThread, SIGNAL(finished()),
			this, SLOT(signalParsingFinished()));
	connect(&fragSaverThread, SIGNAL(finished()),
			this, SLOT(signalParsingFinished()));
}


/**
 * Destructor
 */
Parser::~Parser()
{

}


bool Parser::readAce(const QStringList &files, int filesSize)
{
	parserThread.setFileList(files);
	parserThread.start();
	contigSaverThread.start();
	fragSaverThread.start();

	return true;
}


void Parser::totalSizeSignaled(int size)
{
	emit totalSize(size);
}


void Parser::messageChangeSignaled(const QString &message)
{
	emit messageChanged(message);
}


void Parser::cleanWidgetsSignaled()
{
	emit cleanWidgets();
}


void Parser::parsingStartedSignaled()
{
	emit parsingStarted();
}


void Parser::parsingProgressSignaled(int parsedSize)
{
	emit parsingProgress(parsedSize);
}


void Parser::signalParsingFinished()
{
	if (parserThread.isFinished()
			&& contigSaverThread.isFinished()
			&& fragSaverThread.isFinished())
		emit parsingFinished();
}


/**
 * Reads the contig and fragment sequences from the given files in ACE format.
 *
 * @param files : Reference to the list of files to be parsed
 * @param filesSize : Number of files to be parsed
 *
 * @return Returns true on success and false on failure
 */
//bool Parser::readAce(const QStringList &files, int filesSize)
//{
//	QTime startTime, endTime;
//	startTime = QTime::currentTime();
//	char refName[50], fragName[50], complement;
//	short readFlag, contigFlag;
//	int fragLength, startPos, qualStart, qualEnd, alignStart, alignEnd;
//	int refSize, contigIndex, fragIndex, fragIndex2, fragCount, maxYPos;
//	int contigNum, fragNum, afNum, fileNum, lineCount, lineSize;
//	int numContigs, numFrags, numFragsContig, tmpParsedSize, maxPartitionSize;
//	int fragsInCurrentContig, i;
//	int totalFragSize, avgFragSize;
//	long totalContigSize;
//	QString fileName, filesSizeStr, message, orderFile;
//	QHash<QByteArray, int> fragNumMappings;
//	QByteArray line;
//	bool parsedASLine;
//	Fragment *frag;
//	Contig *contig;
//	File *fileObject;
//	quint64 parsedSize, totalFileSize;
//	//QHash<QByteArray, int> fragNameOccurenceHash;
//	QString fileBaseName;
//	QList<Fragment *> fragList;
//	const char *ASLineFormat, *COLineFormat, *AFLineFormat, *RDLineFormat, *QALineFormat;
//	QByteArray emptyByteArray;
//
//	/* Initialization */
//	contigNum = 0;
//	fragNum = 0;
//	afNum = 0;
//	fileNum = 1;
//	isOrderFileLoaded = false;
//	lineCount = 0;
//	tmpParsedSize = 0;
//	parsedSize = Q_UINT64_C(0);
//	totalFileSize = Q_UINT64_C(0);
//    QFileInfo fileInfo(files.at(0));
//    orderFile = fileInfo.absolutePath() + "/order.txt";
//    maxPartitionSize = MAX_CONTIG_PARTITION;
//    totalContigSize = 0;
//    totalFragSize = 0;
//    avgFragSize = 0;
//    numContigsSaved = 0;
//    numContigsFragsSaved = 0;
//    numContigsSnpsSaved = 0;
//    numContigsDestroyed = 0;
//    isDbAvailable = true;
//    ASLineFormat = "AS %d %d";
//    COLineFormat = "CO %s %d %d %*d %*c";
//    AFLineFormat = "AF %s %c %d";
//    RDLineFormat = "RD %s %d %*d %*d";
//    QALineFormat = "QA %d %d %d %d";
//    emptyByteArray = "";
//    const int fragQueueSizeMax = 50000;
//    const int parsedSizeInterval = 5000;
//
//
//	/* Emit signal to indicate total size of files */
//	foreach (QString str, files)
//		totalFileSize += QFileInfo(str).size();
//	if (QFile::exists(orderFile))
//		totalFileSize += QFileInfo(orderFile).size();
//	emit totalSize((int) ((qreal) totalFileSize / BYTE_TO_MBYTE));
//
//	/* Remove existing rows from DB tables */
//	//Database::deleteAll();
//
//	loadedContigsSet.clear();
//	Contig::setNumContigs(0);
//
//	FragmentSaverThread fragSaverThread;
//	ContigSaverThread contigSaverThread;
//	ContigDestroyerThread contigDestroyerThread;
//	contigSaverThread.start();
//	fragSaverThread.start();
//	contigDestroyerThread.start();
//
//
//	/* Emit signal to indicate file parsing */
//	emit messageChanged("Parsing contig files...");
//	emit cleanWidgets();
//	emit parsingStarted();
//	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//
//	filesSizeStr = QString::number(filesSize);
//	for (i = 0; i < filesSize; ++i)
//	{
//		/* File I/O */
//		fileName = files.at(i);
//		QFile file(fileName);
//		QTextStream in(&file);
//		in.setIntegerBase(10);
//		in.setCodec("UTF-8");
//		fileBaseName = QFileInfo(fileName).baseName();
//
//		contigFlag = 0;
//		readFlag = 0;
//		fragCount = 0;
//		numContigs = 0;
//		numFrags = 0;
//		numFragsContig = 0;
//		contigIndex = 0;
//		fragIndex = -1;
//		fragIndex2 = -1;
//		parsedASLine = false;
//
//		/* Emit signal to indicate file parsing */
//		message = "Parsing contig file "
//			+ QString::number(i+1)
//			+ " of " + filesSizeStr + "..."
//			+ "\n(File: " + fileBaseName + ")";
//		emit messageChanged(message);
//		//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//
//		/* Try opening the file */
//		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
//		{
//			QMessageBox::warning(
//					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
//					tr("Basejumper"),
//					tr("Cannot read file %1:\n%2.")
//					.arg(file.fileName())
//					.arg(file.errorString()));
//			return false;
//		}
//
//		/* Get each line from the input file */
//		while(!in.atEnd())
//		{
//			line.clear();
//			line = in.readLine().toAscii();
//			lineSize = line.size();
//			parsedSize += lineSize;
//			tmpParsedSize += lineSize;
//
//			/* Get counts of contigs and fragments */
//			if (!parsedASLine
//					&& sscanf(line, ASLineFormat, &numContigs, &numFrags) == 2)
//			{
//				parsedASLine = true;
//			}
//			/* Get name and size of each reference contig */
//			else if (sscanf(line,
//							COLineFormat,
//							refName,
//							&refSize,
//							&numFragsContig) == 3)
//			{
//				contigNum++;
//				contigIndex++;
//				fragsInCurrentContig = 0;
//				maxYPos = 0;
//				fragList.clear();
//
//				contig = new Contig;
//				fileObject = new File(fileNum, fileBaseName, fileName);
//
//				/* Store id, name, size, and number of reads for this contig */
//				contig->id = contigNum;
//				contig->name = refName;
//				loadedContigsSet.insert(contig->name);
//				contig->size = refSize;
//				//contig->seq.clear();
//				contig->seq.reserve(contig->size + 1);
//				contig->seq = "";
//				contig->numberReads = numFragsContig;
//				contig->order = contigNum;
//				contig->readStartIndex = fragNum + 1;
//				contig->readEndIndex = contig->readStartIndex + numFragsContig - 1;
//				contig->zoomLevels = (int) log((double) contig->size);
//				contig->fileId = fileNum;
//				contig->file = fileObject;
//
//				/* For now, populate Contig::orderMap with parse order.
//				 * This could change if an order file is used later. */
//				Contig::orderMap[contig->order] = contig->id;
//
//				/* Read contig sequence */
//				while (true)
//				{
//					line = in.readLine().toAscii();
//					if (line.isEmpty()) break;
//					contig->seq += line;
//					lineSize = line.size();
//					parsedSize += lineSize;
//					tmpParsedSize += lineSize;
//					if (tmpParsedSize >= parsedSizeInterval)
//					{
//						tmpParsedSize = 0;
//						emit parsingProgress(parsedSize / BYTE_TO_MBYTE);
//						//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//					}
//				}
//				contigFlag = 0;
//				totalContigSize += contig->size;
//			}
//			/* Get name, complement status, and mapped position of each read */
//			else if (sscanf(line, AFLineFormat, fragName, &complement, &startPos) == 3)
//			{
//				/* Skip contig sequence in fragment list */
//				if (strcmp(refName, fragName) == 0)
//				{
//					/* Subtract reference from list of fragments for each contig;
//					 * Not sure if this is necessary after first contig */
//					--numFrags;
//					--numFragsContig;
//					--contig->numberReads;
//					--contig->readEndIndex;
//				}
//				else
//				{
//					afNum++;
//					fragIndex2++;
//					frag = new Fragment();
//					//contig->fragList->append(frag);
//					fragList.append(frag);
//					frag->id = afNum;
//					frag->name = fragName;
//					frag->complement = complement;
//					frag->startPos = startPos;
//					frag->contigNumber = contigNum;
//					frag->yPos = -1;
//					frag->numMappings = 1;
//					//if (fragNameOccurenceHash.contains(frag->name))
//					//	fragNameOccurenceHash[frag->name]++;
//					//else
//					//	fragNameOccurenceHash.insert(frag->name, 1);
//				}
//			}
//			/* Collect sequence lines for current fragment */
//			else if (!readFlag && sscanf(line, RDLineFormat, fragName, &fragLength) == 2)
//			{
//				if (strcmp(refName, fragName) != 0)
//				{
//					fragCount++;
//					fragNum++;
//					fragIndex++;
//					fragsInCurrentContig++;
//					//frag = contig->fragList->at(fragIndex);
//					frag = fragList.at(fragIndex);
//
//					/* This program assumes the fragments are in the same order
//					 * in the AF list and the RD list; if not, show an error
//					 * message */
//					if (strcmp(frag->name, fragName) != 0)
//					{
//						QMessageBox::warning((
//								reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
//								tr("Basejumper"),
//								tr("Reads in AF section are in a different order than in RD section.\n"));
//						return false;
//					}
//
//					/* Store fragment length in array */
//					frag->size = fragLength;
//					totalFragSize += frag->size;
//					frag->seq.reserve(frag->size);
//					frag->endPos = frag->startPos + frag->size - 1;
//
//					/* Read fragment sequence */
//					while (true)
//					{
//						line = in.readLine().toAscii();
//						if (line.isEmpty()) break;
//						frag->seq += line;
//					}
//					parsedSize += frag->size;
//					tmpParsedSize += frag->size;
//					//if (fragSaverThread.getQueueSize() > fragQueueSizeMax)
//					//	fragSaverThread.wait();
//					//qDebug() << "waiting begin...";
//					while (fragSaverThread.getQueueSize() > fragQueueSizeMax)
//					{
//						if (!fragSaverThread.isRunning())
//						{
//							qDebug() << "fragSaverThread is not running";
//							fragSaverThread.start();
//						}
//						fragSaverThread.wait(2000);
//					}
//					//qDebug() << "waiting end...";
//					fragSaverThread.addFrag(frag);
//					if (tmpParsedSize >= parsedSizeInterval)
//					{
//						tmpParsedSize = 0;
//						emit parsingProgress(parsedSize / BYTE_TO_MBYTE);
//						//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//					}
//
//					/* If all fragments belonging to the current contig
//					 * have been parsed */
//					if (contig->numberReads == fragsInCurrentContig)
//					{
//						qDebug() << "Last leg of parsing contig " << contig->id;
//						/* Calculate contig coverage */
//						contig->coverage = ((float) totalFragSize) / contig->size;
//
//						/* Calculate average fragment size */
//						avgFragSize = (int) ceil(((float) totalFragSize) / contig->numberReads);
//
//						contigSaverThread.addContig(contig);
//						fragIndex = -1;
//						fragIndex2 = -1;
//					}
//				}
//			}
//			/* Get bases that are high-quality and were mapped successfully */
//			else if (sscanf(line, QALineFormat, &qualStart, &qualEnd, &alignStart, &alignEnd) == 4)
//			{
//				frag->qualStart = qualStart;
//				frag->qualEnd = qualEnd;
//				frag->alignStart = alignStart;
//				frag->alignEnd = alignEnd;
//			}
//			/* When any blank line is detected, stop sequence collection */
//			else if (line.isEmpty())
//			{
//
//			}
//
//			if (tmpParsedSize >= parsedSizeInterval)
//			{
//				tmpParsedSize = 0;
//				emit parsingProgress(parsedSize / BYTE_TO_MBYTE);
//				//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//			}
//		} /* end while */
//
//		file.close();
//		fileNum++;
//	} /* end for */
//
//    Contig::setTotalSize(totalContigSize);
//
//	qDebug() << "Waiting for threads to finish...begin";
//	fragSaverThread.wait();
//	contigSaverThread.wait();
//	//contigDestroyerThread.wait();
//	qDebug() << "Waiting for threads to finish...end";
//
//	//qDebug() << "Update frag mapping...Begin";
//	//if (!updateFragMapping(fragNameOccurenceHash))
//	//	return false;
//	//qDebug() << "Update frag mapping...End";
//
//   //emit parsingFinished();
//
//	/* If number of contigs is 0, then display a message and return false */
//	if (contigNum == 0)
//	{
//		QMessageBox::warning(
//				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
//				tr("Basejumper"),
//				tr("No contigs found in selected file(s)."));
//		return false;
//	}
//	Contig::setNumContigs(contigNum);
//
//    /* Parse 'order.txt' file, if it exists */
//    if (QFile::exists(orderFile))
//    {
//    	isOrderFileLoaded = readOrderFile(orderFile);
//    	emit orderParsingFinished();
//    }
//
//	return true;
//}


/**
 * Reads the contig and fragment sequences from the given files in ACE format.
 *
 * @param files : Reference to the list of files to be parsed
 * @param filesSize : Number of files to be parsed
 *
 * @return Returns true on success and false on failure
 */
//bool Parser::readAce(const QStringList &files, int filesSize)
//{
//	QTime startTime, endTime;
//	startTime = QTime::currentTime();
//	char refName[50], fragName[50], complement;
//	short readFlag, contigFlag;
//	int fragLength, startPos, qualStart, qualEnd, alignStart, alignEnd;
//	int refSize, contigIndex, fragIndex, fragIndex2, fragCount, maxYPos;
//	int contigNum, fragNum, afNum, fileNum, lineCount, lineSize;
//	int numContigs, numFrags, numFragsContig, tmpParsedSize, maxPartitionSize;
//	int numPartitions, binNum1, binNum2, fragsInCurrentContig, i, j;
//	int totalFragSize, avgFragSize;
//	long totalContigSize;
//	QString fileName, filesSizeStr, message, orderFile;
//	QHash<QByteArray, int> fragNumMappings;
//	QByteArray line;
//	bool parsedASLine;
//	Fragment *frag;
//	Contig *contig;
//	quint64 parsedSize, totalFileSize;
//	QList <QList <Fragment *>*> bin;
//	QList<Fragment *> *subBin;
//	QHash<QByteArray, int> fragNameOccurenceHash;
//
//	/* Initialization */
//	contigNum = 0;
//	fragNum = 0;
//	afNum = 0;
//	fileNum = 1;
//	isOrderFileLoaded = false;
//	lineCount = 0;
//	tmpParsedSize = 0;
//	parsedSize = Q_UINT64_C(0);
//	totalFileSize = Q_UINT64_C(0);
//    QFileInfo fileInfo(files.at(0));
//    orderFile = fileInfo.absolutePath() + "/order.txt";
//    maxPartitionSize = MAX_CONTIG_PARTITION;
//    totalContigSize = 0;
//    totalFragSize = 0;
//    avgFragSize = 0;
//
//	/* Emit signal to indicate total size of files */
//	foreach (QString str, files)
//		totalFileSize += QFileInfo(str).size();
//	if (QFile::exists(orderFile))
//		totalFileSize += QFileInfo(orderFile).size();
//	emit totalSize((int) ceil((qreal) totalFileSize / BYTE_TO_MBYTE));
//
//	/* Remove existing rows from DB tables */
//	//Database::deleteAll();
//
//	loadedContigsSet.clear();
//	Contig::setNumContigs(0);
//
//	/* Emit signal to indicate file parsing */
//	emit messageChanged("Parsing contig files...");
//	emit cleanWidgets();
//	emit parsingStarted();
//	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//
//	filesSizeStr = QString::number(filesSize);
//	for (i = 0; i < filesSize; ++i)
//	{
//		/* File I/O */
//		fileName = files.at(i);
//		QFile file(fileName);
//		QTextStream in(&file);
//		in.setIntegerBase(10);
//		in.setCodec("UTF-8");
//
//		contigFlag = 0;
//		readFlag = 0;
//		fragCount = 0;
//		numContigs = 0;
//		numFrags = 0;
//		numFragsContig = 0;
//		contigIndex = 0;
//		fragIndex = -1;
//		fragIndex2 = -1;
//		parsedASLine = false;
//
//		/* Emit signal to indicate file parsing */
//		message = "Parsing contig file "
//			+ QString::number(i+1)
//			+ " of " + filesSizeStr + "..."
//			+ "\n(File: " + QFileInfo(fileName).baseName() + ")";
//		emit messageChanged(message);
//		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//
//		/* Try opening the file */
//		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
//		{
//			QMessageBox::warning(
//					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
//					tr("Basejumper"),
//					tr("Cannot read file %1:\n%2.")
//					.arg(file.fileName())
//					.arg(file.errorString()));
//			return false;
//		}
//
//		/* Get each line from the input file */
//		while(!in.atEnd())
//		{
//			line = in.readLine().toAscii();
//			lineSize = line.size();
//			parsedSize += lineSize;
//			tmpParsedSize += lineSize;
//
//			/* Get counts of contigs and fragments */
//			if (!parsedASLine && sscanf(line, "AS %d %d", &numContigs, &numFrags) == 2)
//			{
//				parsedASLine = true;
//			}
//			/* Get name and size of each reference contig */
//			else if (sscanf(line,
//							"CO %s %d %d %*d %*c",
//							refName,
//							&refSize,
//							&numFragsContig) == 3)
//			{
//				contigNum++;
//				contigIndex++;
//				fragsInCurrentContig = 0;
//				maxYPos = 0;
//
//				contig = new Contig;
//
//				/* Store id, name, size, and number of reads for this contig */
//				contig->id = contigNum;
//				contig->name = refName;
//				loadedContigsSet.insert(contig->name);
//				contig->size = refSize;
//				contig->seq.clear();
//				contig->seq.resize(contig->size + 1);
//				contig->seq = "";
//				contig->numberReads = numFragsContig;
//				contig->order = contigNum;
//				contig->readStartIndex = fragNum + 1;
//				contig->readEndIndex = contig->readStartIndex + numFragsContig - 1;
//				contig->zoomLevels = (int) log((double) contig->size);
//				contig->fileId = fileNum;
//
//				// For now, populate Contig::orderMap with parse order.
//				// This could change if an order file is used later.
//				Contig::orderMap[contig->order] = contig->id;
//
//				numPartitions = (int) ceil((qreal) contig->size / maxPartitionSize);
//				for (int j = 0; j < numPartitions; ++j)
//					bin.append(new QList<Fragment *>);
//
//				/* Read contig sequence */
//				while (true)
//				{
//					line = in.readLine().toAscii();
//					if (line.isEmpty()) break;
//					contig->seq += line;
//					parsedSize += line.size();
//					tmpParsedSize += line.size();
//					if (tmpParsedSize >= 5000)
//					{
//						tmpParsedSize = 0;
//						emit parsingProgress((int) ceil((qreal) parsedSize / BYTE_TO_MBYTE));
//						QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//					}
//				}
//				contigFlag = 0;
//				totalContigSize += contig->size;
//			}
//			/* Get name, complement status, and mapped position of each read */
//			else if (sscanf(line, "AF %s %c %d", fragName, &complement, &startPos) == 3)
//			{
//				/* Skip contig sequence in fragment list */
//				if (strcmp(refName, fragName) == 0)
//				{
//					/* Subtract reference from list of fragments for each contig;
//					 * Not sure if this is necessary after first contig */
//					--numFrags;
//					--numFragsContig;
//					--contig->numberReads;
//					--contig->readEndIndex;
//				}
//				else
//				{
//					afNum++;
//					fragIndex2++;
//					frag = new Fragment();
//					contig->fragList->append(frag);
//					frag->id = afNum;
//					frag->name = fragName;
//					frag->complement = complement;
//					frag->startPos = startPos;
//					frag->contigNumber = contigNum;
//					frag->yPos = -1;
//					frag->numMappings = 1;
//					if (fragNameOccurenceHash.contains(frag->name))
//						fragNameOccurenceHash[frag->name]++;
//					else
//						fragNameOccurenceHash.insert(frag->name, 1);
//				}
//			}
//			/* Collect sequence lines for current fragment */
//			else if (!readFlag && sscanf(line, "RD %s %d %*d %*d", fragName, &fragLength) == 2)
//			{
//				if (strcmp(refName, fragName) != 0)
//				{
//					fragCount++;
//					fragNum++;
//					fragIndex++;
//					fragsInCurrentContig++;
//					frag = contig->fragList->at(fragIndex);
//
//					/* This program assumes the fragments are in the same order
//					 * in the AF list and the RD list; if not, show an error
//					 * message */
//					if (strcmp(frag->name, fragName) != 0)
//					{
//						QMessageBox::warning((
//								reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
//								tr("Basejumper"),
//								tr("Reads in AF section are in a different order than in RD section.\n"));
//						return false;
//					}
//
//					/* Store fragment length in array */
//					frag->size = fragLength;
//					totalFragSize += frag->size;
//					frag->seq.reserve(frag->size);
//					frag->endPos = frag->startPos + frag->size - 1;
//
//
//					/* Put fragments in appropriate bins */
//					binNum1 = (int) ceil((qreal) (frag->startPos - FRAG_DESC_GAP) / maxPartitionSize);
//					binNum1--;
//					if (binNum1 < 0) binNum1 = 0; /* Take care of negative fragment positions */
//					binNum2 = (int) ceil((qreal) frag->endPos / maxPartitionSize);
//					binNum2--;
//					/* Take care of instances where fragment end position
//					 * is greater than number of bases in contig */
//					if (binNum2 >= bin.size()) binNum2 = bin.size() - 1;
//					if (binNum1 == binNum2)
//						bin[binNum1]->append(frag);
//					else
//					{
//						bin[binNum1]->append(frag);
//						for (j = binNum1 + 1; j <= binNum2; ++j)
//							bin[j]->prepend(frag);
//					}
//
//					/* Read fragment sequence */
//					while (true)
//					{
//						line = in.readLine().toAscii();
//						if (line.isEmpty()) break;
//						frag->seq += line;
//					}
//					parsedSize += frag->size;
//					tmpParsedSize += frag->size;
//					if (tmpParsedSize >= 5000)
//					{
//						tmpParsedSize = 0;
//						emit parsingProgress((int) ceil((qreal) parsedSize / BYTE_TO_MBYTE));
//						QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//					}
//
//					/* If all fragments belonging to the current contig
//					 * have been parsed */
//					if (contig->numberReads == fragsInCurrentContig)
//					{
//						/* Calculate contig coverage */
//						contig->coverage = ((qreal) totalFragSize) / contig->size;
//
//						/* Calculate average fragment size */
//						avgFragSize = (int) ceil(((qreal) totalFragSize) / contig->numberReads);
//
//						/* Assign vertical positions to fragments */
//						emit messageChanged("");
//						QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//						emit messageChanged("Aligning Reads...");
//						QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//						startTime = QTime::currentTime();
//						//assignFragYPos(bin, maxYPos);
//						assignFragYPos(contig, avgFragSize, maxYPos);
//						contig->maxFragRows = maxYPos;
//						endTime = QTime::currentTime();
//						//qDebug() << "assignFragYPos() = " << startTime.msecsTo(endTime);
//
//						/* Save data into DB */
//						emit messageChanged("");
//						QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//						emit messageChanged("Saving data...");
//						QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//						//Database::beginTransaction();
//						startTime = QTime::currentTime();
//						if (!insertFileIntoDB(fileName, fileNum))
//						{
//							delete contig;
//							file.close();
//							return false;
//						}
//						if (!insertContigIntoDB(contig))
//						{
//							delete contig;
//							file.close();
//							return false;
//						}
//						endTime = QTime::currentTime();
//						//qDebug() << "insertContigIntoDB() = " << startTime.msecsTo(endTime);
//
//						startTime = QTime::currentTime();
//						if (!insertFragsIntoDB(contig->getFragList()))
//						{
//							delete contig;
//							file.close();
//							return false;
//						}
//						endTime = QTime::currentTime();
//						//qDebug() << "insertFragsIntoDB() = " << startTime.msecsTo(endTime);
//
//						startTime = QTime::currentTime();
//						if (!insertSnpIntoDB(contig, bin, maxPartitionSize))
//						{
//							delete contig;
//							file.close();
//							return false;
//						}
//						//insertSnpIntoDB(contig, avgFragSize);
//						endTime = QTime::currentTime();
//						//qDebug() << "insertSnpIntoDB() = " << startTime.msecsTo(endTime);
//						//Database::endTransaction();
//
//						delete contig;
//						fragIndex = -1;
//						fragIndex2 = -1;
//
//						/* Clean-up the bin */
//						foreach (subBin, bin)
//						{
//							subBin->clear();
//							delete subBin;
//						}
//						bin.clear();
//					}
//				}
//			}
//			/* Get bases that are high-quality and were mapped successfully */
//			else if (sscanf(line, "QA %d %d %d %d", &qualStart, &qualEnd, &alignStart, &alignEnd) == 4)
//			{
//				frag->qualStart = qualStart;
//				frag->qualEnd = qualEnd;
//				frag->alignStart = alignStart;
//				frag->alignEnd = alignEnd;
//			}
//			/* When any blank line is detected, stop sequence collection */
//			else if (line.isEmpty())
//			{
//
//			}
//
//			if (tmpParsedSize >= 5000)
//			{
//				tmpParsedSize = 0;
//				emit parsingProgress((int) ceil((qreal) parsedSize / BYTE_TO_MBYTE));
//				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//			}
//		}
//		file.close();
//		fileNum++;
//	}
//	if (!updateFragMapping(fragNameOccurenceHash))
//		return false;
//
//    Contig::setTotalSize(totalContigSize);
//
//    emit parsingFinished();
//
//	/* If number of contigs is 0, then display a message and return false */
//	if (contigNum == 0)
//	{
//		QMessageBox::warning(
//				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
//				tr("Basejumper"),
//				tr("No contigs found in selected file(s)."));
//		return false;
//	}
//	Contig::setNumContigs(contigNum);
//
//    /* Parse 'order.txt' file, if it exists */
//    if (QFile::exists(orderFile))
//    {
//    	isOrderFileLoaded = readOrderFile(orderFile);
//    	emit orderParsingFinished();
//    	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
//    }
//    endTime = QTime::currentTime();
//    //qDebug() << "readAce total time = " << startTime.msecsTo(endTime) << endl;
//	return true;
//}


/**
 * Reads the given annotation files in BED format and stores data into the DB.
 *
 * In order to display annotation, order.txt file should either exist in the
 * same folder as the annotation files or in the same folder as the sequence
 * files.
 *
 * @param fileList : Reference to the list of files that contain
 * annotation data
 * @param listSize : Size of the file list
 * @param path : Reference to the path to the folder that contains the
 * annotation files
 *
 * @return Returns true on success and false on failure.
 */
bool Parser::readBedFiles(
		const QStringList &fileList,
		int listSize,
		QString &path)
{
	QString filename, filenameFull, genesTrack, snpsTrack, message;
	QString exonTrack, intronTrack, utr5pTrack, utr3pTrack;
	QByteArray line;
	QList<QByteArray> fieldList;
	QSqlQuery query, query2, query3, query4, query5, query6;
	int index, annotId, contigId, fileId;
	int parsedFileIndex, annotationTypeId;
	int tmpParsedSize, maxPartitionSize, numPartitions, binNum1, binNum2;
	bool isTrackKnown, isGeneFileLoaded;
	QRegExp pathRegExp;
	quint64 totalFileSize, parsedSize;
	QStringList geneStructureFileList;
	Gene *gene;
	QHash<int, QList<Gene *> *> contig_geneList_map;
	Gene::Strand direction;
	AnnotationList::Type trackType;
	QList<QString> chromList;
	QList<int> startPosList;
	QList<int> endPosList;
	QList<int> contigIdList;
	QString chromName, annotName, contigChromName, str;
	int annotStartPos, annotEndPos, contigIdListSize;
	int chromStartPos, chromEndPos, startPosWithinContig, endPosWithinContig;

	/* Initialize variables */
	genesTrack = "name=\"Genes\"";
	snpsTrack = "name=\"SNPs\"";
	exonTrack = "name=\"Exons\"";
	intronTrack = "name=\"Introns\"";
	utr3pTrack = "name=\"3PUTR\"";
	utr5pTrack = "name=\"5PUTR\"";
	trackType = AnnotationList::Custom;
	pathRegExp.setPattern(".+\\/$");
	totalFileSize = Q_UINT64_C(0);
	parsedSize = Q_UINT64_C(0);
	tmpParsedSize = 0;
	maxPartitionSize = MAX_CONTIG_PARTITION;
	annotId = 0;
	isGeneFileLoaded = false;
	parsedFileIndex = 0;

	/* If the given path does not contain '/' seperator
	 * at the end, add it. */
	if (!pathRegExp.exactMatch(path))
		path += "/";

	/* Emit signal to indicate total size of annotation files */
	foreach (QString str, fileList)
	{
		if (isOrderFileLoaded
				&& str.compare("order.txt", Qt::CaseInsensitive) == 0)
			continue;
		str = path + str;
		totalFileSize += QFileInfo(str).size();
	}
	emit totalSize((int) ceil((qreal) totalFileSize / BYTE_TO_MBYTE));

	/* Parse order.txt file, if it has not been parsed already. */
	if (!isOrderFileLoaded)
	{
		index = fileList.indexOf(QRegExp("order\\.txt$", Qt::CaseInsensitive));

		/* If 'order.txt' exists */
		if (index >= 0)
		{
			filename = path + fileList.at(index);
			isOrderFileLoaded = readOrderFile(filename);
			if (!isOrderFileLoaded)
				return false;
			parsedSize += QFileInfo(filename).size();
			emit parsingProgress((int) ceil((qreal) parsedSize / BYTE_TO_MBYTE));
			emit orderParsingFinished();
			QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		}
		/* If 'order.txt' does not exist, then throw
		 * an error. */
		else
		{
			QMessageBox::critical(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Error: Couldn't find file 'order.txt'. 'order.txt' file"
						" should either exist in the directory containing"
						" ACE files or in the directory containing"
						" annotation files."));
			return false;
		}
	}

	/* Send a signal to indicate parsing of annotation files */
	emit messageChanged("Parsing annotation files...");

	/* Create a matrix or a table consisting of chromosome name,
	 * contig start position, contig end position and contig ID.
	 * This matrix will be used in determining the contig to
	 * which an annotation belongs to. */
	str = "select chromosome.name, "
			" chrom_contig.contigId, "
			" chrom_contig.chromStart,"
			" chrom_contig.chromEnd "
			" from chrom_contig, chromosome "
			" where chrom_contig.chromId = chromosome.id ";
	if (!query2.exec(str))
	{
		QMessageBox::critical(
			(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
			tr("Basejumper"),
			tr("Error fetching from 'chrom_contig' and 'chromosome' tables.\nReason: "
					+ query2.lastError().text().toAscii()));
		return false;
	}
	while (query2.next())
	{
		chromList.append(query2.value(0).toString());
		startPosList.append(query2.value(2).toInt());
		endPosList.append(query2.value(3).toInt());
		contigIdList.append(query2.value(1).toInt());
	}
	contigIdListSize = contigIdList.size();

	query.prepare("insert into annotation "
			" (contigId, startPos, endPos, name, annotationTypeId) "
			" values("
			" :contigId, "
			" :startPos, "
			" :endPos, "
			" :name, "
			" :annotationTypeId) ");

	query3.prepare("insert or replace into file "
			" (file_name, filepath) "
			" values "
			" (:file_name, :filePath) ");
	query4.prepare("insert into gene "
			" (geneId, yPos, strand) "
			" values (:geneId, :yPos, :strand)");
	query5.prepare("update contig "
			" set maxGeneRows = :maxGeneRows "
			" where id = :contigId");
	query6.prepare("select annotationType.id "
			" from annotationType, file "
			" where annotationType.fileId = file.id "
			" and file.file_name = :fileName "
			" and annotationType.type = :type");

	/* Begin transaction */
	//Database::beginTransaction();

	/* Parse each file in the list */
	for (int i = 0; i < listSize; ++i)
	{
		isTrackKnown = false;
		filename = fileList.at(i);
		if (!filename.endsWith(".bed"))
			continue;

		/* Send a signal to indicate the file name being parsed */
		message = "Parsing annotation file "
			+ QString::number(++parsedFileIndex) + " of "
			+ QString::number(listSize) + "..."
			+ "\n(File: " + filename + ")";
		emit messageChanged(message);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

		/* Save filename in DB */
		query3.bindValue(":file_name", filename);
		query3.bindValue(":filePath", path + filename);
		if (!query3.exec())
		{
			QMessageBox::critical(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Error inserting filename into DB.\nReason: "
						+ query3.lastError().text().toAscii()));
			query2.clear();
			query6.clear();
			//Database::rollbackTransaction();
			return false;
		}
		fileId = query3.lastInsertId().toInt();

		/* Try opening the file */
		filenameFull = path + filename;
		QFile file(filenameFull);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Cannot read file %1:\n%2.")
					.arg(file.fileName())
					.arg(file.errorString()));
			query2.clear();
			query6.clear();
			//Database::rollbackTransaction();
			return false;
		}

		/* Create a text stream */
		QTextStream in(&file);
		in.setIntegerBase(10);
		in.setCodec("UTF-8");

		/* Read each line and store data in DB */
		while (!in.atEnd())
		{
			line = in.readLine().toAscii();
			parsedSize += line.size();
			tmpParsedSize += line.size();

			/* Find what track this file contains */
			if (line == "")
				continue;
			else if (!isTrackKnown && line.startsWith("track name="))
			{
				fieldList = line.split(' ');

				if (fieldList.at(1) == genesTrack)
				{
					trackType = AnnotationList::Gene;
					isGeneFileLoaded = true;
				}
				else if (fieldList.at(1) == snpsTrack)
					trackType = AnnotationList::Snp;
				else if (fieldList.at(1) == intronTrack
						|| fieldList.at(1) == exonTrack
						|| fieldList.at(1) == utr5pTrack
						|| fieldList.at(1) == utr3pTrack)
				{
					geneStructureFileList.append(filename);
					--parsedFileIndex;
					break;
				}
				else
					trackType = AnnotationList::Custom;
				isTrackKnown = true;
				fieldList.clear();

				/* Determine annotationTypeId */
				query6.bindValue(":fileName", filename);
				query6.bindValue(":type", (int) trackType);
				if (!query6.exec())
				{
					QMessageBox::critical(
						(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
						tr("Basejumper"),
						tr("Error fetching data from DB.\nReason: "
								+ query6.lastError().text().toAscii()));
					query2.clear();
					query6.clear();
					//Database::rollbackTransaction();
					return false;
				}
				if (query6.next())
					annotationTypeId = query6.value(0).toInt();
				else
				{
					QMessageBox::critical(
						QApplication::activeWindow(),
						tr("Basejumper"),
						tr("Error: Couldn't find 'annotationTypeId'"));
					query2.clear();
					query6.clear();
					//Database::rollbackTransaction();
					return false;
				}

				continue;
			}
			fieldList = line.split('\t');
			chromName = QString(fieldList.at(0));
			annotStartPos = fieldList.at(1).toInt();
			annotEndPos = fieldList.at(2).toInt();
			annotName = QString(fieldList.at(3));

			for (int j = 0; j < contigIdListSize; ++j)
			{
				chromStartPos = startPosList.at(j);
				chromEndPos = endPosList.at(j);
				contigChromName = chromList.at(j);
				contigId = contigIdList.at(j);

				if (annotStartPos >= chromStartPos
						&& annotEndPos <= chromEndPos
						&& chromName.compare(contigChromName) == 0)
				{
					/* Insert into Annotation table */
					++annotId;
					startPosWithinContig = annotStartPos - chromStartPos;
					endPosWithinContig = annotEndPos - chromStartPos;
					query.bindValue(":contigId", contigId);
					query.bindValue(":startPos", startPosWithinContig);
					query.bindValue(":endPos", endPosWithinContig);
					query.bindValue(":name", annotName);
					query.bindValue(":annotationTypeId", annotationTypeId);
					if (!query.exec())
					{
						QMessageBox::critical(
							(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
							tr("Basejumper"),
							tr("Error inserting data into 'annotation' table.\nReason: "
									+ query.lastError().text().toAscii()));
						query2.clear();
						query6.clear();
						//Database::rollbackTransaction();
						return false;
					}

					/* If this is a gene track, put genes in appropriate bins */
					if (trackType == AnnotationList::Gene)
					{
						if (fieldList.at(5) == "+")
							direction = Gene::Downstream;
						else
							direction = Gene::Upstream;

						gene = new Gene(annotId,
								chromName,
								startPosWithinContig,
								endPosWithinContig,
								-1,
								direction);
						if (!contig_geneList_map.contains(contigId))
							contig_geneList_map.insert(contigId, new QList<Gene *>);
						contig_geneList_map.value(contigId)->append(gene);
					}
				}
			}

			fieldList.clear();

			/* Emit signal indicating the size that has been already parsed */
			if (tmpParsedSize >= 5000)
			{
				tmpParsedSize = 0;
				emit parsingProgress((int) ceil((qreal) parsedSize / BYTE_TO_MBYTE));
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}
		}
	}

	QList<int> keys = contig_geneList_map.keys();
	if (keys.size() > 0)
	{
		QList<Gene *> *geneList;
		QList <QList <Gene *>*> bin;
		QList<Gene *> *subBin;
		int key, maxY, j, k;

		foreach (key, keys)
		{
			numPartitions = (int) ceil((qreal) Contig::getSize(key) / maxPartitionSize);
			for (j = 0; j < numPartitions; ++j)
				bin.append(new QList<Gene *>);
			geneList = contig_geneList_map.value(key);
			foreach (Gene *g, *geneList)
			{
				binNum1 = (int) ceil((qreal) g->startPos / maxPartitionSize);
				binNum1--;
				if (binNum1 < 0) binNum1 = 0; /* Take care of negative gene positions */
				binNum2 = (int) ceil((qreal) g->endPos / maxPartitionSize);
				binNum2--;
				if (binNum1 == binNum2)
					bin[binNum1]->append(g);
				else
				{
					bin[binNum1]->append(g);
					for (k = binNum1 + 1; k <= binNum2; ++k)
						bin[k]->prepend(g);
				}
			}

			/* Assign vertical positions to genes */
			maxY = 0;
			assignGeneYPos(bin, maxY);

			/* Insert into Gene table */
			foreach (Gene *g, *geneList)
			{
				query4.bindValue(":geneId", g->id);
				query4.bindValue(":yPos", g->yPos);
				query4.bindValue(":strand", g->direction);
				if (!query4.exec())
				{
					QMessageBox::critical(
						(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
						tr("Basejumper"),
						tr("Error inserting data into 'gene' table.\nReason: "
								+ query4.lastError().text().toAscii()));
					query2.clear();
					query6.clear();
					//Database::rollbackTransaction();
					return false;
				}
			}

			/* Update Contig table */
			query5.bindValue(":maxGeneRows", maxY);
			query5.bindValue(":contigId", key);
			if (!query5.exec())
			{
				QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Error updating 'contig' table.\nReason: "
							+ query5.lastError().text().toAscii()));
				query2.clear();
				query6.clear();
				//Database::rollbackTransaction();
				return false;
			}

			/* Delete each gene */
			foreach (Gene *g, *geneList)
				delete g;

			/* Clean-up the bin */
			foreach (subBin, bin)
			{
				subBin->clear();
				delete subBin;
			}
			bin.clear();

			/* Delete geneList */
			contig_geneList_map[key] = NULL;
			delete geneList;
		}
	}

	query2.clear();
	query6.clear();
	//Database::endTransaction();

	/* Parse gene structure files */
	readStructureFiles(
			geneStructureFileList,
			geneStructureFileList.size(),
			path,
			parsedSize,
			parsedFileIndex,
			listSize);

	emit annotationLoaded();
	emit parsingProgress((int) ceil((qreal) parsedSize / BYTE_TO_MBYTE));
	emit annotationParsingFinished();
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	return true;
}


/**
 * Reads gene sub-structure files.
 *
 * These files contain exon, intron, 3PUTR, and 5PUTR positions in BED
 * format.
 *
 * @param fileList : Reference to the list of files that contain
 * gene-substructure data
 * @param listSize : Size of the file list
 * @param path : Reference to the path to the folder that contains the
 * gene-substructure files
 */
bool Parser::readStructureFiles(
		const QStringList &fileList,
		int listSize,
		QString &path,
		quint64 parsedSize,
		int parsedFileIndex,
		int parsedTotalFiles)
{
	QSqlQuery query, query2, query3;
	QString filename, filenameFull, exonTrack, intronTrack, utr5pTrack, utr3pTrack;
	QString geneName, geneStructureName, chromName, tmp, message, str;
	QByteArray line;
	QList<QByteArray> fieldList;
	bool isTrackKnown;
	int start, end, geneId, contigId, tmpParsedSize;
	GeneStructure::SubstructureType subStructureType;
	QList<QString> chromList;
	QList<int> startPosList;
	QList<int> endPosList;
	int contigIdListSize, annotIdListSize, startPosWithinContig, endPosWithinContig;
	QList<QString> contigChromList;
	QList<int> contigIdList;
	QList<int> contigStartPosList;
	QList<int> contigEndPosList;
	QList<int> annotIdList;
	QList<int> annotContigIdList;
	QList<int> annotStartPosList;
	QList<int> annotEndPosList;
	QList<QString> annotNameList;


	contigId = 0;
	tmpParsedSize = 0;
	exonTrack = "name=\"Exons\"";
	intronTrack = "name=\"Introns\"";
	utr5pTrack = "name=\"5PUTR\"";
	utr3pTrack = "name=\"3PUTR\"";

	/* Create a matrix or a table consisting of chromosome name,
	 * contig start position, contig end position and contig ID.
	 * This matrix will be used in determining the contig to
	 * which an gene structure belongs to. */
	str = "select chromosome.name, "
			" chrom_contig.contigId, "
			" chrom_contig.chromStart,"
			" chrom_contig.chromEnd "
			" from chrom_contig, chromosome "
			" where chrom_contig.chromId = chromosome.id ";
	if (!query3.exec(str))
	{
		QMessageBox::critical(
			(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
			tr("Basejumper"),
			tr("Error fetching from 'chrom_contig' and 'chromosome' tables.\nReason: "
					+ query3.lastError().text().toAscii()));
		return false;
	}
	while (query3.next())
	{
		contigChromList.append(query3.value(0).toString());
		contigIdList.append(query3.value(1).toInt());
		contigStartPosList.append(query3.value(2).toInt());
		contigEndPosList.append(query3.value(3).toInt());
	}
	contigIdListSize = contigIdList.size();

	/* Create a matrix or a table consisting of annotation ID,
	 * contig ID associated with the annotation, annotation start position,
	 * annotation end position, and annotation name. This matrix will be
	 * used in determining the annotation to which a gene sub structure
	 * belongs to. */
	str = "select annotation.id, annotation.contigId, annotation.startPos, "
			" annotation.endPos, annotation.name "
			" from annotationType, annotation "
			" where annotationType.id = annotation.annotationTypeId "
			" and annotationType.type = " + QString::number((int) AnnotationList::Gene) +
			" order by annotation.contigId asc";
	if (!query2.exec(str))
	{
		QMessageBox::critical(
			(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
			tr("Basejumper"),
			tr("Error fetching from 'chrom_contig' and 'chromosome' tables.\nReason: "
					+ query2.lastError().text().toAscii()));
		return false;
	}
	while (query2.next())
	{
		annotIdList.append(query2.value(0).toInt());
		annotContigIdList.append(query2.value(1).toInt());
		annotStartPosList.append(query2.value(2).toInt());
		annotEndPosList.append(query2.value(3).toInt());
		annotNameList.append(query2.value(4).toString());
	}
	annotIdListSize = annotIdList.size();

	//Database::beginTransaction();
	query.prepare("insert into geneStructure "
			" (type, geneId, name, start, end) "
			" values "
			" (:type, :geneId, :name, :start, :end) ");

	/* Parse each file in the list */
	for (int i = 0; i < listSize; ++i)
	{
		isTrackKnown = false;

		/* Open file */
		filename = fileList.at(i);
		if (!filename.endsWith(".bed"))
			continue;
		filenameFull = path + filename;
		QFile file(filenameFull);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Cannot read file %1:\n%2.")
					.arg(file.fileName())
					.arg(file.errorString()));
			return false;
		}

		/* Send a signal to indicate the file name being parsed */
		message = "Parsing annotation file "
			+ QString::number(++parsedFileIndex) + " of "
			+ QString::number(parsedTotalFiles)	+ "..."
			+ "\n(File: " + filename + ")";
		emit messageChanged(message);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

		/* Create a text stream */
		QTextStream in(&file);
		in.setIntegerBase(10);
		in.setCodec("UTF-8");

		/* Read each line and store data in DB */
		while (!in.atEnd())
		{
			line = in.readLine().toAscii();
			parsedSize += line.size();
			tmpParsedSize += line.size();

			/* Find what track this file contains */
			if (line == "")
				continue;
			else if (!isTrackKnown && line.startsWith("track name="))
			{
				fieldList = line.split(' ');

				if (fieldList.at(1) == intronTrack)
					subStructureType = GeneStructure::INTRON;
				else if (fieldList.at(1) == exonTrack)
					subStructureType = GeneStructure::EXON;
				else if (fieldList.at(1) == utr5pTrack)
					subStructureType = GeneStructure::UTR5P;
				else if (fieldList.at(1) == utr3pTrack)
					subStructureType = GeneStructure::UTR3P;
				isTrackKnown = true;
				fieldList.clear();

				continue;
			}
			fieldList = line.split('\t');
			chromName = QString(fieldList.at(0));
			start = fieldList.at(1).toInt();
			end = fieldList.at(2).toInt();
			geneStructureName = QString(fieldList.at(3));

			for (int j = 0; j < contigIdListSize; ++j)
			{
				if (start >= contigStartPosList.at(j)
						&& end <= contigEndPosList.at(j)
						&& chromName.compare(contigChromList.at(j), Qt::CaseInsensitive) == 0)
				{
					contigId = contigIdList.at(j);
					startPosWithinContig = start - contigStartPosList.at(j);
					endPosWithinContig = end - contigStartPosList.at(j);

					for (int k = 0; k < annotIdListSize; ++k)
					{
						if (contigId == annotContigIdList.at(k)
							&& startPosWithinContig >= annotStartPosList.at(k)
							&& endPosWithinContig <= annotEndPosList.at(k)
							&& geneStructureName.startsWith(annotNameList.at(k), Qt::CaseInsensitive))
						{
							geneId = annotIdList.at(k);

							/* Save gene sub-structure in DB */
							query.bindValue(":type", (int) subStructureType);
							query.bindValue(":geneId", geneId);
							query.bindValue(":name", geneStructureName);
							query.bindValue(":start", startPosWithinContig);
							query.bindValue(":end", endPosWithinContig);
							if (!query.exec())
							{
								QMessageBox::critical(
									(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
									tr("Basejumper"),
									tr("Error inserting data into geneStructure table.\nReason: "
											+ query.lastError().text().toAscii()));
								//Database::rollbackTransaction();
								return false;
							}
							break;
						}
					}
				}
			}

			/* Emit signal indicating the size that has been already parsed */
			if (tmpParsedSize >= 5000)
			{
				tmpParsedSize = 0;
				emit parsingProgress((int) ceil((qreal) parsedSize / BYTE_TO_MBYTE));
				QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
			}
		}
	}
	//Database::endTransaction();
	return true;
}


/**
 * Reads order.txt file
 *
 * @param filename : Name of the file containing order data
 *
 * @return Returns true on success and false on failure
 */
bool Parser::readOrderFile(const QString &filename)
{
	QSqlQuery query, query1, query2, query3, query4, query5, query7, query11;
	QList<QString> tokenList;
	int annotOrder, contigOrder;
	QString line, queryStr, chromName;
	bool annotationSection, sequenceSection;
	QRegExp commentRegExp;
	QRegExp annotationRegExp, annotationItemsRegExp, annotationEndRegExp;
	QRegExp sequenceRegExp, sequenceItemsRegExp, sequenceEndRegExp;
	QHash<int, int> contigOrderSizeHash, contigIdDistanceHash, contigOrderIdHash;
	QHash<QString, QString> fileAliasHash, contigChromHash;
	QHash<QString, QString> contigStartPosHash, contigEndPosHash;
	QHash<QString, int> contigOrderHash;
	QSet<QString> contigsSet;
	QList<QString> annotationFileList, annotationTypeList, annotationAliasList;
	QList<QString> chromList, startPosList, endPosList, contigNameList;
	int tokenListSize;

	/* Initialization */
	annotationSection = false;
	sequenceSection = false;
	annotOrder = 0;
	contigOrder = 0;
	fileOrderHash.clear();

	commentRegExp.setPattern("^\\#.*$");

	annotationRegExp.setPattern(QRegExp::escape("[annotation files]"));
	annotationRegExp.setCaseSensitivity(Qt::CaseInsensitive);
	annotationItemsRegExp.setPattern("^.+\\.[a-z]{3}\\t[(snp)|(gene)|(custom)]\\t?.*$");
	annotationItemsRegExp.setCaseSensitivity(Qt::CaseInsensitive);
	annotationEndRegExp.setPattern(QRegExp::escape("[/annotation files]"));

	sequenceRegExp.setPattern(QRegExp::escape("[sequence files]"));
	sequenceRegExp.setCaseSensitivity(Qt::CaseInsensitive);
	sequenceItemsRegExp.setPattern("^.+\\t\\d+\\t\\d+\\t.+$");
	sequenceItemsRegExp.setCaseSensitivity(Qt::CaseInsensitive);
	sequenceEndRegExp.setPattern(QRegExp::escape("[/sequence files]"));

	/* Send a signal to indicate parsing of order.txt file */
	emit orderParsingStarted();
	emit messageChanged("Parsing order.txt file...");
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	/* Open file */
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Cannot read file %1:\n%2.")
				.arg(file.fileName())
				.arg(file.errorString()));
		emit messageChanged("");
		return false;
	}

	/* Send a signal indicating the index of the file being parsed */
	emit parsingFile(1);
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	/* Read lines from file */
	QTextStream in(&file);
	in.setIntegerBase(10);
	in.setCodec("UTF-8");
	while (!in.atEnd())
	{
		line = in.readLine();

		/* Matches [annotation files] section */
		if (!annotationSection && annotationRegExp.exactMatch(line))
			annotationSection = true;
		/* Matches lines in annotation section */
		else if (annotationSection && annotationItemsRegExp.exactMatch(line))
		{
			tokenList = line.split('\t');
			tokenListSize = tokenList.size();

			/* If there are exactly 2 columns */
			if (tokenListSize == 2)
			{
				annotationFileList.append(tokenList.at(0));
				annotationTypeList.append(tokenList.at(1));
				/* Use file name as alias */
				annotationAliasList.append(tokenList.at(0));
			}
			/* If there are exactly 3 columns */
			else if (tokenListSize == 3)
			{
				annotationFileList.append(tokenList.at(0));
				annotationTypeList.append(tokenList.at(1));
				annotationAliasList.append(tokenList.at(2));
			}
			/* If there are less than 2 columns or more than 3 columns */
			else
			{
				QMessageBox::critical(
						QApplication::activeWindow(),
						tr("Basejumper"),
						tr("In order.txt file, annotation section has either "
								"more than 3 columns or less than 2 columns. The "
								"required 2 columns are annotation file name "
								"and annotation type. The optional third "
								"column is annotation alias. "));
				emit messageChanged("");
				return false;
			}
		}
		/* Matches annotation end section */
		else if (annotationSection && annotationEndRegExp.exactMatch(line))
			annotationSection = false;
		/* Matches [sequence files] section */
		else if (!sequenceSection && sequenceRegExp.exactMatch(line))
			sequenceSection = true;
		/* Matches lines in sequence section */
		else if (sequenceSection && sequenceItemsRegExp.exactMatch(line))
		{
			tokenList = line.split('\t');
			if (tokenList.size() != 4)
			{
				QMessageBox::critical(
						QApplication::activeWindow(),
						tr("Basejumper"),
						tr("In order.txt file, sequence section does not have "
								"4 columns. The 4 columns needed are: "
								"chromosome name, start position of contig, "
								"end position of contig, and contig name. "));
				emit messageChanged("");
				return false;
			}

			chromList.append(tokenList.at(0));
			startPosList.append(tokenList.at(1));
			endPosList.append(tokenList.at(2));
			contigNameList.append(tokenList.at(3));
		}
		/* Matches sequence end section */
		else if (sequenceSection && sequenceEndRegExp.exactMatch(line))
			sequenceSection = false;
		/* Matches empty line or comment */
		else if (line.isEmpty() || commentRegExp.exactMatch(line))
			continue;
	}

	/* Check if the loaded contigs match with the contigs
	 * in order.txt file. */
	contigsSet = contigNameList.toSet();
	if (!compareSets(loadedContigsSet, loadedContigsSet.size(),
			contigsSet, contigsSet.size()))
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::warning(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Loaded contigs and the contigs in"
						" 'order.txt' file do not match. You will not"
						" be able to load annotation files without"
						" fixing this first. "));
		return false;
	}

	/* Begin transaction */
	//if (!Database::beginTransaction())
	//	return false;

	/* Insert filename into 'file' table and insert annotation types
	 * into 'annotationType' table */
	int fileId = 0;
	query3.prepare("insert or ignore into file "
			" (file_name) "
			" values "
			" (:fileName)");
	query4.prepare("insert into annotationType "
			" (type, annotOrder, alias, fileId) "
			" values "
			" (:type, :order, :alias, :fileId) ");
	for (int i = 0; i < annotationFileList.size(); ++i)
	{
		/* Insert filename into 'file' table */
		query3.bindValue(":fileName", annotationFileList.at(i));
		if (!query3.exec())
		{
			QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Error inserting data into 'file' table.\nReason: "
							+ query3.lastError().text().toAscii()));
			emit messageChanged("");
			//Database::rollbackTransaction();
			return false;
		}
		if (query3.lastInsertId().isNull()
				|| query3.lastInsertId().toInt() <= 0)
		{
			QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Error fetching file ID "));
			emit messageChanged("");
			//Database::rollbackTransaction();
			return false;
		}
		fileId = query3.lastInsertId().toInt();

		/* Insert annotation types into 'annotationType' table */
		if (annotationTypeList.at(i) == "snp")
			query4.bindValue(":type", (int) AnnotationList::Snp);
		else if (annotationTypeList.at(i) == "gene")
			query4.bindValue(":type", (int) AnnotationList::Gene);
		else
			query4.bindValue(":type", (int) AnnotationList::Custom);
		query4.bindValue(":order", i+1);
		query4.bindValue(":alias", annotationAliasList.at(i));
		query4.bindValue(":fileId", fileId);
		if (!query4.exec())
		{
			QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Error inserting data into 'annotationType' table.\nReason: "
							+ query4.lastError().text().toAscii()));
			emit messageChanged("");
			//Database::rollbackTransaction();
			return false;
		}
	}

	/* Insert chromosome into 'chromosome' table */
	query1.prepare(" insert or ignore into chromosome "
			" (id, name) "
			" values"
			" (:id, :chromName) ");
	QSet<QString> chromSet = chromList.toSet();
	int index = 0;
	foreach (QString chromName, chromSet.values())
	{
		query1.bindValue(":id", ++index);
		query1.bindValue(":name", chromName);
		if (!query1.exec())
		{
			QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Error inserting data into 'chromosome' table.\nReason: "
							+ query1.lastError().text().toAscii()));
			emit messageChanged("");
			//Database::rollbackTransaction();
			return false;
		}
	}

	/* Insert chromosome-contig mapping into 'chrom_contig' table */
	query2.prepare(" insert into chrom_contig "
			" (chromId, contigId, chromStart, chromEnd) "
			" values("
			" (select id from chromosome where name = :chromName), "
			" (select id from contig where name = :contigName), "
			" :chromStart, "
			" :chromEnd) ");
	for (int i = 0; i < contigNameList.size(); ++i)
	{
		query2.bindValue(":chromName", chromList.at(i));
		query2.bindValue(":contigName", contigNameList.at(i));
		query2.bindValue(":chromStart", startPosList.at(i));
		query2.bindValue(":chromEnd", endPosList.at(i));
		if (!query2.exec())
		{
			QMessageBox::critical(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Error inserting data into 'chrom_contig' table.\nReason: "
						+ query2.lastError().text().toAscii()));
			emit messageChanged("");
			//Database::rollbackTransaction();
			return false;
		}
	}

	/* Update 'contigOrder' column in 'contig' table */
	query7.prepare("update contig "
			" set contigOrder = :contigOrder "
			" where name = :contigName");
	for (int i = 0; i < contigNameList.size(); ++i)
	{
		query7.bindValue(":contigOrder", i+1);
		query7.bindValue(":contigName", contigNameList.at(i));
		if (!query7.exec())
		{
			QMessageBox::critical(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Error updating 'contig' table.\nReason: "
						+ query7.lastError().text().toAscii()));
			emit messageChanged("");
			//Database::rollbackTransaction();
			return false;
		}
	}

	/* Fetch 'id' and 'contigOrder' from 'contig' table. */
	queryStr = "select id, contigOrder "
			" from contig "
			" order by contigOrder asc";
	if (!query.exec(queryStr))
	{
		QMessageBox::critical(
			(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
			tr("Basejumper"),
			tr("Error fetching from contig table.\nReason: "
					+ query.lastError().text().toAscii()));
		emit messageChanged("");
		//Database::rollbackTransaction();
		return false;
	}
	while (query.next())
		Contig::orderMap[query.value(1).toInt()] = query.value(0).toInt();

//	for (QMap<int, int>::iterator i = Contig::orderMap.begin(); i != Contig::orderMap.end(); i++)
//	{
//		//std::cout << i.key() << " => " << i.value() << endl;
//	}


	/* Clear the prepared queries */
	query.clear();
	query1.clear();
	query2.clear();
	query4.clear();
	query5.clear();
	query7.clear();

	/* End transaction */
	//if (!Database::endTransaction())
	//	return false;

	/* Send signal to indicate that order.txt parsing is finished */
	//emit orderParsingFinished();

	return true;
}


/**
 * Parses the given file containing cytoband data
 *
 * @param cytoband filename
 * @return True on success, False on failure
 */
bool Parser::readCytoband(const QString &filename)
{
	QString line;
	QRegExp pattern;
	int lineNum;
	QList<QString> tokenList;
	QSqlQuery query, query2;
	QHash<QString, int> chromNameIdHash;

	/* Initialize */
	pattern.setPattern("^.+\\t\\d+\\t\\d+\\t.+\\t.+$");
	lineNum = 0;
	query.prepare("insert into cytoband "
			" (chromId, chromStart, chromEnd, name, stain) "
			" values "
			" (:chromId, :chromStart, :chromEnd, :name, :stain)");

	/* Prefetch chromosome data */
	QString str = "select id, name "
			" from chromosome ";
	if (!query2.exec(str))
	{
		QMessageBox::critical(
				QApplication::activeWindow(),
				tr("Basejumper"),
				tr("Error fetching data from 'chromosome' table.\nReason: "
						+ query2.lastError().text().toAscii()));
		return false;
	}
	while (query.next())
		chromNameIdHash.insert(query2.value(1).toString(), query2.value(0).toInt());
	if (chromNameIdHash.size() < 1)
	{
		QMessageBox::critical(
				QApplication::activeWindow(),
				tr("Basejumper"),
				tr("No existing chromosomes found in database"));
		return false;
	}

	/* Open file */
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QMessageBox::critical(
				QApplication::activeWindow(),
				tr("Basejumper"),
				tr("Cannot read file %1:\n%2.")
				.arg(file.fileName())
				.arg(file.errorString()));
		emit messageChanged("");
		return false;
	}

	/* Start transaction */
	//Database::beginTransaction();

	/* Read lines from file */
	QTextStream in(&file);
	in.setIntegerBase(10);
	in.setCodec("UTF-8");
	while (!in.atEnd())
	{
		line = in.readLine();
		lineNum++;

		if (!pattern.exactMatch(line))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Invalid data at line %1 in %2.")
					.arg(lineNum)
					.arg(file.fileName()));
			emit messageChanged("");
			return false;
		}
		tokenList = line.split('\t');

		query.bindValue(":chromId", chromNameIdHash.value(tokenList.at(0)));
		query.bindValue(":chromStart", tokenList.at(1).toInt());
		query.bindValue(":chromEnd", tokenList.at(2).toInt());
		query.bindValue(":name", tokenList.at(3));
		query.bindValue(":stain", tokenList.at(4));
		if (!query.exec())
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error inserting data into 'cytoband' table.\nReason: "
							+ query.lastError().text().toAscii()));
			emit messageChanged("");
			//Database::rollbackTransaction();
			file.close();
			return false;
		}

	}
	/* End transaction */
	//Database::endTransaction();

	file.close();

	return true;
}


/*
 * Inserts contig into DB
 */
bool Parser::insertContigIntoDB(const Contig *contig)
{
	QSqlQuery sqlQuery, sqlQuery2;

	/* Prepare sql query */
	sqlQuery.prepare("insert into contig "
			" (id, name, size, numberReads, readStartIndex, readEndIndex, "
			" seq, contigOrder, coverage, zoomLevels, maxFragRows, fileId) "
			" values "
			" (:id, :name, :size, :numberReads, :readStartIndex, "
			" :readEndIndex, :seq, :contigOrder, "
			" :coverage, :zoomLevels, :maxFragRows, :fileId)");
	sqlQuery2.prepare("insert into contigSeq "
			" (contigId, seq) "
			" values "
			" (:contigId, :seq)");

	//if (!Database::beginTransaction())
	//	return false;

	/* 'contig' table */
	sqlQuery.bindValue(":id", contig->id);
	sqlQuery.bindValue(":name", QString(contig->name));
	sqlQuery.bindValue(":size", contig->size);
	sqlQuery.bindValue(":numberReads", contig->numberReads);
	sqlQuery.bindValue(":readStartIndex", contig->readStartIndex);
	sqlQuery.bindValue(":readEndIndex", contig->readEndIndex);
	sqlQuery.bindValue(":seq", "");
	sqlQuery.bindValue(":contigOrder", contig->order);
	sqlQuery.bindValue(":coverage", contig->coverage);
	sqlQuery.bindValue(":zoomLevels", contig->zoomLevels);
	sqlQuery.bindValue(":maxFragRows", contig->maxFragRows);
	sqlQuery.bindValue(":fileId", contig->fileId);
	if (!sqlQuery.exec())
	{
		QMessageBox::critical(
			(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
			tr("Basejumper"),
			tr("Error inserting contig into DB.\nReason: "
					+ sqlQuery.lastError().text().toAscii()));
		//Database::rollbackTransaction();
		return false;
	}

	/* 'contigSeq' table */
	sqlQuery2.bindValue(":contigId", contig->id);
	sqlQuery2.bindValue(":seq", contig->seq);
	if (!sqlQuery2.exec())
	{
		QMessageBox::critical(
			(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
			tr("Basejumper"),
			tr("Error inserting contigSeq into DB.\nReason: "
					+ sqlQuery2.lastError().text().toAscii()));
		//Database::rollbackTransaction();
		return false;
	}

	//if (!Database::endTransaction())
	//	return false;

	return true;
}


/*
 * Inserts fragments into DB
 */
bool Parser::insertFragsIntoDB(const QList<Fragment *> &list)
{
	QSqlQuery query;
	Fragment *frag;
	int listSize, i;

	/* Prepare sql query */
	query.prepare(" insert into fragment "
			" (id, name, size, startPos, endPos, alignStart, alignEnd, "
			" qualStart, qualEnd, complement, seq, contig_id, "
			" yPos, numMappings) "
			" values "
			" (:id, :name, :size, :startPos, :endPos, :alignStart, "
			" :alignEnd, :qualStart, :qualEnd, :complement, "
			" :seq, :contigId, :yPos, :numMappings) ");

	/* Initialize */
	listSize = list.size();

	//if (!Database::beginTransaction())
	//{
	//	qDebug() << "insert frags begin transaction";
	//	return false;
	//}

	/* Execute the query for each fragment */
	for (i = 0; i < listSize; ++i)
	{
		frag = list.at(i);

		query.bindValue(":id", frag->id);
		query.bindValue(":name", QString(frag->name));
		query.bindValue(":size", frag->size);
		query.bindValue(":startPos", frag->startPos);
		query.bindValue(":endPos", frag->endPos);
		query.bindValue(":alignStart", frag->alignStart);
		query.bindValue(":alignEnd", frag->alignEnd);
		query.bindValue(":qualStart", frag->qualStart);
		query.bindValue(":qualEnd", frag->qualEnd);
		query.bindValue(":complement", frag->complement);
		query.bindValue(":seq", frag->seq);
		query.bindValue(":contigId", frag->contigNumber);
		query.bindValue(":yPos", frag->yPos);
		query.bindValue(":numMappings", frag->numMappings);
		if (!query.exec())
		{
			QMessageBox::critical(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Error inserting fragment into DB.\nReason: "
						+ query.lastError().text().toAscii()));
			//Database::rollbackTransaction();
			return false;
		}
	}

	//if (!Database::endTransaction())
	//{
	//	qDebug() << "insert frags end transaction";
	//	return false;
	//}

	return true;
}


/*
 * Given a hash (key: fragment name; value: number of occurences
 * in the current file), updates the numMappings column in
 * the fragment table in the DB.
 */
bool Parser::updateFragMapping(const QHash<QByteArray, int> &hash)
{
	QString connectionName = QString(this->metaObject()->className());
	{
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getFragDBName());
		QSqlQuery query(db);
		QByteArray name;
		QList<QByteArray> list;

		if (!db.transaction())
			return false;

		query.prepare("update fragment "
				" set numMappings = :numMappings "
				" where name = :name");

		list = hash.keys();
		foreach (name, list)
		{
			/* No need to update when value is 1 because by default
			 * numMappings is 1 */
			if (hash.value(name) == 1)
				continue;

			query.bindValue(":numMappings", hash.value(name));
			query.bindValue(":name", name);
			if (!query.exec())
			{
				QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Error inserting fragment into DB.\nReason: "
							+ query.lastError().text().toAscii()));
				db.rollback();
				return false;
			}
		}
		if (!db.commit())
			return false;
	}
	QSqlDatabase::removeDatabase(connectionName);

	return true;
}


/*
 * Inserts file name and associated contig IDs into the DB
 */
bool Parser::insertFileIntoDB(const QString &fileName, const int fileNum)
{
	QString str;
	QSqlQuery sqlQuery;

	/* Insert into 'file' table */
	str = "insert or ignore into file "
		" (id, file_name, filepath) "
		" values "
		" (" + QString::number(fileNum) + ", '"
		+ QFileInfo(fileName).fileName() + "', '"
		+ fileName + "')";
	if (!sqlQuery.exec(str))
	{
		QMessageBox::critical(
			(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
			tr("Basejumper"),
			tr("Error inserting file into DB.\nReason: "
					+ sqlQuery.lastError().text().toAscii()));
		return false;
	}

	return true;
}


/*
 * Inserts SNP positions into the DB
 */
bool Parser::insertSnpIntoDB(const Contig *contig,
		const QList<QList <Fragment *>*> &bin,
		const int maxPartitionSize)
{
	QTime startTime9, endTime9;
	startTime9 = QTime::currentTime();
	Fragment *frag;
	int j, k, l, pos;
	int numPartitions, contigId;
	int contigStart, contigEnd, fragStartIndex, fragEndIndex;
	int fragStart, fragEnd, offset, index, index2, contigSize, fragListSize;
	QVariantList posList, percentList, contigIdList;
	QSqlQuery query;
	QList <Fragment *>* fragList;
	QHash<int, int> snpHash, overlapHash;

		/* Initialize variables */
	contigId = contig->id;
	contigSize = contig->size;
	fragStartIndex = contig->readStartIndex;
	fragEndIndex = contig->readEndIndex;
	numPartitions = bin.size();
	contigEnd = 0;

	/* Prepare query */
	query.prepare("insert into snp_pos "
			" (contig_id, pos, variationPercent) "
			" values "
			" (:contigId, :pos, :variationPercent)");

	//if (!Database::beginTransaction())
	//	return false;

	for (l = 0; l < numPartitions; ++l)
	{
		fragList = bin.at(l);
		fragListSize = fragList->size();
		offset = maxPartitionSize * l;

		/* Calculate start and end position of the contig partition */
		contigStart = contigEnd + 1;
		if (contigStart > contigSize)
			contigStart = contigSize;
		contigEnd = contigStart + maxPartitionSize - 1;
		if (contigEnd > contigSize)
			contigEnd = contigSize;


		/* Foreach fragment that belongs to this contig partition,
		 * find the SNPs */
		for (j = 0; j < fragListSize; ++j)
		{
			frag = fragList->at(j);

			/* Calculate fragment start and end position */
			if (frag->startPos < contigStart)
			{
				fragStart = contigStart;
				index2 = fragStart - frag->startPos;
			}
			else
			{
				fragStart = frag->startPos;
				index2 = 0;
			}
			if (frag->endPos > contigEnd)
				fragEnd = contigEnd;
			else
				fragEnd = frag->endPos;

			/* Keep track of number of frags that have a different
			 * base (SNP), and total number of frags that overlap
			 * at that position */
			index = fragStart - offset - 1;
			for (k = fragStart - 1; k < fragEnd; ++k)
			{
				if (overlapHash.contains(index))
					overlapHash[index]++;
				else
					overlapHash[index] = 1;

				if (tolower(contig->seq.at(k))
						!= tolower(frag->seq.at(index2)))
				{
					if (snpHash.contains(index))
						snpHash[index]++;
					else
						snpHash[index] = 1;
				}
				index++;
				index2++;
			}
		}

		/* Store SNP data into DB in batch form */
		foreach (index, snpHash.keys())
		{
			pos = offset + index;
			contigIdList.append(QVariant(contigId));
			posList.append(QVariant(pos));
			percentList.append(QVariant((int) (((qreal) snpHash.value(index) /
					overlapHash.value(index)) * 100)));
		}
		query.bindValue(":contigId", contigIdList);
		query.bindValue(":pos", posList);
		query.bindValue(":percentList", percentList);
		if (!query.execBatch())
		{
			QMessageBox::critical(
				(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
				tr("Basejumper"),
				tr("Error inserting SNP positions into DB.\nReason: "
						+ query.lastError().text().toAscii()));
			//Database::rollbackTransaction();
			return false;
		}

		contigIdList.clear();
		posList.clear();
		percentList.clear();
		snpHash.clear();
		overlapHash.clear();

	}
	//if (!Database::endTransaction())
	//	return false;
	endTime9 = QTime::currentTime();
	//qDebug() << "insertSnpIntoDB = " << startTime9.msecsTo(endTime9) << endl;

	return true;
}


/*
 * Inserts SNPs into DB
 */
void Parser::insertSnpIntoDB(
		const int contigId,
		const QHash<int, int> &posVariationPercentHash)
{
	qDebug() << "contig id = " << contigId << ", hash size = " << posVariationPercentHash.size();
	QSqlQuery query;
	query.prepare("insert into snp_pos "
			" (contig_id, pos, variationPercent) "
			" values "
			" (:contigId, :pos, :variationPercent)");
	//Database::beginTransaction();
	query.bindValue(":contigId", contigId);
	QList<int> keys = posVariationPercentHash.keys();
	foreach (int key, keys)
	{
		query.bindValue(":pos", key);
		query.bindValue(":percentList", posVariationPercentHash.value(key));
		if (!query.exec())
		{
			QMessageBox::critical(
				QApplication::activeWindow(),
				tr("Basejumper"),
				tr("Error inserting SNP positions into DB.\nReason: "
						+ query.lastError().text().toAscii()));
			//Database::rollbackTransaction();
			return;
		}
	}
	//Database::endTransaction();
}


/*
 * Assign vertical position to each fragment.
 *
 * This algorithm compares genes that belong to the same list, thereby
 * localizing comparison.
 */
void Parser::assignFragYPos(QList<QList <Fragment *>*> &bin, int &maxY)
{
	//qDebug() << "start assign frag y pos" << endl;
	QTime startTime, endTime;
	startTime = QTime::currentTime();
	int j, key, maxKey, fragListSize;
	QList<Fragment *> list;
	QList <Fragment *> *fragList;
	QMultiHash<int, Fragment *> hash;
	Fragment *frag, *otherFrag;
	bool overlap;

	foreach (fragList, bin)
	{
		maxKey = 0;
		fragListSize = fragList->size();
		for (j = 0; j < fragListSize; ++j)
		{
			frag = fragList->value(j);

			/* If fragment already has a y-position, insert the
			 * fragment in the hash at the appropriate position */
			if (frag->yPos >= 0)
			{
				hash.insert(frag->yPos, frag);
				maxKey = (maxKey < frag->yPos)? frag->yPos: maxKey;
				continue;
			}
			/* If the hash is empty, insert fragment in the hash
			 * at the 0th key and assign 0 to the y-position of
			 * this fragment */
			else if (hash.isEmpty())
			{
				hash.insert(0, frag);
				frag->yPos = 0;
				continue;
			}

			/* At each y-position (or each key of the hash),
			 * check if there are any fragments that will overlap
			 * with the current fragment. */
			for (key = 0; key <= maxKey; ++key)
			{
				list = hash.values(key);
				overlap = false;

				/* Check if the given fragment overlaps with any other
				 * fragments at this y-position (or key of hash) */
				foreach (otherFrag, list)
				{
					if ((frag->startPos - FRAG_DESC_GAP) > otherFrag->endPos
							|| frag->endPos < (otherFrag->startPos - FRAG_DESC_GAP))
						continue;
					else
					{
						overlap = true;
						if (!hash.contains(key + 1))
						{
							hash.insert(key + 1, frag);
							frag->yPos = key + 1;
							maxKey = (maxKey < frag->yPos)? frag->yPos: maxKey;
						}
						break;
					}
				}

				/* If there is no overlap at this y-position,
				 * then assign this y-position to the fragment */
				if (!overlap)
				{
					hash.insert(key, frag);
					frag->yPos = key;
					break;
				}
				/* If a y-position has already been assigned to this
				 * fragment, then move-on to the next fragment */
				else if (frag->yPos >= 0)
					break;
			}
		}
		hash.clear();
		maxY = (maxY < maxKey)? maxKey: maxY;
	}
	endTime = QTime::currentTime();
	//qDebug() << "assignFragYPos() = " << startTime.msecsTo(endTime) << endl;
}


/*
 * Compares the given sets and return true if both the sets
 * contain the same elements, or return false if they
 * do not contain the same elements.
 */
bool Parser::compareSets(
		const QSet<QString> &set1,
		const int size1,
		const QSet<QString> &set2,
		const int size2)
{
	if (size1 != size2)
		return false;

	/* Checks if elements of set1 are in set2. It's
	 * sufficient to only check if the elements of one
	 * set are present in the other set. */
	foreach (QString val, set1.values())
	{
		if (!set2.contains(val))
			return false;
	}

	return true;
}


/**
 * Assigns vertical position to each gene.
 *
 * This algorithm compares genes that belong to the same list, thereby
 * localizing comparison.
 *
 * @param bin : Reference to a list of a list of genes
 * @param maxY : Reference to an int in which the maximum y-position
 * will be saved
 */
void Parser::assignGeneYPos(QList<QList <Gene *>*> &bin, int &maxY)
{
	int j, key, maxKey, geneListSize;
	QList<Gene *> list;
	QList <Gene *> *geneList;
	QMultiHash<int, Gene *> hash;
	Gene *gene, *otherGene;
	bool overlap;

	foreach (geneList, bin)
	{
		maxKey = 0;
		geneListSize = geneList->size();
		for (j = 0; j < geneListSize; ++j)
		{
			gene = geneList->value(j);

			/* If gene already has a y-position, insert the
			 * gene in the hash at the appropriate position */
			if (gene->yPos >= 0)
			{
				hash.insert(gene->yPos, gene);
				maxKey = (maxKey < gene->yPos)? gene->yPos: maxKey;
				continue;
			}
			/* If the hash is empty, insert gene in the hash
			 * at the 0th key and assign 0 to the y-position of
			 * this gene */
			else if (hash.isEmpty())
			{
				hash.insert(0, gene);
				gene->yPos = 0;
				continue;
			}

			/* At each y-position (or each key of the hash),
			 * check if there are any genes that will overlap
			 * with the current gene. */
			for (key = 0; key <= maxKey; ++key)
			{
				list = hash.values(key);
				overlap = false;

				/* Check if the given gene overlaps with any other
				 * genes at this y-position (or key of hash) */
				foreach (otherGene, list)
				{
					if (gene->startPos > otherGene->endPos
							|| gene->endPos < otherGene->startPos)
						continue;
					else
					{
						overlap = true;
						if (!hash.contains(key + 1))
						{
							hash.insert(key + 1, gene);
							gene->yPos = key + 1;
							maxKey = (maxKey < gene->yPos)? gene->yPos: maxKey;
						}
						break;
					}
				}

				/* If there is no overlap at this y-position,
				 * then assign this y-position to the gene */
				if (!overlap)
				{
					hash.insert(key, gene);
					gene->yPos = key;
					break;
				}
				/* If a y-position has already been assigned to this
				 * gene, then move-on to the next gene */
				else if (gene->yPos >= 0)
					break;
			}
		}
		hash.clear();
		maxY = (maxY < maxKey)? maxKey: maxY;
	}
}


/*
 * Assigns y-position to each fragment of the given contig
 */
void Parser::assignFragYPos(
		Contig *contig,
		const int avgFragSize,
		int &maxYPos)
{
	int numPartitions, partitionBegin, partitionEnd, i, j, lowestAvailableYPos;
	QList<QList <Fragment *> *> container;
	QList<Fragment *> *partition;
	QHash<int, Fragment*> occupiedPositionsHash;
	Fragment *frag, *tmp;
	QSqlQuery query;

	/* Create partitions */
	numPartitions = (int) ceil((qreal) contig->size / avgFragSize);
	for (i = 0; i < numPartitions; ++i)
		container.append(new QList<Fragment *>);

	/* Put fragments in appropriate partitions */
	foreach (frag, contig->getFragList())
	{
		partitionBegin = (int) ceil((qreal) (frag->startPos - FRAG_DESC_GAP) / avgFragSize);
		--partitionBegin;
		/* Take care of negative fragment positions */
		if (partitionBegin < 0)
			partitionBegin = 0;

		partitionEnd = (int) ceil((qreal) frag->endPos / avgFragSize);
		--partitionEnd;
		/* Take care of instances where fragment end position
		 * is greater than number of bases in contig */
		if (partitionEnd >= container.size())
			partitionEnd = container.size() - 1;

		if (partitionBegin == partitionEnd)
			container[partitionBegin]->append(frag);
		else
		{
			container[partitionBegin]->append(frag);
			for (j = partitionBegin + 1; j <= partitionEnd; ++j)
				container[j]->prepend(frag);
		}
	}


	/* For each fragment in each of the partitions,
	 * find a suitable y-position */
	foreach (partition, container)
	{
		lowestAvailableYPos = 0;

		foreach (Fragment *f, *partition)
		{
			if (f->yPos < 0)
			{
				while (occupiedPositionsHash.contains(lowestAvailableYPos))
					++lowestAvailableYPos;
				f->yPos = lowestAvailableYPos;
				occupiedPositionsHash[f->yPos] = f;
				++lowestAvailableYPos;
			}
			else if (f->yPos >= 0
					&& !occupiedPositionsHash.contains(f->yPos))
			{
				occupiedPositionsHash.insert(f->yPos, f);
				if (lowestAvailableYPos == f->yPos)
					++lowestAvailableYPos;
			}
			else if (f->yPos >= 0
					&& occupiedPositionsHash.contains(f->yPos)
					&& occupiedPositionsHash.value(f->yPos) != f)
			{
				tmp = occupiedPositionsHash.value(f->yPos);
				occupiedPositionsHash[f->yPos] = f;

				if (lowestAvailableYPos == f->yPos)
					++lowestAvailableYPos;
				tmp->yPos = lowestAvailableYPos;
				occupiedPositionsHash.insert(tmp->yPos, tmp);
				++lowestAvailableYPos;
			}
			maxYPos = (maxYPos < f->yPos)? f->yPos : maxYPos;
		}
		occupiedPositionsHash.clear();
	}

	/* Delete partitions */
	foreach (partition, container)
	{
		partition->clear();
		delete partition;
	}
	container.clear();
}


/*
 * Inserts SNP positions into the DB
 */
void Parser::insertSnpIntoDB(const Contig *contig, const int avgFragSize)
{
	int numPartitions, partitionBegin, partitionEnd, i, j, pos, variationPercent;
	int contigStart, contigEnd, fragStart, fragEnd, index;
	QList<QList <Fragment *> *> container;
	QList<Fragment *> *partition;
	Fragment *frag;
	QHash<int, int> posSnpHash;
	QHash<int, int> posOverlapHash;
	QVariantList posList, percentList, contigIdList;
	QSqlQuery query;
	QTime startTime, endTime;
	QTime startTime1, startTime2, startTime3, endTime1, endTime2, endTime3;
	QTime startTime4, endTime4, startTime5, endTime5;
	int sum2 = 0, sum3 = 0, sum4 = 0, sum5 = 0;

	/* Prepare query */
	query.prepare("insert into snp_pos "
			" (contig_id, pos, variationPercent) "
			" values "
			" (:contigId, :pos, :variationPercent)");
	query.bindValue(":contigId", contig->id);

	startTime = QTime::currentTime();
	/* Create partitions */
	numPartitions = (int) ceil((qreal) contig->size / avgFragSize);
	for (i = 0; i < numPartitions; ++i)
		container.append(new QList<Fragment *>);

	/* Put fragments in appropriate partitions */
	foreach (frag, contig->fragList->getList())
	{
		qDebug() << "here";
		partitionBegin = (int) ceil((qreal) frag->startPos / avgFragSize);
		--partitionBegin;
		/* Take care of negative fragment positions */
		if (partitionBegin < 0)
			partitionBegin = 0;

		partitionEnd = (int) ceil((qreal) frag->endPos / avgFragSize);
		--partitionEnd;
		/* Take care of instances where fragment end position
		 * is greater than number of bases in contig */
		if (partitionEnd >= container.size())
			partitionEnd = container.size() - 1;

		if (partitionBegin == partitionEnd)
			container[partitionBegin]->append(frag);
		else
		{
			container[partitionBegin]->append(frag);
			for (j = partitionBegin + 1; j <= partitionEnd; ++j)
				container[j]->prepend(frag);
		}
	}
	endTime = QTime::currentTime();
	//qDebug() << "partition creation = " << startTime.msecsTo(endTime);

	/* For each partition */
	startTime1 = QTime::currentTime();
	contigEnd = 0;
	for (i = 0; i < numPartitions; ++i)
	{
		partition = container.at(i);

		/* Calculate contig start and end position */
		contigStart = contigEnd + 1;
		if (contigStart > contig->size)
			contigStart = contig->size;
		contigEnd = contigStart + avgFragSize - 1;
		if (contigEnd > contig->size)
			contigEnd = contig->size;

		/* Initialize hashes for overlaps and SNPs */
		startTime2 = QTime::currentTime();

		/* For each fragment */
		foreach (frag, *partition)
		{
			startTime4 = QTime::currentTime();
			/* Calculate fragment start and end position */
			if (frag->startPos < contigStart)
			{
				fragStart = contigStart;
				index = fragStart - frag->startPos;
			}
			else
			{
				fragStart = frag->startPos;
				index = 0;
			}
			if (frag->endPos > contigEnd)
				fragEnd = contigEnd;
			else
				fragEnd = frag->endPos;
			endTime4 = QTime::currentTime();
			sum4 += startTime4.msecsTo(endTime4);

			/* Find SNPs by comparing each base of fragment to
			 * each base of contig */
			startTime5 = QTime::currentTime();
			for (j = fragStart-1; j < fragEnd; ++j)
			{
				if (posOverlapHash.contains(j))
					posOverlapHash[j]++;
				else
					posOverlapHash[j] = 1;

				if (tolower(contig->seq.at(j)) != tolower(frag->seq.at(index)))
				{
					if (posSnpHash.contains(j))
						posSnpHash[j]++;
					else
						posSnpHash[j] = 1;
				}
				index++;
			}
			endTime5 = QTime::currentTime();
			sum5 += startTime5.msecsTo(endTime5);
		}
		endTime2 = QTime::currentTime();
		sum2 += startTime2.msecsTo(endTime2);


		startTime3 = QTime::currentTime();
		QList<int> snpKeys = posSnpHash.keys();
		foreach (pos, snpKeys)
		{
			query.bindValue(":pos", pos);
			variationPercent = (int) ((qreal) posSnpHash.value(pos) / posOverlapHash.value(pos)) * 100;
			query.bindValue(":variationPercent", variationPercent);
			if (!query.exec())
			{
				QMessageBox::critical(
					(reinterpret_cast<QMainWindow *>(parent()))->centralWidget(),
					tr("Basejumper"),
					tr("Error inserting SNP positions into DB.\nReason: "
							+ query.lastError().text().toAscii()));
				return;
			}
		}
		posSnpHash.clear();
		posOverlapHash.clear();
		endTime3 = QTime::currentTime();
		sum3 += startTime3.msecsTo(endTime3);
	}
	foreach (partition, container)
		delete partition;

	endTime1 = QTime::currentTime();
	//qDebug() << "Time for finding all SNPs = " << startTime1.msecsTo(endTime1);
	//qDebug() << "Time for all partitions = " << sum2;
	//qDebug() << "Time for saving in DB = " << sum3;
	//qDebug() << "sum4 = " << sum4;
	//qDebug() << "sum5 = " << sum5;

}



















