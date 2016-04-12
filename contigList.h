
#ifndef CONTIGLIST_H_
#define CONTIGLIST_H_

#include <QObject>
#include <QList>
#include "contig.h"

class ContigList : public QObject
{
	Q_OBJECT

public:
	static int snpThreshold;

	ContigList();
	~ContigList();
	Contig *getContigUsingOrder(const int);
	Contig *getContigUsingId(const int);
	inline int getSnpThreshold() { return snpThreshold; };

	public slots:
	void setLoadSequenceFlag(bool);
	void reset();
	void mapOrderAndId();
	void setSnpThreshold(const int);

private:
	Contig *currentContig;
	QMap<int, Contig *> idContigMap;	/* Maps contig ID to contig */
	QMap<int, int> orderIdMap;			/* Maps contig order to ID */
	QMap<int, int> idOrderMap;			/* Maps contig ID to order */
	bool loadSequenceFlag;				/* Flag that indicates whether contig sequence should be loaded */

	Contig * getContig(const int, bool);
};
#endif /* CONTIGLIST_H_ */
