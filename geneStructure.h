
#ifndef GENESTRUCTURE_H_
#define GENESTRUCTURE_H_
#include <QObject>

class GeneStructure : public QObject
{
	Q_OBJECT

public:
	enum SubstructureType {EXON, INTRON, UTR5P, UTR3P};
	int id;
	QString name;
	enum SubstructureType type;
	int start;
	int end;

	GeneStructure(int, const QString &, SubstructureType, int, int);
	~GeneStructure();

};

#endif /* GENESTRUCTURE_H_ */
