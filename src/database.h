
#ifndef DATABASE_H_
#define DATABASE_H_

#include <QObject>
#include <QSqlDatabase>

class Database : public QObject
{
	Q_OBJECT

public:
	enum Table {File, Chromosome, Chrom_contig, Contig, ContigSeq, Fragment,
		Cytoband, Bookmark, SearchQueries, AnnotationType, Annotation,
		Gene, GeneStructure, Snp_pos};
	Database();
	~Database();
    bool createConnection();
    void closeConnection();
    void createTables();
    bool beginTransaction();
    static bool beginTransaction(QSqlDatabase);
    bool rollbackTransaction();
    static bool rollbackTransaction(QSqlDatabase);
    bool endTransaction();
    static bool endTransaction(QSqlDatabase);
    static QSqlDatabase createConnection(const QString &, const QString &);

    inline static QString &getContigDBName() { return contigDBName; };
    inline static QString &getFragDBName() { return fragDBName; };
    inline static QString &getSnpDBName() { return snpDBName; };
    inline static QString &getAnnotationDBName() { return annotationDBName; };

    public slots:
    void deleteAll();

private:
	//QSqlDatabase contigDB;
	//QSqlDatabase fragDB;
	//QSqlDatabase snpDB;
	//QSqlDatabase annotationDB;
	static QString contigDBConnection;
	static QString fragDBConnection;
	static QString snpDBConnection;
	static QString annotDBConnection;
	static QString contigDBName;
	static QString fragDBName;
	static QString snpDBName;
	static QString annotationDBName;

	void deleteContig();
	void deleteFrag();
	void deleteSnp();
	void deleteAnnotation();
};


#endif /* DATABASE_H_ */
