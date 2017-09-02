
#include "fragmentSaverThread.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "math.h"
#include "database.h"

#define	FRAG_DESC_GAP	20
#define	PARTITION_SIZE	500

extern QQueue<Fragment *> fragQueue;
extern QMutex fragMutex;
extern QWaitCondition fragQueueNotFull;
extern QWaitCondition fragQueueNotEmpty;

extern QList<quint16> partitionList;
extern int contigPartitions;
extern QMutex partitionListMutex;
extern const int maxPartitionSize;
int firstPartition, lastPartition;
QList<int> commonLevelsList;



/**
 * Constructor
 * @return
 */
FragmentSaverThread::FragmentSaverThread()
	: connectionName(this->metaObject()->className())
{
	insertSqlString = "insert into fragment "
				" (id, name, size, startPos, endPos, alignStart, alignEnd, "
				" qualStart, qualEnd, complement, seq, contig_id, "
				" yPos, numMappings) "
				" values "
				" (:id, :name, :size, :startPos, :endPos, :alignStart, "
				" :alignEnd, :qualStart, :qualEnd, :complement, "
				" :seq, :contigId, :yPos, :numMappings)";
}


/**
 * Destructor
 * @return
 */
FragmentSaverThread::~FragmentSaverThread()
{

}


/**
 * Implements the run method
 */
void FragmentSaverThread::run()
{
	{
		Fragment *frag = NULL;
		int oldContigId = 0, newContigId = 0, partitionNum = 0;
		QHash<int, int> partition_fragCountHash;
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getFragDBName());
		QSqlQuery query(db);
		query.prepare(insertSqlString);

		if (!db.transaction())
		{
			qCritical() << "Error beginning transaction: "
				<< db.lastError().text();
			return;
		}

		forever
		{
			fragMutex.lock();
			if (fragQueue.isEmpty())
				fragQueueNotEmpty.wait(&fragMutex);
			while (!fragQueue.isEmpty())
				fragQueuePrivate.enqueue(fragQueue.dequeue());
			fragQueueNotFull.wakeAll();
			fragMutex.unlock();

			while (!fragQueuePrivate.isEmpty())
			{
				frag = fragQueuePrivate.dequeue();
				if (frag == NULL)
					break;

//				/* Clear the hash for fragments of new contig */
//				newContigId = frag->id;
//				if (oldContigId != newContigId)
//				{
//					partition_fragCountHash.clear();
//					oldContigId = newContigId;
//				}

				query.bindValue(":id", frag->id);
				query.bindValue(":name", frag->name);
				query.bindValue(":size", frag->size);
				query.bindValue(":startPos", frag->startPos);
				query.bindValue(":endPos", frag->endPos);
				query.bindValue(":alignStart", frag->alignStart);
				query.bindValue(":alignEnd", frag->alignEnd);
				query.bindValue(":qualStart", frag->qualStart);
				query.bindValue(":qualEnd", frag->qualEnd);
				query.bindValue(":complement", frag->complement);
				query.bindValue(":seq", frag->seq);
				query.bindValue(":contigId", frag->contigNumber);
				query.bindValue(":yPos", frag->yPos);
				query.bindValue(":numMappings", frag->numMappings);
				delete frag;
				if (!query.exec())
				{
					qCritical() << "Error inserting fragment into DB in "
						<< this->metaObject()->className()
						<< ". Reason: "
						<< query.lastError().text();
					db.rollback();
					return;
				}
			}

			if (frag == NULL)
				break;

		} /* end forever loop */

		if (!db.commit())
			qCritical() << "Error ending transaction: "
				<< db.lastError().text();

	} /* end block */
	QSqlDatabase::removeDatabase(connectionName);
}


void FragmentSaverThread::assignYPos(Fragment *frag)
{
//	firstPartition = frag->startPos / maxPartitionSize;
//	lastPartition = frag->endPos / maxPartitionSize;
//
//	partitionListMutex.lock();
//	for (int i = firstPartition; i <= lastPartition; ++i)
//	{
//
//	}
//	partitionListMutex.unlock();
}


