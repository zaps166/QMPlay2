#include <FFDecHWAccel.hpp>

class FFDecVDPAU_NW : public FFDecHWAccel
{
public:
	FFDecVDPAU_NW(QMutex &, Module &);
private:
	~FFDecVDPAU_NW();

	bool set();

	QString name() const;

	int decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &, bool flush, unsigned hurry_up);

	bool open(StreamInfo &, Writer *);
};
