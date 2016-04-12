
#include "contigDestroyerThread.h"

/* Global variables */
extern QWaitCondition contigSaved;
extern QWaitCondition snpSaved;
extern QWaitCondition fragSaved;
extern QMutex mutex;
extern QMutex destroyerMutex;
extern QReadWriteLock readWriteLock;
extern int numContigsSaved;
extern int numContigsFragsSaved;
extern int numContigsSnpsSaved;
extern int numContigsDestroyed;


/**
 * Adds the given contig to queue
 * @param contig : Pointer to the contig to be added to the queue
 */
void ContigDestroyerThread::addContig(Contig *contig)
{
	QMutexLocker locker(&destroyerMutex);
	contigQueue.enqueue(contig);
	if (!isRunning())
		start();
}


/**
 * Implements the run method
 */
void ContigDestroyerThread::run()
{
	Contig *contig;

	destroyerMutex.lock();
	forever
	{
		if (contigQueue.isEmpty())
			break;
		qDebug() << "deleting contig...begin";
		if (numContigsDestroyed >= numContigsSaved)
			contigSaved.wait(&destroyerMutex);
		if (numContigsDestroyed >= numContigsFragsSaved)
			fragSaved.wait(&destroyerMutex);
		//if (numContigsDestroyed >= numContigsSnpsSaved)
		//	snpSaved.wait(&mutex);
		contig = contigQueue.dequeue();
		qDebug() << "deleting contig " << contig->id;
		delete contig;
		numContigsDestroyed++;
	}
	destroyerMutex.unlock();
	qDebug() << "destroyer finished";
}

