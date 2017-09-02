
#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QtGui>
//#include <QSqlDatabase>

QString Database::contigDBConnection = "contigDBConnection";
QString Database::fragDBConnection = "fragDBConnection";
QString Database::snpDBConnection = "snpDBConnection";
QString Database::annotDBConnection = "annotDBConnection";
QString Database::contigDBName = "contigDB";
QString Database::fragDBName = "fragDB";
QString Database::snpDBName = "snpDB";
QString Database::annotationDBName = "annotationDB";


/**
 * Constructor
 */
Database::Database()
{

}


/**
 * Destructor
 */
Database::~Database()
{

}


/**
 * Creates a new DB connection
 */
bool Database::createConnection()
{
	createTables();
	return true;
}


QSqlDatabase Database::createConnection(
		const QString &connectionName,
		const QString &dbName)
{
	QSqlDatabase db;

	if (QSqlDatabase::contains(connectionName))
		db = QSqlDatabase::database(connectionName, true);
	else
	{
		db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
		db.setDatabaseName(dbName);
		db.setHostName("localhost");
		db.setUserName("");
		db.setPassword("");
		if (!db.open())
		{
			qCritical() << QObject::tr("Database Error. Reason: ")
				<< db.lastError().text();
		}
	}

	QSqlQuery query(db);
	if (!query.exec("PRAGMA synchronous=0;"))
		qCritical() << "Error issuing 'pragma synchronous' command in Database. Reason: "
			<< query.lastError().text();
	if (!query.exec("PRAGMA encoding='UTF-8';"))
		qCritical() << "Error issuing 'pragma encoding' command in Database. Reason: "
			<< query.lastError().text();
	if (!query.exec("PRAGMA journal_mode='TRUNCATE';"))
		qCritical() << "Error issuing 'pragma journal_mode' command in Database. Reason: "
			<< query.lastError().text();
//	if (!query.exec("PRAGMA cache_size=10000;"))
//		qCritical() << "Error issuing 'pragma cache_size' command in Database. Reason: "
//			<< query.lastError().text();
//	if (!query.exec("PRAGMA page_size=2048;"))
//		qCritical() << "Error issuing 'pragma page_size' command in Database. Reason: "
//			<< query.lastError().text();

	return db;
}


/**
 * Closes connection
 */
void Database::closeConnection()
{
	deleteContig();
	deleteFrag();
	deleteSnp();
	deleteAnnotation();
}


/*
 * Deletes Contig DB
 */
void Database::deleteContig()
{
	{
		QSqlDatabase db = createConnection(contigDBConnection, contigDBName);
		QSqlQuery query(db);
		query.exec("begin");
		query.exec("delete from contigSeq");
		query.exec("delete from chrom_contig");
		query.exec("delete from cytoband");
		query.exec("delete from chromosome");
		query.exec("delete from contig");
		query.exec("delete from file");
		query.exec("end");
		query.exec("vacuum");
	}
	QSqlDatabase::removeDatabase(contigDBConnection);
}


/*
 * Deletes Fragment DB
 */
void Database::deleteFrag()
{
	{
		QSqlDatabase db = createConnection(fragDBConnection, fragDBName);
		QSqlQuery query(db);
		query.exec("begin");
		query.exec("delete from fragment");
		query.exec("end");
		query.exec("vacuum");
	}
	QSqlDatabase::removeDatabase(fragDBConnection);
}


/*
 * Deletes SNP DB
 */
void Database::deleteSnp()
{
	{
		QSqlDatabase db = createConnection(snpDBConnection, snpDBName);
		QSqlQuery query(db);
		query.exec("begin");
		query.exec("delete from snp_pos");
		query.exec("end");
		query.exec("vacuum");
	}
	QSqlDatabase::removeDatabase(snpDBConnection);
}


/*
 * Deletes annotation DB
 */
void Database::deleteAnnotation()
{
	{
		QSqlDatabase db = createConnection(annotDBConnection, annotationDBName);
		QSqlQuery query(db);
		query.exec("begin");
		query.exec("delete from annotationType");
		query.exec("delete from geneStructure");
		query.exec("delete from gene");
		query.exec("delete from annotation");
		query.exec("end");
		query.exec("vacuum");
	}
	QSqlDatabase::removeDatabase(annotDBConnection);
}


/**
 * Deletes all rows of all the tables
 */
void Database::deleteAll()
{
    /* Show wait cursor */
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    deleteContig();
    deleteFrag();
    deleteSnp();
    deleteAnnotation();

    /* Restore the normal cursor */
    QApplication::restoreOverrideCursor();
}


/**
 * Creates tables
 */
