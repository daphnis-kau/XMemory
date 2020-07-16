#include <stdlib.h>
#include "CDebug.h"

#pragma warning(disable:4740)

#ifdef _ANDROID
#include "Android.h"
#endif

namespace XMemory
{
	CDebug& CDebug::Instance()
	{
		static CDebug s_Instance;
		return s_Instance;
	}

	CDebug::CDebug()
	{ 
#ifdef _WIN32
		m_nMoudleCount		= 0;
		m_nMoudleMaxCount	= 0;
		m_pModuleBase		= NULL;
		InitLock( m_Lock );
		SymInitialize( GetCurrentProcess(), NULL, FALSE );
#endif

#ifdef _ANDROID
		void* pHandle = dlopen( "libcorkscrew.so", RTLD_NOW );
		if( pHandle )
		{
			m_unwind_backtrace = (unwindFn)dlsym( pHandle, "unwind_backtrace" );  
			m_get_backtrace_symbols = (unwindSymbFn)dlsym( pHandle, "get_backtrace_symbols" );  
			m_free_backtrace_symbols = (unwindSymbFreeFn)dlsym( pHandle, "free_backtrace_symbols" );  
		}
		else
		{
			m_unwind_backtrace = NULL;  
			m_get_backtrace_symbols = NULL;  
			m_free_backtrace_symbols = NULL;  
		}
#endif
	}

	CDebug::~CDebug()
	{ 
#ifdef _WIN32
		SymCleanup( GetCurrentProcess() );
		DestroyLock( m_Lock );
#endif
	}

#ifdef _WIN32
	bool CDebug::AddModule( uint64_t nModuleBase )
	{
		for( size_t i = 0; i < m_nMoudleCount; i++ )
			if( m_pModuleBase[i] == nModuleBase )
				return false;
		if( m_nMoudleCount == m_nMoudleMaxCount )
		{
			m_nMoudleMaxCount += 32;
			uint64_t* pNewBuf = (uint64_t*)malloc( m_nMoudleMaxCount*sizeof(uint64_t) );
			memcpy( pNewBuf, m_pModuleBase, m_nMoudleCount*sizeof(uint64_t) );
			if( m_pModuleBase )
				free( m_pModuleBase );
			m_pModuleBase = pNewBuf;
		}

		if( m_pModuleBase )
			m_pModuleBase[m_nMoudleCount++] = nModuleBase;
		return true;
	}

	void CDebug::CheckAndLoadSymbol()
	{
		EnterLock( m_Lock );
		BOOL bRe = EnumerateLoadedModules64( GetCurrentProcess(), (PENUMLOADED_MODULES_CALLBACK64)&EnumModuleCallback, this );
		LeaveLock( m_Lock );
		if( !bRe )
		{
			char szError[64];
			sprintf_s( szError, "EnumerateLoadedModules64 failed with error code:%d", GetLastError() );
			throw( szError );
		}
	}

	BOOL CALLBACK CDebug::EnumModuleCallback( char* sModuleName, uint64_t nModuleBase, uint32_t, void* pArg )
	{
		if( static_cast<CDebug*>( pArg )->AddModule( nModuleBase ) )
			SymLoadModule64( GetCurrentProcess(), NULL, sModuleName, sModuleName, nModuleBase, NULL );

		return TRUE;
	}
#endif

