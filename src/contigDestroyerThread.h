
#ifndef CONTIGDESTROYERTHREAD_H_
#define CONTIGDESTROYERTHREAD_H_

#include <QThread>
#include "contig.h"

class ContigDestroyerThread : public QThread
{
	Q_OBJECT

public:
	void addContig(Contig *);

protected:
	void run();

private:
	QQueue<Contig *> contigQueue;
};

#endif /* CONTIGDESTROYERTHREAD_H_ */
