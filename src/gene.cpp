
#include "gene.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QtCore>

/**
 * Constructor
 */
Gene::Gene()
{

}


/**
 * Constructor
 */
Gene::Gene(int id,
		const QString &name,
		int startPos,
		int endPos,
		int yPos,
		Strand direction)
	: Annotation(id, name, startPos, endPos)
{
	this->yPos = yPos;
	this->direction = direction;


	int structureId, structureStart, structureEnd;
	GeneStructure::SubstructureType structureType;
	QString structureName, str;
	QSqlQuery query;

	str = "select id, name, start, end, type "
			" from geneStructure "
			" where geneId = " + QString::number(this->id);
	if (!query.exec(str))
	{
		qDebug() << "Error fetching geneStructure from DB.\nReason: " << query.lastError().text();
		return;
	}
	while (query.next())
	{
		structureId = query.value(0).toInt();
		structureName = query.value(1).toInt();
		structureStart = query.value(2).toInt();
		structureEnd = query.value(3).toInt();
		structureType = (enum GeneStructure::SubstructureType) query.value(4).toInt();
		substructures.append(new GeneStructure(
				structureId,
				structureName,
				structureType,
				structureStart,
				structureEnd));
	}
}


/**
 * Destructor
 */
Gene::~Gene()
{
	foreach (GeneStructure *gs, substructures)
		delete gs;
	substructures.clear();
}


/**
 * Adds substructure to this gene
 */
void Gene::addSubstructure(GeneStructure *gs)
{
	substructures.append(gs);
}


/**
 * Returns a pointer to a QVector containing gene substructures
 *
 * @return Returns a pointer to a vector containing gene substructures
 */
QVector<GeneStructure *> * Gene::getSubstructures()
{
	return &substructures;
}




