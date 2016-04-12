
#ifndef CONTIGSAVERTHREAD_H_
#define CONTIGSAVERTHREAD_H_

#include <QThread>
#include "contig.h"

extern QMutex contigMutex;
extern QQueue<Contig *> contigQueue;
extern bool moreContigs;
extern QWaitCondition contigQueueNotFull;
extern QWaitCondition contigQueueNotEmpty;

class ContigSaverThread : public QThread
{
	Q_OBJECT

public:
	ContigSaverThread();
	~ContigSaverThread();
//	void addContig(Contig *contig)
//	{
//		QMutexLocker locker(&mutex);
//		contigQueue.enqueue(contig);
//		if (!isRunning()) start();
//	}

protected:
	void run();

//private:
	//QQueue<Contig *> contigQueue;
};

#endif /* CONTIGSAVERTHREAD_H_ */
