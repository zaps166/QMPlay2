#include <FFDecHWAccel.hpp>

class FFDecVAAPI : public FFDecHWAccel
{
public:
	FFDecVAAPI( QMutex &, Module & );
private:
	bool set();

	QString name() const;

	bool open( StreamInfo *, Writer * );
};
