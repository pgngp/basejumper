
#include "geneOverlapExporter.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QApplication>
#include "contig.h"
#include <QMessageBox>
#include "annotationList.h"

/**
 * Constructor
 */
GeneOverlapExporter::GeneOverlapExporter()
{
	geneScope = Null;
}


/**
 * Destructor
 */
GeneOverlapExporter::~GeneOverlapExporter()
{

}


/**
 * Exports names of genes that overlap with fragments.
 *
 * The genes exported will depend on whether the user wants
 * to export all the genes present in the genome, or only
 * those genes that are present in the current contig,
 * or only those genes that are present in the Base View.
 */
void GeneOverlapExporter::exportGenes()
{
	QSqlQuery query;
	QString str;

	/* Create and execute the widget that will get the scope
	 * from the user */
	GeneOverlapExportDialog *dialog = new GeneOverlapExportDialog(
			QApplication::activeWindow(), Qt::Dialog);
	connect(dialog, SIGNAL(scope(int)), this, SLOT(setScope(int)));
	dialog->exec();
	delete dialog;

	/* If no scope has been set */
	if (geneScope == Null)
		return;
	/* If user wants to export all the genes in the genome */
	else if (geneScope == Genome)
	{
		str = "select annotation.name, fragment.name "
				" from annotationType, annotation, fragment "
				" where annotationType.id = annotation.annotationTypeId "
				" and annotation.contigId = fragment.contig_id "
				" and annotationType.type = " + QString::number((int) AnnotationList::Gene) +
				" and annotation.startPos <= fragment.endPos "
				" and annotation.endPos >= fragment.startPos ";
	}
	/* If the user wants to export only those genes that are
	 * present in the current contig */
	else if (geneScope == Contig)
	{
		/* Throw error if contig ID is not set */
		if (Contig::currentId == 0)
		{
			qCritical() << "Error: In GeneOverlapExporter::exportGenes(), "
					"contigId = 0. Cannot export names of overlapping genes.";
			return;
		}
		str = "select annotation.name, fragment.name "
				" from annotationType, annotation, fragment "
				" where annotationType.id = annotation.annotationTypeId "
				" and annotation.contigId = fragment.contig_id "
				" and annotationType.type = " + QString::number((int) AnnotationList::Gene) +
				" and annotation.contigId = " + QString::number(Contig::currentId) +
				" and annotation.startPos <= fragment.endPos "
				" and annotation.endPos >= fragment.startPos ";
	}
	/* If the user wants to export only those genes that are present
	 * in the Base View */
	else
	{
		/* Throw error if contig ID, start position, or end
		 * position are not set */
		if (Contig::currentId == 0
				|| Contig::startPos == 0
				|| Contig::endPos == 0)
		{
			qCritical() << "Error: In GeneOverlapExporter::exportGenes(), "
					"either contig Id = 0 and/or start position = 0 "
					"and/or end position = 0. Cannot export names of "
					"overlapping genes.";
			return;
		}

		str = "select annotation.name, fragment.name "
				" from annotationType, annotation, fragment "
				" where annotationType.id = annotation.annotationTypeId "
				" and annotation.contigId = fragment.contig_id "
				" and annotationType.type = " + QString::number((int) AnnotationList::Gene) +
				" and annotation.contigId = " + QString::number(Contig::currentId) +
				" and annotation.startPos >= " + QString::number(Contig::startPos) +
				" and annotation.endPos <= " + QString::number(Contig::endPos) +
				" and annotation.startPos <= fragment.endPos "
				" and annotation.endPos >= fragment.startPos ";
	}

	if (!query.exec(str))
	{
		qCritical() << "Error fetching from 'annotation' and 'fragment' tables. "
			<< query.lastError().text();
		return;
	}

	/* Open output file for writing data */
	QFile file("geneOverlaps.txt");
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		qCritical() << "Error opening file: " << file.fileName();
		qCritical() << file.errorString();
		return;
	}
	QTextStream out(&file);
	out << "Gene name\tRead name\n";

	/* Write data to output file */
	while (query.next())
	{
		out << query.value(0).toString() << "\t"
			<< query.value(1).toString() << "\n";
	}
	file.close();

	/* Display success message */
	QString text = "Gene names exported successfully to " + file.fileName() + ""
			" in the local directory.";
	QMessageBox::information(
			QApplication::activeWindow(),
			"Success!",
			text);

}


/**
 * Sets scope of the export
 *
 * @param s : Scope of export
 */
void GeneOverlapExporter::setScope(int s)
{
	geneScope = (enum Scope) s;
}
