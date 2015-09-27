#include <FFDecHWAccel.hpp>

class FFDecVDPAU_NW : public FFDecHWAccel
{
public:
	FFDecVDPAU_NW( QMutex &, Module & );
private:
	~FFDecVDPAU_NW();

	bool set();

	QString name() const;

	int decode( Packet &encodedPacket, QByteArray &decoded, bool flush, unsigned );

	bool open( StreamInfo *, Writer * );
};
