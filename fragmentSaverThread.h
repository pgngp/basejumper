
#ifndef FRAGMENTSAVERTHREAD_H_
#define FRAGMENTSAVERTHREAD_H_

#include <QThread>
#include "contig.h"
#include "fragment.h"

class FragmentSaverThread : public QThread
{
	Q_OBJECT

public:
	FragmentSaverThread();
	~FragmentSaverThread();

protected:
	void run();

private:
	QString connectionName;
	QString insertSqlString;
	QQueue<Fragment *> fragQueuePrivate;

	void assignFragYPos(Contig *, const int, int &);
	void assignYPos(Fragment *);
};
#endif /* FRAGMENTSAVERTHREAD_H_ */
