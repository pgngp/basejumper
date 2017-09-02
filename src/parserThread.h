
#ifndef PARSERTHREAD_H_
#define PARSERTHREAD_H_

#include <QThread>
#include <QHash>
#include <QSet>
#include <QStringList>

class ParserThread : public QThread
{
	Q_OBJECT

public:
	ParserThread();
	~ParserThread();
	inline void setFileList(const QStringList &fileList) { files = fileList; }

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

protected:
	void run();

private:
	QStringList files;
	QHash<QString, int> fileOrderHash;
	int numberOfContigs;
	bool isOrderFileLoaded;
	bool hasContigMismatch;
	QSet<QString> loadedContigsSet;

	bool readAce(const QStringList &files, const int filesSize);
};
#endif /* PARSERTHREAD_H_ */
