
#ifndef GLOBALVIEWPAINTERTHREAD_H_
#define GLOBALVIEWPAINTERTHREAD_H_

#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QMutex>
#include <QHash>

struct ContigStruct
{
	int id;
	quint16 xStart;
	quint16 xEnd;
	quint16 yStart;
	quint16 yEnd;
	float stretchFactor;
};

class GlobalViewPainterThread : public QThread
{
	Q_OBJECT

public:
	GlobalViewPainterThread();
	~GlobalViewPainterThread();
	QImage getImage();
	inline void setWidth(const uint w) { width = w; };
	inline void setHeight(const uint h) { height = h; }
	QHash<int, int> & getLabelXPosHash();
	QHash<int, int> & getLabelYPosHash();

	signals:
	void contigDataGenerated(const QList<ContigStruct *> &);

protected:
	void run();

private:
	QMutex mutex;
	QImage image;
	quint16 width;
	quint16 height;
	QHash<int, int> contigOrderXPosHash;
	QHash<int, int> contigOrderYPosHash;
	QList<ContigStruct *> contigStructList;
};
#endif /* GLOBALVIEWPAINTERTHREAD_H_ */