	void CDebug::DebugAddress2Symbol( void* pAddress, char* szSymbolBuf, uint32_t nSize )
	{
#ifdef _WIN32
		char szBuffer[sizeof(SYMBOL_INFO)+1024];
		SYMBOL_INFO* pInfo		= reinterpret_cast<SYMBOL_INFO*>( szBuffer );
		pInfo->SizeOfStruct		= sizeof(SYMBOL_INFO);
		pInfo->MaxNameLen		= sizeof(szBuffer) - sizeof(SYMBOL_INFO);
		DWORD64 uDisplacement	= 0;

		if( !SymFromAddr( GetCurrentProcess(), (DWORD64)pAddress, &uDisplacement, pInfo ) )
		{
			CDebug::Instance().CheckAndLoadSymbol();
			if( !SymFromAddr( GetCurrentProcess(), (DWORD64)pAddress, &uDisplacement, pInfo ) )
				return;
		}

		strcpy_safe( szSymbolBuf, pInfo->Name, nSize );
#else
#ifndef _ANDROID
		char** arySymbol;
		arySymbol = backtrace_symbols( &pAddress, 1 );
		strcpy_safe( szSymbolBuf, *arySymbol, nSize );
		free( arySymbol );
#else
		if( m_get_backtrace_symbols == NULL || m_free_backtrace_symbols == NULL )
			return;
		backtrace_symbol_t symbols;  
		backtrace_frame_t frame = { (uintptr_t)pAddress, 0, 0 };
		m_get_backtrace_symbols( &frame, 1, &symbols );
		const char* symbolName = symbols.demangled_name ? symbols.demangled_name : symbols.symbol_name;  
		strcpy_safe( szSymbolBuf, symbolName, nSize );
		m_free_backtrace_symbols( &symbols, 1 ); 
#endif
#endif
	}

#ifdef _WIN32
	void CDebug::DebugGenMiniDump( EXCEPTION_POINTERS* pException, const wchar_t* szFileName, bool bFullMemDump )
	{
		HANDLE hReportFile = CreateFileW( szFileName, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, 0 );
		if( !hReportFile )
			return;

		_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

		ExInfo.ThreadId = ::GetCurrentThreadId();
		ExInfo.ExceptionPointers = pException;
		ExInfo.ClientPointers = NULL;

		BOOL bRet = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), hReportFile, bFullMemDump ? MiniDumpWithFullMemory : MiniDumpNormal , &ExInfo, NULL, NULL );
		if( !bRet )
		{  
			wchar_t lpMsgBuf[256];
			DWORD dw = GetLastError(); 

			FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), lpMsgBuf, 0, NULL );
			
			wchar_t szBuf[256] = { 0 };
			wcscat_s( szBuf, L"failed with error " );
			wcscat_s( szBuf, lpMsgBuf );
			MessageBoxW(NULL, szBuf, L"Error", MB_OK); 
		}
		CloseHandle( hReportFile );
	}
#endif

	size_t CDebug::GetStack( void** pStack, uint16_t nBegin, uint16_t nEnd, void* pMinStack )
	{
#ifdef _WIN32
		typedef USHORT (WINAPI *FunctionType)( __in ULONG, __in ULONG, __out PVOID*, __out_opt PULONG );
		static FunctionType funBackTrace = (FunctionType)( 
			GetProcAddress( LoadLibraryW( L"kernel32.dll" ), "RtlCaptureStackBackTrace" ) );
		// Then use 'func' as if it were CaptureStackBackTrace
		return funBackTrace ? funBackTrace( nBegin, nEnd - nBegin, pStack, NULL ) : 0;
#else
	#ifndef _ANDROID
		nBegin++;
		nEnd++;

		void* pTempStack[2048];
		::backtrace( pTempStack, 2048 );
		for( size_t i = nBegin; i < 2048 && i < nEnd; i++ )
		{
			if( pTempStack[i] == 0 )
				return i - nBegin;
			pStack[ i - nBegin ] = pTempStack[i];
		}
		return Min<size_t>( 2048, nEnd ) - nBegin;
	#else
		if( m_unwind_backtrace == NULL )
			return 0;

		void** pStackStart = (void**)( pMinStack ? pMinStack : &pStack );
		void** pMaxStack = CAndroidApp::GetInstance().GetMaxMainStack();
		uint32_t nMaxCount = CAndroidApp::GetInstance().GetMainStackSize()/(sizeof(void*));
		if( pStackStart > pMaxStack || pMaxStack - pStackStart > nMaxCount )
		{
			nMaxCount = 1024;
			pMaxStack = pStackStart + nMaxCount;
		}

		uint32_t nCount = 0;
		for( int i = 0; pStackStart < pMaxStack && nCount < nEnd; i++, pStackStart++ )
		{
			backtrace_symbol_t symbols;  
			backtrace_frame_t frame = { (uintptr_t)*pStackStart, 0, 0 };
			m_get_backtrace_symbols( &frame, 1, &symbols );
			const char* symbolName = symbols.demangled_name ? symbols.demangled_name : symbols.symbol_name;  
			if( symbolName == NULL || symbolName[0] == 0 || symbolName[0] == '_' || 
				( symbolName[0] >= 'a' && symbolName[0] <= 'z' ) || nCount++ < nBegin )
				continue;
			pStack[ nCount - nBegin - 1] = *pStackStart;
		}
		return nCount > nBegin ? nCount - nBegin : 0;

		//backtrace_frame_t StackFrame[1024];  
		//uint32_t nCount = m_unwind_backtrace( StackFrame, nBegin, nEnd );  
		//for( int i = 0; i < nCount; i++ )
		//	pStack[i] = (void*)(ptrdiff_t)StackFrame[i].absolute_pc;
		//return nCount;
	#endif // _ANDROID
#endif
	}

};
