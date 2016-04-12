
#include "file.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QtGui>
#include "database.h"


/**
 * Constructor
 */
File::File()
{
	id = 0;
	name = "";
	path = "";
}


/**
 * Constructor with initial values
 */
File::File(const int id, const QString &name, const QString &path)
{
	this->id = id;
	this->name = name;
	this->path = path;
}


/**
 * Destructor
 */
File::~File()
{

}


/**
 * Returns the file name for the given file ID
 */
QString File::getName(const int id)
{
	QString name;
	QString connectionName = "File";
	{
		QSqlDatabase db =
			Database::createConnection(connectionName, Database::getContigDBName());
		QSqlQuery q(db);
		QString s;

		name = "";
		s = "select file_name "
				" from file "
				" where id = " + QString::number(id);
		if (!q.exec(s))
		{
			qCritical() << "Error fetching file name in "
					<< QObject::staticMetaObject.className()
					<< q.lastError().text();
			db.close();
			return name;
		}
		if (q.next())
			name = q.value(0).toString();
		db.close();
	}
	QSqlDatabase::removeDatabase(connectionName);
	return name;
}
