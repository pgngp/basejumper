
#ifndef GENE_H_
#define GENE_H_

#include "annotation.h"
#include "geneStructure.h"
#include <QMap>
#include <QVector>

class Gene : public Annotation
{
	Q_OBJECT

public:
	enum Strand {Upstream, Downstream};

	int yPos;
	Strand direction;

	Gene();
	Gene(int, const QString &, int, int, int, Strand);
	~Gene();
	void addSubstructure(GeneStructure *);
	QVector<GeneStructure *> *getSubstructures();

private:
	QVector<GeneStructure *> substructures;
};
#endif /* GENE_H_ */
