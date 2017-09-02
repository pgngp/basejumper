
#include "annotation.h"


/**
 * Constructor
 */
Annotation::Annotation()
{
	id = 0;
	name = "";
	startPos = 0;
	endPos = 0;
}


/**
 * Constructor
 */
Annotation::Annotation(
		int id,
		const QString &name,
		int startPos,
		int endPos)
{
	this->id = id;
	this->name = name;
	this->startPos = startPos;
	this->endPos = endPos;
}


/**
 * Destructor
 */
Annotation::~Annotation()
{

}


void Annotation::setName(const QString &name)
{
	this->name = name;
}


QString Annotation::getName() const
{
	return name;
}


void Annotation::setStartPos(int startPos)
{
	this->startPos = startPos;
}


int Annotation::getStartPos() const
{
	return startPos;
}


void Annotation::setEndPos(int endPos)
{
	this->endPos = endPos;
}


int Annotation::getEndPos() const
{
	return endPos;
}


