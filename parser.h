
#ifndef PARSER_H_
#define PARSER_H_

#include <QWidget>
#include <QSqlDatabase>
#include <QMultiHash>
#include <QFile>
#include "contig.h"
#include "fragment.h"
#include "gene.h"
#include "maparea.h"
#include "stdio.h"
#include <QTextStream>
#include <QSet>
#include <QLinkedList>
#include "snpLocatorThread.h"
#include "fragmentSaverThread.h"
#include "contigSaverThread.h"
#include "contigDestroyerThread.h"
#include "parserThread.h"

class Parser : public QObject
{
	Q_OBJECT

public:
	Parser();
    ~Parser();
    bool readAce(const QStringList &, int);
    bool readBedFiles(const QStringList &, int, QString &);
    bool readOrderFile(const QString &);
    bool readCytoband(const QString &);
    bool readStructureFiles(
    		const QStringList &,
    		int,
    		QString &,
    		quint64,
    		int,
    		int);

    public slots:
    void totalSizeSignaled(int);
    void messageChangeSignaled(const QString &);
    void cleanWidgetsSignaled();
    void parsingStartedSignaled();
    void parsingProgressSignaled(int);
    void signalParsingFinished();

    signals:
    void messageChanged(const QString &);
    void cleanWidgets();
    void parsingStarted();
    void parsingFinished();
    void parsingFile(int);
    void parsingProgress(int);
    void fileSize(int);
    void totalSize(int);
    void parsedSize(int);
    void annotationLoaded();
    void orderParsingStarted();
    void orderParsingFinished();
    void annotationParsingFinished();
    void maxYPosChanged(const int);

private:
	void assignFragYPos(QList<QList <Fragment *>*> &, int &);
	void assignFragYPos(Contig *, const int, int &);
	void assignGeneYPos(QList<QList <Gene *>*> &, int &);
	bool updateFragMapping(const QHash<QByteArray, int> &);
	bool insertContigIntoDB(const Contig *);
	bool insertFragsIntoDB(const QList<Fragment *> &);
	bool insertFileIntoDB(const QString &, const int);
    bool insertSnpIntoDB(
    		const Contig *,
    		const QList<QList <Fragment *>*> &,
    		const int);
    void insertSnpIntoDB(const Contig *, const int);
    bool compareSets(
    		const QSet<QString> &,
    		const int,
    		const QSet<QString> &,
    		const int);

	QHash<QString, int> fileOrderHash;
	int numberOfContigs;
	bool isOrderFileLoaded;
	bool hasContigMismatch;
	QSet<QString> loadedContigsSet;
	ParserThread parserThread;
	FragmentSaverThread fragSaverThread;
	ContigSaverThread contigSaverThread;

private slots:
	void insertSnpIntoDB(const int, const QHash<int, int> &);
};

#endif /* PARSER_H_ */
