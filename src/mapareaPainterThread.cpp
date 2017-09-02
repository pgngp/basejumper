
#include "mapareaPainterThread.h"

/**
 * Constructor
 * @return
 */
MapareaPainterThread::MapareaPainterThread()
{
	width = 0;
	height = 0;
	contig = NULL;
}


/**
 * Destructor
 * @return
 */
MapareaPainterThread::~MapareaPainterThread()
{

}


/**
 * Implements the run method
 */
void MapareaPainterThread::run()
{
	if (contig == NULL)
		return;


}


/**
 * Sets contig pointer to the given contig pointer
 * @param contig
 */
void MapareaPainterThread::setContig(Contig *contig)
{
	if (contig == NULL)
		return;

	QMutexLocker locker(&mutex);
	this->contig = contig;
}

