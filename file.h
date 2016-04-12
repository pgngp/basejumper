
#ifndef FILE_H_
#define FILE_H_

#include <QObject>
#include <QFile>

class File : public QFile
{
	Q_OBJECT

public:
	File();
	File(const int, const QString &, const QString &);
	~File();
	static QString getName(const int);

	/** Returns/sets id */
	inline int getId() const { return id; };
	inline void setId(const int i) { id = i; };

	/** Returns/sets name */
	inline QString & getName() { return name; };
	inline void setName(const QString &n) { name = n; };

	/** Returns/sets path */
	inline QString & getPath() { return path; };
	inline void setPath(const QString &p) { path = p; };

private:
	int id;
	QString name;
	QString path;

};

#endif /* FILE_H_ */
