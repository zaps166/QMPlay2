#include <QtGlobal>

#ifdef Q_OS_WIN
	#include <IPC_Windows.cpp>
#else
	#include <IPC_Unix.cpp>
#endif
