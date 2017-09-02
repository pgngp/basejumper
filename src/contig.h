
#ifndef CONTIG_H_
#define CONTIG_H_

#include <QObject>
#include <QMap>
#include <QHash>
#include "annotationList.h"
#include "fragmentList.h"
#include "file.h"


class Contig : public QObject
{
	Q_OBJECT

public:
	Contig();
	~Contig();
	void getAnnotation();
	void getSnps(const int);
	void resetFrags();
	void resetAnnotationLists();
	static int getSize(const int);
	static int getSeq(const int, QString &);

	inline void setName(const QByteArray &name) { this->name = name; };
	inline QByteArray & getName() { return name; };

	inline void setSequence(const QByteArray &seq) { this->seq = seq; };
	inline QByteArray & getSequence() { return this->seq; };

	inline void setSize(const int size) { this->size = size; };
	inline int getSize() const { return this->size; };

	inline void setNumberReads(const int num) { this->numberReads = num; };
	inline int getNumberReads() const { return this->numberReads; };

	inline static int getNumContigs() { return numContigs; };
	inline static void setNumContigs(const int n) { numContigs = n; };

	inline static int getCurrentId() { return currentId; };
	inline static void setCurrentId(const int id) { currentId = id; };

	inline static void setTotalSize(const int size) { totalSize = size; };
	inline static int getTotalSize() { return totalSize; };

	inline static void setStartPos(const int pos) { startPos = pos; };
	inline static int getStartPos() { return startPos; };

	inline static void setEndPos(const int pos) { endPos = pos; };
	inline static int getEndPos() { return endPos; };

	inline QList<Fragment *> & getFragList() { return fragList->getList(); };
	inline void getFrags() { fragList->getFrags(); };

	inline File * getFile() { return file; };
	inline void setFile(File *f) { file = f; };

	int id;							/* Unique ID of this contig */
	QByteArray seq;					/* Contig sequence */
    QByteArray name;				/* Name of the contig */
    int size;						/* Number of sequences in this contig */
    int numberReads;				/* Number of reads that belong to this contig */
    int readStartIndex;				/* Starting index of reads that belong to this contig */
    int readEndIndex;				/* Ending index of reads that belong to this contig */
    QHash<int, quint8> snpPosHash;	/* Holds SNP positions of the contig */
    int order;						/* Holds the order of the contig */
    int fileId;						/* Holds the ID of the file that this contig belongs to */
    qreal coverage;					/* Average coverage of the contig */
    int maxFragRows;				/* Max number of fragment rows */
    int maxGeneRows;				/* Max number of gene rows */
    int zoomLevels;					/* Number of zoom levels */
    FragmentList *fragList;			/* Fragments belonging to this contig */
    QList<AnnotationList *> annotationLists;	/* List of annotation lists */
    File *file;						/* File this contig belongs to */

    static int numContigs;			/* Number of loaded contigs */
    static int currentId;			/* Id of the currently displayed contig */
    static int totalSize;			/* Total size of all the loaded contigs */
    static int startPos;			/* Holds the start position of the sequence */
    static int endPos;				/* Holds the end position of the sequence */
    static int midPos;				/* Holds the mid position of the sequence */
    static QMap<int, int> orderMap;	/* Maps contig order number => id */

	public slots:
	void reset();
	void getSeq();

	/** Resets/empties the sequence */
	inline void resetSeq() { seq = ""; };
	/** Resets Snp hash */
	inline void resetSnps() { snpPosHash.clear(); };

};

#endif /* CONTIG_H_ */
