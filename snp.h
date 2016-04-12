
#ifndef SNP_H_
#define SNP_H_

#include <QObject>

class Snp : public QObject
{
	Q_OBJECT

public:
	Snp();
	~Snp();

	/** Returns threshold */
	static int getThreshold() { return threshold; };
	/** Sets threshold */
	static void setThreshold(const int x) { threshold = x; };

private:
	static int threshold;
};

#endif /* SNP_H_ */
