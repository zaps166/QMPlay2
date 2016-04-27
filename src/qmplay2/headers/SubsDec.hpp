#ifndef SUBSDEC_HPP
#define SUBSDEC_HPP

#include <QStringList>

class LibASS;

class SubsDec
{
public:
	static SubsDec *create(const QString &);
	static QStringList extensions();

	virtual bool toASS(const QByteArray &, LibASS *, double) = 0;

	virtual ~SubsDec();
};

#endif //SUBSDEC_HPP
