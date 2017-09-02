
#ifndef INTERMEDIATEVIEWPAINTERTHREAD_H_
#define INTERMEDIATEVIEWPAINTERTHREAD_H_

#include <QThread>
#include "contig.h"

namespace IVPainterThreadNS
{
	struct ContigStruct
	{
		quint16 xStart;
		quint16 xEnd;
	};
}

class IntermediateViewPainterThread : public QThread
{
	Q_OBJECT

public:
	IntermediateViewPainterThread();
	~IntermediateViewPainterThread();
	QImage getImage();
	inline void setWidth(const uint w) { width = w; };
	inline void setHeight(const uint h) { height = h; }
	QPoint &getLabelPos();

	public slots:
	void setContig(Contig *);

	signals:
	void contigDataGenerated(const IVPainterThreadNS::ContigStruct &);

protected:
	void run();

private:
	QMutex mutex;
	QImage image;
	quint16 width;
	quint16 height;
	Contig *contig;
	QPoint labelPos;
	IVPainterThreadNS::ContigStruct contigStruct;
};
#endif /* INTERMEDIATEVIEWPAINTERTHREAD_H_ */
