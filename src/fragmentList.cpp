
#include "fragmentList.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QtGui>
#include <cmath>
#include "contig.h"
#include "database.h"


/**
 * Constructor
 */
FragmentList::FragmentList(Contig *contig)
{
	this->contig = contig;
}


/**
 * Destructor
 */
FragmentList::~FragmentList()
{
	resetList();
	contig = NULL;
}


/**
 * Fetches fragments from the database
 */
void FragmentList::getFrags()
{
	QString connectionName = QString(this->metaObject()->className());
	{
		QSqlDatabase db =
			Database::createConnection(
				connectionName,
				Database::getFragDBName());
		QSqlQuery query(db);
		QString str;
		Fragment *frag;

		str = "select id, "
				" size, "
				" startPos, "
				" endPos, "
				" alignStart, "
				" alignEnd, "
				" qualStart, "
				" qualEnd, "
				" complement, "
				" seq, "
				" name, "
				" yPos, "
				" numMappings "
				" from fragment "
				" where contig_id = " + QString::number(contig->id);
	//			" and startPos < " + QString::number(contig->endPos) + ""
	//			" and endPos > " + QString::number(contig->startPos);
		if (!query.exec(str))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error fetching data from 'fragment' table.\nReason: "
							+ query.lastError().text().toAscii()));
			db.close();
			return;
		}
		while (query.next())
		{
			frag = new Fragment();
			frag->id = query.value(0).toInt();
			frag->size = query.value(1).toInt();
			frag->startPos = query.value(2).toInt();
			frag->endPos = query.value(3).toInt();
			frag->alignStart = query.value(4).toInt();
			frag->alignEnd = query.value(5).toInt();
			frag->qualStart = query.value(6).toInt();
			frag->qualEnd = query.value(7).toInt();
			frag->complement = query.value(8).toChar().toAscii();
			frag->seq = query.value(9).toByteArray();
			frag->name = query.value(10).toByteArray();
			frag->yPos = query.value(11).toInt();
			frag->numMappings = query.value(12).toInt();
			frag->contigNumber = contig->id;
			list.append(frag);
		}

		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
}


/**
 * Clears the list
 */
void FragmentList::resetList()
{
	foreach (Fragment *frag, list)
		delete frag;
	list.clear();
	//qDebug() << "resetList for contig " << contig->id;
}

