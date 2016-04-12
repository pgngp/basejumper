
#include "geneStructure.h"

/**
 * Constructor
 */
GeneStructure::GeneStructure(
		int id,
		const QString &name,
		SubstructureType type,
		int start,
		int end)
{
	this->id = id;
	this->name = name;
	this->type = type;
	this->start = start;
	this->end = end;
}


/**
 * Destructor
 */
GeneStructure::~GeneStructure()
{

}
