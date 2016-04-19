#ifndef CPU_HPP
#define CPU_HPP

#if defined(__i386) || defined(__i386__) || defined(_M_IX86)
	#define QMPLAY2_CPU_X86_32
	#define QMPLAY2_CPU_X86
#elif defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
	#define QMPLAY2_CPU_X86_64
	#define QMPLAY2_CPU_X86
#endif

#endif // CPU_HPP
