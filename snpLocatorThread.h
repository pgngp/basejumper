
#ifndef SNPLOCATORTHREAD_H_
#define SNPLOCATORTHREAD_H_

#include <QThread>
#include "contig.h"
#include <QSqlDatabase>

class SnpLocatorThread : public QThread
{
	Q_OBJECT

public:
	void addContig(Contig *);

protected:
	void run();

private:
	QQueue<Contig *> contigQueue;

};

#endif /* SNPLOCATORTHREAD_H_ */
