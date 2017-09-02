
#ifndef ANNOTATION_H_
#define ANNOTATION_H_

#include <QObject>

class Annotation : public QObject
{
	Q_OBJECT

public:
	int id;
	QString name;
	int startPos;
	int endPos;

	Annotation();
	Annotation(const QString &, int, int, int, int);
	Annotation(int, const QString &, int, int, int, int);
	Annotation(int, const QString &, int, int);
	~Annotation();
	void setName(const QString &);
	QString getName() const;
	void setStartPos(int);
	int getStartPos() const;
	void setEndPos(int);
	int getEndPos() const;
};

#endif /* ANNOTATION_H_ */
