#include <Demuxer.hpp>

class ToneGenerator : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS(ToneGenerator)
public:
	ToneGenerator(Module &);

	bool set();
private:
	bool metadataChanged() const;

	QString name() const;
	QString title() const;
	double length() const;
	int bitrate() const;

	bool dontUseBuffer() const;

	bool seek(int, bool);
	bool read(Packet &, int &);
	void abort();

	bool open(const QString &);

	/**/

	volatile bool aborted;
	mutable volatile bool metadata_changed;
	bool fromUrl;
	double pos;
	quint32 srate;
	QVector<quint32> freqs;
};

#define ToneGeneratorName "ToneGenerator"