///**
// * Implements the run method
// */
//void FragmentSaverThread::run()
//{
//	{
//		Fragment *frag;
//		QSqlDatabase db =
//			Database::createConnection(
//				connectionName,
//				Database::getFragDBName());
//		QSqlQuery query(db);
//		query.prepare(insertSqlString);
//		if (!db.transaction())
//		{
//			qCritical() << "Error beginning transaction: "
//				<< db.lastError().text();
//			return;
//		}
//
//		forever
//		{
//			fragMutex.lock();
//			if (!fragQueue.isEmpty())
//				frag = fragQueue.dequeue();
//			else
//			{
//				fragQueueNotEmpty.wait(&fragMutex);
//				fragMutex.unlock();
//				continue;
//			}
//			if (frag == NULL)
//			{
//				fragMutex.unlock();
//				break;
//			}
//			fragQueueNotFull.wakeAll();
//			fragMutex.unlock();
//
//			query.bindValue(":id", frag->id);
//			query.bindValue(":name", frag->name);
//			query.bindValue(":size", frag->size);
//			query.bindValue(":startPos", frag->startPos);
//			query.bindValue(":endPos", frag->endPos);
//			query.bindValue(":alignStart", frag->alignStart);
//			query.bindValue(":alignEnd", frag->alignEnd);
//			query.bindValue(":qualStart", frag->qualStart);
//			query.bindValue(":qualEnd", frag->qualEnd);
//			query.bindValue(":complement", frag->complement);
//			query.bindValue(":seq", frag->seq);
//			query.bindValue(":contigId", frag->contigNumber);
//			query.bindValue(":yPos", frag->yPos);
//			query.bindValue(":numMappings", frag->numMappings);
//			delete frag;
//			if (!query.exec())
//			{
//				qCritical() << "Error inserting fragment into DB in "
//					<< this->metaObject()->className()
//					<< ". Reason: "
//					<< query.lastError().text();
//				db.rollback();
//				break;
//			}
//
//		} /* end forever loop */
//
//		if (!db.commit())
//			qCritical() << "Error ending transaction: "
//				<< db.lastError().text();
//
//	} /* end block */
//	QSqlDatabase::removeDatabase(connectionName);
//}


/*
 * Assigns y-position to each fragment of the given contig
 */
void FragmentSaverThread::assignFragYPos(
		Contig *contig,
		const int avgFragSize,
		int &maxYPos)
{
	int numPartitions, partitionBegin, partitionEnd, i, j, lowestAvailableYPos;
	QList<QList <Fragment *> *> container;
	QList<Fragment *> *partition;
	QHash<int, Fragment*> occupiedPositionsHash;
	Fragment *frag, *tmp;
	QSqlQuery query;

	/* Create partitions */
	numPartitions = (int) ceil((qreal) contig->size / avgFragSize);
	for (i = 0; i < numPartitions; ++i)
		container.append(new QList<Fragment *>);

	/* Put fragments in appropriate partitions */
	foreach (frag, contig->getFragList())
	{
		partitionBegin = (int) ceil((qreal) (frag->startPos - FRAG_DESC_GAP) / avgFragSize);
		--partitionBegin;
		/* Take care of negative fragment positions */
		if (partitionBegin < 0)
			partitionBegin = 0;

		partitionEnd = (int) ceil((qreal) frag->endPos / avgFragSize);
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

	/* For each fragment in each of the partitions,
	 * find a suitable y-position */
	foreach (partition, container)
	{
		lowestAvailableYPos = 0;

		foreach (Fragment *f, *partition)
		{
			if (f->yPos < 0)
			{
				while (occupiedPositionsHash.contains(lowestAvailableYPos))
					++lowestAvailableYPos;
				f->yPos = lowestAvailableYPos;
				occupiedPositionsHash[f->yPos] = f;
				++lowestAvailableYPos;
			}
			else if (f->yPos >= 0
					&& !occupiedPositionsHash.contains(f->yPos))
			{
				occupiedPositionsHash.insert(f->yPos, f);
				if (lowestAvailableYPos == f->yPos)
					++lowestAvailableYPos;
			}
			else if (f->yPos >= 0
					&& occupiedPositionsHash.contains(f->yPos)
					&& occupiedPositionsHash.value(f->yPos) != f)
			{
				tmp = occupiedPositionsHash.value(f->yPos);
				occupiedPositionsHash[f->yPos] = f;

				if (lowestAvailableYPos == f->yPos)
					++lowestAvailableYPos;
				tmp->yPos = lowestAvailableYPos;
				occupiedPositionsHash.insert(tmp->yPos, tmp);
				++lowestAvailableYPos;
			}
			maxYPos = (maxYPos < f->yPos)? f->yPos : maxYPos;
		}
		occupiedPositionsHash.clear();
	}

	/* Delete partitions */
	foreach (partition, container)
	{
		partition->clear();
		delete partition;
	}
	container.clear();
}



