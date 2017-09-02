
#ifndef FRAGMENT_H_
#define FRAGMENT_H_

#include <QString>

class Fragment
{
public:
	Fragment();
	inline ~Fragment() {};

	inline void setId(const int id) { this->id = id; };
	inline int getId() const { return this->id; };

	inline void setName(const QByteArray &name) { this->name = name; };
	inline QByteArray getName() const { return this->name; };

	inline void setSequence(const QByteArray &s) { this->seq = s; };
	inline QByteArray getSequence() const {return this->seq; };

	inline void setSize(const quint16 s) { this->size = s; };
	inline quint16 getSize() const { return this->size; };

	inline void setStartPos(const int pos) { this->startPos = pos; };
	inline int getStartPos() const {return this->startPos; };

	inline void setAlignStart(const int s) { this->alignStart = s; };
	inline int getAlignStart() const { return this->alignStart; };

	inline void setAlignEnd(const int e) { this->alignEnd = e; };
	inline int getAlignEnd() const { return this->alignEnd; };

	inline void setQualStart(const int s) { this->qualStart = s; };
	inline int getQualStart() const { return this->qualStart; };

	inline void setQualEnd(const int e) { this->qualEnd = e; };
	inline int getQualEnd() const { return this->qualEnd; };

	inline void setComplement(const QChar c) { this->complement = c; };
	inline QChar getComplement() const { return this->complement; };

	inline void setContigNumber(const quint16 n) { this->contigNumber = n; };
	inline quint16 getContigNumber() const { return this->contigNumber; };

	int id;
    QByteArray name;
    QByteArray seq;
    quint16 size;
    int startPos;
    int endPos;
    int alignStart;
    int alignEnd;
    int qualStart;
    int qualEnd;
    QChar complement;
    quint16 contigNumber;
    qint16 yPos;
    quint16 numMappings;
};

#endif /* FRAGMENT_H_ */
