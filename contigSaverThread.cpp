
#include "contigSaverThread.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include "database.h"


/**
 * Constructor
 * @return
 */
ContigSaverThread::ContigSaverThread()
{

}


/**
 * Destructor
 * @return
 */
ContigSaverThread::~ContigSaverThread()
{

}


/**
 * Implements the run method
 */
void ContigSaverThread::run()
{
	//qDebug() << "ContigSaverThread begin***";
	QString connectionName = QString(this->metaObject()->className());
	{
		Contig *contig;
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getContigDBName());

		QSqlQuery sqlQuery(db);
		sqlQuery.prepare("insert into contig "
				" (id, name, size, numberReads, readStartIndex, readEndIndex, "
				" seq, contigOrder, coverage, zoomLevels, maxFragRows, fileId) "
				" values "
				" (:id, :name, :size, :numberReads, :readStartIndex, "
				" :readEndIndex, :seq, :contigOrder, "
				" :coverage, :zoomLevels, :maxFragRows, :fileId)");
		QSqlQuery sqlQuery2(db);
		sqlQuery2.prepare("insert into contigSeq "
				" (contigId, seq) "
				" values "
				" (:contigId, :seq)");
		QSqlQuery sqlQuery3(db);
		sqlQuery3.prepare("insert or ignore into file "
			" (id, file_name, filepath) "
			" values "
			" (:id, :fileName, :filePath)");

		if (!db.transaction())
		{
			qDebug() << "unable to open xaction in contigSaverThread";
			return;
		}

		forever
		{
			contigMutex.lock();
			if (contigQueue.isEmpty())
			{
				if (!moreContigs)
				{
					contigMutex.unlock();
					break;
				}
				else
				{
					contigQueueNotEmpty.wait(&contigMutex);
					if (contigQueue.isEmpty() && !moreContigs)
					{
						contigMutex.unlock();
						break;
					}
				}
			}
			contig = contigQueue.dequeue();
			contigQueueNotFull.wakeAll();
			contigMutex.unlock();

			/* 'file' table */
			sqlQuery3.bindValue(":id", contig->file->getId());
			sqlQuery3.bindValue(":fileName", contig->file->getName());
			sqlQuery3.bindValue(":filePath", contig->file->getPath());
			if (!sqlQuery3.exec())
			{
				qCritical() << "Error inserting file into DB in "
					<< this->metaObject()->className()
					<< ". Reason: "
					<< sqlQuery3.lastError().text();
				db.rollback();
				break;
			}

			/* 'contig' table */
			sqlQuery.bindValue(":id", contig->id);
			sqlQuery.bindValue(":name", contig->name);
			sqlQuery.bindValue(":size", contig->size);
			sqlQuery.bindValue(":numberReads", contig->numberReads);
			sqlQuery.bindValue(":readStartIndex", contig->readStartIndex);
			sqlQuery.bindValue(":readEndIndex", contig->readEndIndex);
			sqlQuery.bindValue(":seq", "");
			sqlQuery.bindValue(":contigOrder", contig->order);
			sqlQuery.bindValue(":coverage", contig->coverage);
			sqlQuery.bindValue(":zoomLevels", contig->zoomLevels);
			sqlQuery.bindValue(":maxFragRows", contig->maxFragRows);
			sqlQuery.bindValue(":fileId", contig->fileId);
			if (!sqlQuery.exec())
			{
				qCritical() << "Error inserting contig into DB in "
					<< this->metaObject()->className()
					<< ". Reason: "
					<< sqlQuery.lastError().text();
				db.rollback();
				break;
			}

			/* 'contigSeq' table */
			sqlQuery2.bindValue(":contigId", contig->id);
			sqlQuery2.bindValue(":seq", contig->seq);
			if (!sqlQuery2.exec())
			{
				qCritical() << "Error inserting contigSeq into DB in "
					<< this->metaObject()->className()
					<< ". Reason: "
					<< sqlQuery2.lastError().text();
				db.rollback();
				break;
			}
			delete contig;
		}
		if (!db.commit())
			qCritical() << "Error committing transaction in "
				<< this->metaObject()->className()
				<< ". Reason: "
				<< db.lastError().text();
	}
	QSqlDatabase::removeDatabase(connectionName);
	//qDebug() << "ContigSaverThread end***";
}

