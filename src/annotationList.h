#ifndef ANNOTATIONLIST_H_
#define ANNOTATIONLIST_H_
#include "annotation.h"
#include <QList>
#include <QObject>
#include <QtGui>

class Contig;

class AnnotationList : public QObject
{
	Q_OBJECT

public:
	enum Type {Custom, Gene, Snp};

	AnnotationList();
	AnnotationList(const int, const int, const Type, const int, const QString &);
	AnnotationList(const int, Contig *, const Type, const int, const QString &);
	~AnnotationList();

	inline int getId() const { return this->id; };
	inline void setId(const int id) { this->id = id; };

	inline AnnotationList::Type getType() const { return this->type; }
	inline void setType(const AnnotationList::Type type) { this->type = type; };

	inline QList<Annotation *>& getList() { return this->list; }
	inline void setList(const QList<Annotation *> &list) { this->list = list; };

	inline int getOrder() const { return this->order; };
	inline void setOrder(const int order) { this->order = order; };

	inline QString getAlias() const { return alias; }
	inline void setAlias(const QString &alias) { this->alias = alias; };


private:
	int id;
	QList<Annotation *> list;
	enum Type type;
	int order;
	QString alias;
	Contig *contig;

	void getAnnotation();
};

#endif /* ANNOTATIONLIST_H_ */
