
#include "contig.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QtDebug>
#include <QVariant>
#include "search.h"
#include "gene.h"
#include "database.h"

int Contig::numContigs = 0;
int Contig::currentId = 0;
int Contig::totalSize = 0;
int Contig::startPos = 0;
int Contig::endPos = 0;
int Contig::midPos = 0;
QMap<int, int> Contig::orderMap;


/**
 * Constructor
 */
Contig::Contig()
	: name(""), seq("")
{
	id = 0;
	size = 0;
	numberReads = 0;
	readStartIndex = 0;
	readEndIndex = 0;
	order = 0;
	fileId = 0;
	coverage = 0.0;
	maxFragRows = 0;
	maxGeneRows = 0;
	zoomLevels = 0;
	fragList = new FragmentList(this);
	file = NULL;
}


//Contig::Contig()
//{
//	id = 0;
//	name = "";
//	seq = "";
//	size = 0;
//	numberReads = 0;
//	readStartIndex = 0;
//	readEndIndex = 0;
//	order = 0;
//	fileId = 0;
//	coverage = 0.0;
//	maxFragRows = 0;
//	maxGeneRows = 0;
//	zoomLevels = 0;
//	fragList = new FragmentList(this);
//	file = NULL;
//}


/**
 * Destructor
 */
Contig::~Contig()
{
	delete fragList;
	delete file;
	resetAnnotationLists();
}


/**
 * Resets the data members
 */
void Contig::reset()
{
	id = 0;
	name = "";
	seq = "";
	size = 0;
	numberReads = 0;
	readStartIndex = 0;
	readEndIndex = 0;
	order = 0;
	maxFragRows = 0;
	maxGeneRows = 0;
	zoomLevels = 0;
	file = NULL;
}


/**
 * Returns size given ID
 */
int Contig::getSize(const int id)
{
	int size;
	QString connectionName = "Contig_getSize";
	{
		QSqlDatabase db =
			Database::createConnection(connectionName, Database::getContigDBName());
		QSqlQuery query(db);
		QString str;

		str = "select size "
				" from contig "
				" where id = " + QString::number(id);
		if (!query.exec(str))
		{
			qDebug() << "Error fetching contig from DB.\nReason: "
				<< query.lastError().text();
			size = -1;
		}

		if (query.next())
			size = query.value(0).toInt();
		else
			size = -1;

		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
	return size;
}


/**
 * Returns sequence given ID
 */
int Contig::getSeq(const int id, QString &seq)
{
	QString connectionName = "Contig";

	{
		QSqlDatabase db =
			Database::createConnection(connectionName, Database::getContigDBName());
		QString queryStr;
		QSqlQuery query(db);

		queryStr = "select seq "
				" from contigSeq "
				" where contigId = " + QString::number(id);
		if (!query.exec(queryStr))
		{
			qDebug() << "Error fetching contig from DB in** "
				<< "Contig"
				<< ". Reason: "
				<< query.lastError().text();
			db.close();
			return -1;
		}
		if (query.next())
			seq = query.value(0).toString();

		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);

	return 1;
}


/**
 * Fetches contig sequence from 'contigSeq' table
 */
void Contig::getSeq()
{
	/* If sequence is already loaded, return */
	if (seq != "")
		return;

	QString connectionName = "Contig_getSeq";
	{
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getContigDBName());
		QString queryStr;
		QSqlQuery query(db);

		queryStr = "select seq "
				" from contigSeq "
				" where contigId = " + QString::number(id);
		if (!query.exec(queryStr))
		{
			qCritical() << "Error fetching contig from DB in "
				<< this->metaObject()->className()
				<< ". Reason: "
				<< query.lastError().text();
			db.close();
			return;
		}
		if (query.next())
			seq = query.value(0).toByteArray();

		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
}


/**
 * Delete all fragments in the list and clear the list
 */
void Contig::resetFrags()
{
	fragList->resetList();
}


/**
 * Delete all annotation lists in the list and clear the list
 */
void Contig::resetAnnotationLists()
{
	foreach (AnnotationList *a, annotationLists)
		delete a;
	annotationLists.clear();
}


/**
 * Fetches annotations
 */
void Contig::getAnnotation()
{
	QString connectionName = "Contig_getAnnotation";

	{
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getAnnotationDBName());
		QSqlQuery query(db);
		QString str, alias;
		int id, order, type;
		AnnotationList *annotList;

		resetAnnotationLists();

		str = "select id, type, annotOrder, alias "
				" from annotationType ";
		if (!query.exec(str))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error fetching data from 'annotationType' table.\nReason: "
							+ query.lastError().text().toAscii()));
			db.close();
			return;
		}
		while (query.next())
		{
			id = query.value(0).toInt();
			type = query.value(1).toInt();
			order = query.value(2).toInt();
			alias = query.value(3).toString();
			annotList = new AnnotationList(id, this, (enum AnnotationList::Type) type, order, alias);
			annotationLists.append(annotList);
		}
		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
}


/**
 * 	Fetches SNP positions
 */
void Contig::getSnps(const int threshold)
{
	QString connectionName = QString(this->metaObject()->className());
	{
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getSnpDBName());
		QSqlQuery query(db);
		QString str;

		str = " select pos "
				" from snp_pos "
				" where contig_id = " + QString::number(id) +
				" and variationPercent >= " + QString::number(threshold);
		if (!query.exec(str))
		{
			QMessageBox::critical(
				QApplication::activeWindow(),
				tr("Basejumper"),
				tr("Error fetching SNP positions from DB.\nReason: "
						+ query.lastError().text().toAscii()));
			db.close();
			return;
		}
		snpPosHash.clear(); // clear the hash before inserting new data
		while (query.next())
			snpPosHash.insert(query.value(0).toInt(), 1);
		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
}









