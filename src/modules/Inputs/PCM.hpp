#include <Demuxer.hpp>

#include <IOController.hpp>

class Reader;

class PCM : public Demuxer
{
public:
	enum FORMAT { PCM_U8, PCM_S8, PCM_S16, PCM_S24, PCM_S32, PCM_FLT, FORMAT_COUNT };

	PCM(Module &);
private:
	bool set();

	QString name() const;
	QString title() const;
	double length() const;
	int bitrate() const;

	bool seek(int);
	bool read(Packet &, int &);
	void abort();

	bool open(const QString &);

	/**/

	IOController< Reader > reader;

	double len;
	FORMAT fmt;
	unsigned char chn;
	int srate, offset;
	bool bigEndian;
};

#define PCMName "PCM Audio"
