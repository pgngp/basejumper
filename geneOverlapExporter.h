
#ifndef GENEOVERLAPEXPORTER_H_
#define GENEOVERLAPEXPORTER_H_
#include <QtCore>
#include "geneOverlapExportDialog.h"

class GeneOverlapExporter : public QObject
{
	Q_OBJECT

public:
	enum Scope {Null, Genome, Contig, BaseView};

	GeneOverlapExporter();
	~GeneOverlapExporter();

	public slots:
	void exportGenes();
	void setScope(int);

private:
	enum Scope geneScope;

};

#endif /* GENEOVERLAPEXPORTER_H_ */
