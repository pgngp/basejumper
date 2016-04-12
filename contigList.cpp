
#include "contigList.h"
#include <QSqlQuery>
#include <QSqlError>
#include "database.h"

int ContigList::snpThreshold = 30;

/**
 * Constructor
 * @return
 */
ContigList::ContigList()
{
	currentContig = NULL;
	loadSequenceFlag = false;
}


/**
 * Destructor
 * @return
 */
ContigList::~ContigList()
{
	foreach (Contig *c, idContigMap.values())
		delete c;
	idContigMap.clear();
	orderIdMap.clear();
	idOrderMap.clear();
	currentContig = NULL;
}


/**
 * Resets data members
 */
void ContigList::reset()
{
	foreach (Contig *c, idContigMap.values())
		delete c;
	idContigMap.clear();
	orderIdMap.clear();
	idOrderMap.clear();
	loadSequenceFlag = false;
	currentContig = NULL;
}


/**
 * Returns a pointer to the contig with the given order
 */
Contig * ContigList::getContigUsingOrder(const int order)
{
	if (order <= 0) return currentContig;

	return getContigUsingId(orderIdMap.value(order));
}


/**
 * Returns a pointer to the contig with the given ID
 */
Contig * ContigList::getContigUsingId(const int id)
{
	if (id <= 0) return currentContig;

	/* Reset the sequence data member of the old contig */
	if (currentContig != NULL)
		currentContig->resetSeq();

	/* If contig is found in the map */
	if (idContigMap.contains(id))
	{
		currentContig = idContigMap.value(id);
		if (loadSequenceFlag == true)
			currentContig->getSeq();
		else
			currentContig->resetSeq();

		/* Load annotation if not already loaded */
		if (currentContig->annotationLists.size() == 0)
			currentContig->getAnnotation();
	}
	/* If contig is not found in the map, fetch it from
	 * the database and store it in the map */
	else
	{
		currentContig = getContig(id, loadSequenceFlag);
		idContigMap.insert(currentContig->id, currentContig);
	}

	return currentContig;
}


/*
 * Fetches contig from the database
 */
Contig * ContigList::getContig(const int id, bool fetchSeq)
{
	QString connectionName = QString(this->metaObject()->className());
	Contig *contig = NULL;
	{
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getContigDBName());
		QSqlQuery query(db);
		QString str;
		File *file;

		if (fetchSeq == false)
		{
			str = "select contig.id, contig.name, contig.size, "
					" contig.numberReads, contig.readStartIndex, "
					" contig.readEndIndex, contig.seq, contig.contigOrder, "
					" contig.coverage, contig.maxGeneRows, "
					" contig.zoomLevels, contig.maxFragRows, contig.fileId, "
					" file.file_name, file.filepath "
					" from contig, file "
					" where contig.fileId = file.id "
					" and contig.id = " + QString::number(id);
		}
		else
		{
			str = "select contig.id, contig.name, contig.size, contig.numberReads, "
					" contig.readStartIndex, contig.readEndIndex, "
					" contigSeq.seq, contig.contigOrder, contig.coverage, "
					" contig.maxGeneRows, contig.zoomLevels, "
					" contig.maxFragRows, contig.fileId, "
					" file.file_name, file.filepath "
					" from contig, contigSeq, file "
					" where contig.id = contigSeq.contigId "
					" and contig.fileId = file.id "
					" and contig.id = " + QString::number(id);
		}

		if (!query.exec(str))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error fetching data from 'contig' table.\nReason: "
							+ query.lastError().text().toAscii()));
			db.close();
			return NULL;
		}

		if (query.next())
		{
			contig = new Contig;
			contig->id = query.value(0).toInt();
			contig->name = query.value(1).toByteArray();
			contig->size = query.value(2).toInt();
			contig->numberReads = query.value(3).toInt();
			contig->readStartIndex = query.value(4).toInt();
			contig->readEndIndex = query.value(5).toInt();
			contig->seq = query.value(6).toByteArray();;
			contig->order = query.value(7).toInt();
			contig->coverage = query.value(8).toDouble();
			contig->maxGeneRows = query.value(9).toInt();
			contig->zoomLevels = query.value(10).toInt();
			contig->maxFragRows = query.value(11).toInt();
			contig->fileId = query.value(12).toInt();
			file = new File(
					contig->fileId,
					query.value(13).toString(),
					query.value(14).toString());
			contig->file = file;
			contig->getFrags();
			contig->getSnps(snpThreshold);
			contig->getAnnotation();
		}

		db.close();
	} /* end scope */
	QSqlDatabase::removeDatabase(connectionName);
	return contig;
}


/**
 * Sets loadSequenceFlag data member
 *
 * @param loadSeq : Indicates whether sequence should be loaded or not
 */
void ContigList::setLoadSequenceFlag(bool loadSeq)
{
	loadSequenceFlag = loadSeq;

	if (loadSequenceFlag == true)
	{
		currentContig->getSeq();
		currentContig->getSnps(snpThreshold);
	}
	else
	{
		currentContig->resetSeq();
		currentContig->resetSnps();
	}
}


/**
 * Maps contig order and ID
 */
void ContigList::mapOrderAndId()
{
	QString connectionName = QString(this->metaObject()->className());
	{
		QSqlDatabase db =
			Database::createConnection(
				QString(this->metaObject()->className()),
				Database::getContigDBName());
		QSqlQuery query(db);
		QString str;
		int id, order;

		str = "select id, contigOrder "
				" from contig ";
		if (!query.exec(str))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error fetching data from 'contig' table.\nReason: "
							+ query.lastError().text().toAscii()));
			db.close();
			return;
		}
		orderIdMap.clear();
		idOrderMap.clear();
		while (query.next())
		{
			id = query.value(0).toInt();
			order = query.value(1).toInt();
			orderIdMap.insert(order, id);
			idOrderMap.insert(id, order);
		}

		/* Update contig order for contigs stored in the cache */
		foreach (Contig *c, idContigMap.values())
			c->order = idOrderMap.value(c->id);

		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
}


/**
 * Sets SNP threshold value
 *
 * @param x : Value to which the threshold is to be set.
 */
void ContigList::setSnpThreshold(const int x)
{
	/* Flag invalid values */
	if (x < 0 || x > 100)
	{
		QMessageBox::critical(
				QApplication::activeWindow(),
				tr("Basejumper"),
				tr("Invalid SNP threshold value"));
		return;
	}
	snpThreshold = x;
	if (currentContig != NULL)
		currentContig->getSnps(snpThreshold);
};