void Database::createTables()
{
	{
		QString str;
		QSqlDatabase contigDB = createConnection(contigDBConnection, contigDBName);
		QSqlDatabase fragDB = createConnection(fragDBConnection, fragDBName);
		QSqlDatabase snpDB = createConnection(snpDBConnection, snpDBName);
		QSqlDatabase annotationDB = createConnection(annotDBConnection, annotationDBName);

		/* Create File table */
		QSqlQuery contigDBQuery(contigDB);
		str = "CREATE TABLE IF NOT EXISTS file "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" file_name VARCHAR(40) NOT NULL, "
				" filepath VARCHAR(200))";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating file table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		/* Create Contig table */
		str = "CREATE TABLE IF NOT EXISTS contig "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" name VARCHAR(30) NOT NULL, "
				" size INTEGER NOT NULL, "
				" numberReads INTEGER NOT NULL, "
				" readStartIndex INTEGER NOT NULL, "
				" readEndIndex INTEGER NOT NULL, "
				" seq TEXT NOT NULL, "
				" contigOrder INTEGER NOT NULL, "
				" coverage REAL NOT NULL,"
				" maxGeneRows INTEGER DEFAULT 0, "
				" zoomLevels INTEGER NOT NULL DEFAULT 0, "
				" maxFragRows INTEGER NOT NULL, "
				" fileId INTEGER NOT NULL "
				" REFERENCES file (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE)";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating contig table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		/* Create contigSeq table */
		str = "CREATE TABLE IF NOT EXISTS contigSeq "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" contigId INTEGER NOT NULL "
				" REFERENCES contig (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" seq TEXT NOT NULL )";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating contigSeq table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		/* Create Fragment table */
		QSqlQuery fragDBQuery(fragDB);
		str = "CREATE TABLE IF NOT EXISTS fragment "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" size INT NOT NULL, "
				" startPos INT NOT NULL, "
				" endPos INT NOT NULL, "
				" alignStart INT NOT NULL, "
				" alignEnd INT NOT NULL, "
				" qualStart INT, "
				" qualEnd INT, "
				" complement CHAR, "
				" seq TEXT NOT NULL, "
				" name VARCHAR(50) NOT NULL, "
				" contig_id INTEGER NOT NULL REFERENCES contig (id) "
				" ON UPDATE CASCADE ON DELETE RESTRICT, "
				" yPos INT NOT NULL, "
				" numMappings INT NOT NULL)";
		if (!fragDBQuery.exec(str))
		{
			qCritical() << "Error creating fragment table in the DB.";
			qCritical() << fragDBQuery.lastError().text();
		}

		/* Create SNP table */
		QSqlQuery snpDBQuery(snpDB);
		str = "CREATE TABLE IF NOT EXISTS snp_pos "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" contig_id INTEGER NOT NULL REFERENCES contig (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" pos INTEGER NOT NULL, "
				" variationPercent INTEGER NOT NULL)";
		if (!snpDBQuery.exec(str))
		{
			qCritical() << "Error creating snp_pos table in the DB.";
			qCritical() << snpDBQuery.lastError().text();
		}

		/* Create Annotation table */
		QSqlQuery annotationDBQuery(annotationDB);
		str = "CREATE TABLE IF NOT EXISTS annotation "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" contigId INTEGER NOT NULL REFERENCES contig (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" startPos INTEGER NOT NULL, "
				" endPos INTEGER NOT NULL, "
				" name VARCHAR(50) NOT NULL, "
				" annotationTypeId INTEGER NOT NULL REFERENCES annotationType (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE) ";
		if (!annotationDBQuery.exec(str))
		{
			qCritical() << "Error creating annotation table in the DB.";
			qCritical() << annotationDBQuery.lastError().text();
		}

		/* Create Gene table */
		str = "CREATE TABLE IF NOT EXISTS gene "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" geneId INTEGER NOT NULL REFERENCES annotation (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" yPos INTEGER NOT NULL, "
				" strand CHAR(1) NOT NULL) ";
		if (!annotationDBQuery.exec(str))
		{
			qCritical() << "Error creating gene table in the DB.";
			qCritical() << annotationDBQuery.lastError().text();
		}

		/* Create geneStructure table */
		str = "CREATE TABLE IF NOT EXISTS geneStructure "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" type INTEGER NOT NULL, "
				" geneId INTEGER NOT NULL REFERENCES annotation (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" name VARCHAR (100) NOT NULL, "
				" start INTEGER NOT NULL, "
				" end INTEGER NOT NULL)";
		if (!annotationDBQuery.exec(str))
		{
			qCritical() << "Error creating geneStructure table in the DB.";
			qCritical() << annotationDBQuery.lastError().text();
		}

		/* Create Chromosome table */
		str = "CREATE TABLE IF NOT EXISTS chromosome "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" name VARCHAR (20) NOT NULL UNIQUE ON CONFLICT IGNORE) ";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating chromosome table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		/* Create Chrom_contig table */
		str = "CREATE TABLE IF NOT EXISTS chrom_contig "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" chromId INTEGER NOT NULL REFERENCES chromosome (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" contigId INTEGER NOT NULL REFERENCES contig (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" chromStart INTEGER NOT NULL, "
				" chromEnd INTEGER NOT NULL) ";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating chrom_contig table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		/* Create 'annotationType' table */
		str = "CREATE TABLE IF NOT EXISTS annotationType "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" type INTEGER NOT NULL, "
				" annotOrder INTEGER NOT NULL, "
				" alias VARCHAR (50) NOT NULL, "
				" fileId INTEGER NOT NULL REFERENCES file (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE) ";
		if (!annotationDBQuery.exec(str))
		{
			qCritical() << "Error creating 'annotationType' table in the DB.";
			qCritical() << annotationDBQuery.lastError().text();
		}

		/* Create 'bookmark' table */
		str = "CREATE TABLE IF NOT EXISTS bookmark "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" name VARCHAR (30) NOT NULL UNIQUE, "
				" filepath VARCHAR (200) NOT NULL, "
				" contigName VARCHAR (30) NOT NULL, "
				" startPos INTEGER NOT NULL)";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating bookmark table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		/* Create 'searchQueries' table */
		str = "CREATE TABLE IF NOT EXISTS searchQueries "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" query VARCHAR (255) NOT NULL UNIQUE, "
				" count INTEGER NOT NULL DEFAULT 0, "
				" lastDatetime DATETIME NOT NULL ) ";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating searchQueries table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		/* Create 'cytoband' table */
		str = "CREATE TABLE IF NOT EXISTS cytoband "
				" (id INTEGER NOT NULL PRIMARY KEY, "
				" chromId INTEGER NOT NULL REFERENCES chromosome (id) "
				" ON DELETE RESTRICT ON UPDATE CASCADE, "
				" chromStart INTEGER NOT NULL, "
				" chromEnd INTEGER NOT NULL, "
				" name VARCHAR(50) NOT NULL, "
				" stain VARCHAR(50) NOT NULL)";
		if (!contigDBQuery.exec(str))
		{
			qCritical() << "Error creating cytoband table in the DB.";
			qCritical() << contigDBQuery.lastError().text();
		}

		contigDB.close();
		fragDB.close();
		annotationDB.close();
		snpDB.close();
	}
	QSqlDatabase::removeDatabase(contigDBConnection);
	QSqlDatabase::removeDatabase(fragDBConnection);
	QSqlDatabase::removeDatabase(snpDBConnection);
	QSqlDatabase::removeDatabase(annotDBConnection);
}


