
#include "snpLocatorThread.h"
#include "math.h"
//#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "database.h"

#define	PARTITION_SIZE	500

extern QWaitCondition contigSaved;
extern QWaitCondition snpSaved;
extern QWaitCondition dbAvailable;
extern QMutex mutex;
extern QReadWriteLock readWriteLock;
extern int numContigsSaved;
extern int numContigsSnpsSaved;
extern bool isDbAvailable;


/**
 * Adds the given contig to queue
 * @param contig : Pointer to the contig to be added to the queue
 */
void SnpLocatorThread::addContig(Contig *contig)
{
	QMutexLocker locker(&mutex);
	contigQueue.enqueue(contig);
	if (!isRunning())
		start();
}


/**
 * Implements the run method
 */
void SnpLocatorThread::run()
{
	Contig *contig;
	Fragment *frag;
	int j, k, partitionBegin, partitionEnd, pos;
	int numPartitions, contigId, variationPercent;
	int contigStart, contigEnd, fragStartIndex, fragEndIndex;
	int fragStart, fragEnd, offset, index, index2, contigSize, fragListSize;
	QList <Fragment *>* fragList;
	QHash<int, quint16> snpHash, overlapHash;
	QList<QList <Fragment *> *> container;

	QSqlDatabase db =
		Database::createConnection(
			QString(this->metaObject()->className()),
			Database::getSnpDBName());
	QSqlQuery query(db);
	query.prepare("insert into snp_pos "
			" (contig_id, pos, variationPercent) "
			" values "
			" (:contigId, :pos, :variationPercent)");

	forever
	{
		mutex.lock();
		if (contigQueue.isEmpty())
		{
			mutex.unlock();
			break;
		}
		//if (numContigsSnpsSaved >= numContigsSaved)
		//	contigSaved.wait(&mutex);
		contig = contigQueue.dequeue();
		mutex.unlock();

		/* Initialize variables */
		contigId = contig->id;
		contigSize = contig->size;
		fragStartIndex = contig->readStartIndex;
		fragEndIndex = contig->readEndIndex;
		contigEnd = 0;

		numPartitions = (int) ceil((qreal) contig->size / PARTITION_SIZE);
		for (int i = 0; i < numPartitions; ++i)
			container.append(new QList<Fragment *>);

		/* Put fragments in appropriate partitions */
		foreach (frag, contig->fragList->getList())
		{
			partitionBegin = (int) ceil((qreal) frag->startPos / PARTITION_SIZE);
			--partitionBegin;
			/* Take care of negative fragment positions */
			if (partitionBegin < 0)
				partitionBegin = 0;

			partitionEnd = (int) ceil((qreal) frag->endPos / PARTITION_SIZE);
			--partitionEnd;
			/* Take care of instances where fragment end position
			 * is greater than number of bases in contig */
			if (partitionEnd >= container.size())
				partitionEnd = container.size() - 1;

			if (partitionBegin == partitionEnd)
				container[partitionBegin]->append(frag);
			else
			{
				container[partitionBegin]->append(frag);
				for (j = partitionBegin + 1; j <= partitionEnd; ++j)
					container[j]->prepend(frag);
			}
		}

		mutex.lock();

		if (!db.transaction())
		{
			qDebug() << "unable to open xaction in snpLocatorThread";
			mutex.unlock();
			break;
		}

		mutex.unlock();

		/* For each partition, find SNPs */
		for (int l = 0; l < numPartitions; ++l)
		{
			fragList = container.at(l);
			fragListSize = fragList->size();
			offset = PARTITION_SIZE * l;

			/* Calculate start and end position of the contig partition */
			contigStart = contigEnd + 1;
			if (contigStart > contigSize)
				contigStart = contigSize;
			contigEnd = contigStart + PARTITION_SIZE - 1;
			if (contigEnd > contigSize)
				contigEnd = contigSize;


			/* Foreach fragment that belongs to this contig partition,
			 * find the SNPs */
			for (j = 0; j < fragListSize; ++j)
			{
				frag = fragList->at(j);

				/* Calculate fragment start and end position */
				if (frag->startPos < contigStart)
				{
					fragStart = contigStart;
					index2 = fragStart - frag->startPos;
				}
				else
				{
					fragStart = frag->startPos;
					index2 = 0;
				}
				if (frag->endPos > contigEnd)
					fragEnd = contigEnd;
				else
					fragEnd = frag->endPos;

				/* Keep track of number of frags that have a different
				 * base (SNP), and total number of frags that overlap
				 * at that position */
				index = fragStart - offset - 1;
				for (k = fragStart - 1; k < fragEnd; ++k)
				{
					if (overlapHash.contains(index))
						overlapHash[index]++;
					else
						overlapHash[index] = 1;

					if (tolower(contig->seq.at(k))
							!= tolower(frag->seq.at(index2)))
					{
						if (snpHash.contains(index))
							snpHash[index]++;
						else
							snpHash[index] = 1;

					}
					index++;
					index2++;
				}
			}

			foreach (int key, snpHash.keys())
			{
				pos = key + offset;
				variationPercent = ((float) snpHash.value(key) / overlapHash.value(key)) * 100;

				query.bindValue(":contigId", contigId);
				query.bindValue(":pos", pos);
				query.bindValue(":variationPercent", variationPercent);
				if (!query.exec())
				{
					qCritical() << "Error inserting SNP positions into DB in "
							<< this->metaObject()->className()
							<< ". Reason: "
							<< query.lastError().text();
					db.rollback();
					break;
				}
			}
			snpHash.clear();
			overlapHash.clear();
		} /* end: for each partition */


		if (!db.commit())
			break;

		mutex.lock();
		numContigsSnpsSaved++;
		snpSaved.wakeAll();
		mutex.unlock();

		foreach (QList<Fragment *> *partition, container)
			delete partition;
		container.clear();
	}

	db.close();

	//qDebug() << "snpLocatorThread end";

}
