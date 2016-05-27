#include <FFDecHWAccel.hpp>

class VDPAUWriter;

class FFDecVDPAU : public FFDecHWAccel
{
public:
	FFDecVDPAU(QMutex &, Module &);
private:
	bool set();

	QString name() const;

	bool open(StreamInfo &, Writer *);
};
