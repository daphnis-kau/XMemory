#pragma once

#include "LockHelp.h"

#ifndef _WIN32
	#ifndef _ANDROID
		#include <execinfo.h>  //for backtrace series function
	#else	
		#include <dlfcn.h>
		/*
		* Describes a single frame of a backtrace.
		*/
		typedef struct {
			uintptr_t absolute_pc;     /* absolute PC offset */
			uintptr_t stack_top;       /* top of stack for this frame */
			size_t stack_size;         /* size of this stack frame */
		} backtrace_frame_t;

		/*
		* Describes the symbols associated with a backtrace frame.
		*/
		typedef struct {
			uintptr_t relative_pc;       /* relative frame PC offset from the start of the library,
										 or the absolute PC if the library is unknown */
			uintptr_t relative_symbol_addr; /* relative offset of the symbol from the start of the
											library or 0 if the library is unknown */
			char* map_name;              /* executable or library name, or NULL if unknown */
			char* symbol_name;           /* symbol name, or NULL if unknown */
			char* demangled_name;        /* demangled symbol name, or NULL if unknown */
		} backtrace_symbol_t;

		typedef ssize_t (*unwindFn)( backtrace_frame_t*, size_t, size_t );  
		typedef void (*unwindSymbFn)( const backtrace_frame_t*, size_t, backtrace_symbol_t* );  
		typedef void (*unwindSymbFreeFn)( backtrace_symbol_t*, size_t );  
	#endif
#else
#define WIN32_LEAN_AND_MEAN		// 从 Windows 头中排除极少使用的资料
	#include <windows.h>
	#include <WinNT.h>
	#include <dbghelp.h>
	#pragma comment(linker, "/defaultlib:dbghelp.lib")
#endif

namespace XMemory
{
	class CMgrInstance;

	class CDebug
	{
		CDebug();
		~CDebug();

#ifdef _WIN32
		SLock				m_Lock;
		size_t				m_nMoudleCount;
		size_t				m_nMoudleMaxCount;
		uint64_t*			m_pModuleBase;
#elif defined _ANDROID
		unwindFn			m_unwind_backtrace;  
		unwindSymbFn		m_get_backtrace_symbols;  
		unwindSymbFreeFn	m_free_backtrace_symbols;  
#endif
		friend class CMgrInstance;
	public:
		static CDebug&	Instance();

#ifdef _WIN32
		static int _stdcall	EnumModuleCallback( char* sModuleName, uint64_t nModuleBase, uint32_t, void* pArg );
		void				CheckAndLoadSymbol();
		bool				AddModule( uint64_t nModuleBase );
		void				DebugGenMiniDump( _EXCEPTION_POINTERS* pException, const wchar_t* szFileName, bool bFullMemDump );
#endif
		void				DebugAddress2Symbol( void* pAddress, char* szSymbolBuf, uint32_t nSize );
		size_t				GetStack( void** pStack, uint16_t uBegin, uint16_t uEnd, void* pMinStack = NULL );
	};
}
