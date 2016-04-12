
#include "fragment.h"


/**
 * Constructor
 */
Fragment::Fragment()
	: name(""), seq(""), complement('\0')
{
	id = 0;
    size = 0;
    startPos = 0;
    endPos = 0;
    alignStart = 0;
    alignEnd = 0;
    qualStart = 0;
    qualEnd = 0;
    contigNumber = 0;
    yPos = 0;
    numMappings = 1;
}


//Fragment::Fragment()
//{
//	id = 0;
//	name = "";
//	seq = "";
//    size = 0;
//    startPos = 0;
//    endPos = 0;
//    alignStart = 0;
//    alignEnd = 0;
//    qualStart = 0;
//    qualEnd = 0;
//    complement = '\0';
//    contigNumber = 0;
//    yPos = 0;
//    numMappings = 1;
//}