/**
 * Begins a transaction
 *
 * @return Returns true on success and false on failure
 */
bool Database::beginTransaction()
{
	QSqlQuery query;

	if (!query.exec("begin"))
	{
		qCritical() << "Error beginning a DB transaction.";
		qCritical() << query.lastError().text();
		return false;
	}

	return true;
}


/**
 * Begins a transaction in the given database
 * @param db : Database connection
 * @return : True on success, False on failure
 */
bool Database::beginTransaction(QSqlDatabase db)
{
	QSqlQuery query(db);

	if (!query.exec("begin immediate"))
	{
		qCritical() << "Error beginning a DB transaction.";
		qCritical() << query.lastError().text();
		return false;
	}

	return true;
}


/**
 * Rolls back a transaction
 *
 * @return Returns true on success and false on failure
 */
bool Database::rollbackTransaction()
{
	QSqlQuery query;

	if (!query.exec("rollback"))
	{
		qCritical() << "Error rolling back a DB transaction.";
		qCritical() << query.lastError().text();
		return false;
	}

	return true;
}


/**
 * Rollsback a transaction in the given database.
 * @param db : Database connection
 * @return : True on success, False on failure
 */
bool Database::rollbackTransaction(QSqlDatabase db)
{
	QSqlQuery query(db);

	if (!query.exec("rollback"))
	{
		qCritical() << "Error rolling back a DB transaction.";
		qCritical() << query.lastError().text();
		return false;
	}

	return true;
}


/**
 * End transaction
 *
 * @return Returns true on success and false on failure
 */
bool Database::endTransaction()
{
	QSqlQuery query;

	if (!query.exec("end"))
	{
		qCritical() << "Error ending a DB transaction.";
		qCritical() << query.lastError().text();
		return false;
	}

	return true;
}


/**
 * End transaction in the given database
 * @param db : Database connection
 * @return : True on success, False on failure
 */
bool Database::endTransaction(QSqlDatabase db)
{
	QSqlQuery query(db);

	if (!query.exec("end"))
	{
		qCritical() << "Error ending a DB transaction.";
		qCritical() << query.lastError().text();
		return false;
	}

	return true;
}
