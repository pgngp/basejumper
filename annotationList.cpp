
#include "annotationList.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QtGui>
#include "contig.h"
#include "gene.h"

#define LINE_HEIGHT 		10
#define	POINT_SIZE_MIN		1.00
#define POINT_SIZE			8
#define TWICE_HEIGHT		20
#define FONT_FAMILY			"Sans Serif"
#define	PADDING				2
#define	DBL_PADDING			4
#define	EXON_TRACK			3
#define	INTRON_TRACK		4
#define	UTR5P_TRACK			5
#define	UTR3P_TRACK			6


/**
 * Constructor
 */
AnnotationList::AnnotationList()
{
	this->id = 0;
	this->contig = NULL;
	this->type = Custom;
	this->order = 0;
	this->alias = "";
}


/**
 * Constructor with initial value parameters
 */
AnnotationList::AnnotationList(
		const int id,
		Contig *contig,
		const Type type,
		const int order,
		const QString &alias)
{
	this->id = id;
	this->contig = contig;
	this->type = type;
	this->order = order;
	this->alias = alias;
	getAnnotation();
}


/**
 * Destructor
 */
AnnotationList::~AnnotationList()
{
	foreach (Annotation *a, list)
		delete a;
	list.clear();
	contig = NULL;
}


/*
 * Fetches annotation from the database
 */
void AnnotationList::getAnnotation()
{
	/* Fetch gene data from 'gene' table */
	if (type == Gene)
	{
		QSqlQuery query;
		QString str, name;
		int annotId, startPos, endPos, yPos, strand;

		/* Fetch genes */
		str = "select annotation.id, "
				" annotation.startPos, "
				" annotation.endPos, "
				" annotation.name,"
				" gene.yPos, "
				" gene.strand "
				" from annotation, gene "
				" where annotation.id = gene.id "
				" and annotation.contigId = " + QString::number(contig->id) + "";
		if (!query.exec(str))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error fetching data from 'annotation' table.\nReason: "
							+ query.lastError().text().toAscii()));
			return;
		}
		while (query.next())
		{
			annotId = query.value(0).toInt();
			startPos = query.value(1).toInt();
			endPos = query.value(2).toInt();
			name = query.value(3).toString();
			yPos = query.value(4).toInt();
			strand = query.value(5).toInt();
			list.append(new Gene::Gene(annotId, name, startPos, endPos, yPos, (enum Gene::Strand) strand));
		}

		/* Fetch max gene rows from 'contig' table */
		str = "select maxGeneRows "
				" from contig "
				" where id = " + QString::number(contig->id);
		if (!query.exec(str))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error fetching data from 'annotation' table.\nReason: "
							+ query.lastError().text().toAscii()));
			return;
		}
		if (query.next())
			contig->maxGeneRows = query.value(0).toInt();
	}
	/* Fetch annotation data from 'annotation' table */
	else
	{
		QSqlQuery query;
		QString str, name;
		int annotId, startPos, endPos;

		str = "select id, startPos, endPos, name "
				" from annotation "
				" where contigId = " + QString::number(contig->id) + ""
				" and annotationTypeId = " + QString::number(id) + "";
		if (!query.exec(str))
		{
			QMessageBox::critical(
					QApplication::activeWindow(),
					tr("Basejumper"),
					tr("Error fetching data from 'annotation' table.\nReason: "
							+ query.lastError().text().toAscii()));
			return;
		}
		while (query.next())
		{
			annotId = query.value(0).toInt();
			startPos = query.value(1).toInt();
			endPos = query.value(2).toInt();
			name = query.value(3).toString();
			list.append(new Annotation(annotId, name, startPos, endPos));
		}
	}
}


