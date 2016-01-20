#include <Demuxer.hpp>

#include <IOController.hpp>

class Reader;

class Rayman2 : public Demuxer
{
public:
	Rayman2(Module &);
private:
	bool set();

	QString name() const;
	QString title() const;
	double length() const;
	int bitrate() const;

	bool seek(int, bool backward);
	bool read(Packet &, int &);
	void abort();

	bool open(const QString &);

	/**/

	void readHeader(const char *data);

	IOController< Reader > reader;

	double len;
	unsigned srate;
	unsigned short chn;
	int predictor[2];
	short stepIndex[2];
};

#define Rayman2Name "Rayman2 Audio"
