
#ifndef MAPAREAPAINTERTHREAD_H_
#define MAPAREAPAINTERTHREAD_H_

#include <QThread>
#include "contig.h"

class MapareaPainterThread : public QThread
{
	Q_OBJECT

public:
	MapareaPainterThread();
	~MapareaPainterThread();

	public slots:
	void setContig(Contig *);

protected:
	void run();

private:
	QImage image;
	QMutex mutex;
	quint16 width;
	quint16 height;
	Contig *contig;

};
#endif /* MAPAREAPAINTERTHREAD_H_ */
