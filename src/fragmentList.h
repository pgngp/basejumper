
#ifndef FRAGMENTLIST_H_
#define FRAGMENTLIST_H_

#include "fragment.h"
#include <QObject>
#include <QtGui>

class Contig;

class FragmentList : public QObject
{
	Q_OBJECT

public:
	FragmentList(Contig *);
	~FragmentList();
	void resetList();
	void getFrags();

	inline int size() const { return list.size(); };
	inline Fragment * at(const int i) const { return list.at(i); };
	inline void append(Fragment *f) { list.append(f); };
	inline QList<Fragment *> & getList() { return list; };

private:
	Contig *contig;
	QList<Fragment *> list;
};

#endif /* FRAGMENTLIST_H_ */
