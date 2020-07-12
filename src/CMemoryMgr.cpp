#include "CMemoryMgr.h"
#include "CDebug.h"
#include "Misc.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

extern "C"
{
#ifdef CHECK_MEMORY_ERROR
	void* __wrap_malloc( size_t nSize ) 
	{
		return XMemory::CMemoryMgr::Instance().AllocFromSystem( nSize );
	}

	void __wrap_free( void* p ) 
	{
		XMemory::CMemoryMgr::Instance().FreeToSystem( p, 0 );
	}
#endif
};

namespace XMemory	
{
#ifdef CHECK_MEMORY_BOUND
	class CMemoryBlock : public TList<CMemoryBlock>::CListNode
	{ public: size_t m_nBlockSize; };
#else
	class CMemoryBlock { public: size_t m_nBlockSize; };
#endif

	void* CMemoryMgr::AllocFromSystem( size_t nSize )
	{
#ifdef CHECK_MEMORY_ERROR
		nSize += sizeof(CMemoryBlock);
		size_t nAllocSize = XMemory::AligenUp( nSize, m_nPageSize );
		EnterLock( m_hGlobalMemoryLock );
		CMemoryBlock* pMem = (CMemoryBlock*)m_pAddrStart;
		m_pAddrStart += nAllocSize + m_nPageSize*2;
		LeaveLock( m_hGlobalMemoryLock );
		uint32_t nFlag = VIRTUAL_PAGE_READ|VIRTUAL_PAGE_WRITE;
		if( !XMemory::CommitMemoryPage( pMem, nAllocSize, nFlag ) )
			return NULL;
		pMem->m_nBlockSize = nSize;

	#ifdef CHECK_MEMORY_BOUND
		memset( pMem + 1, 0xcd, nAllocSize - sizeof(CMemoryBlock) );
		EnterLock( m_hGlobalMemoryLock );
		PushBack( *pMem );
		LeaveLock( m_hGlobalMemoryLock );
	#endif

		return pMem + 1;
#else		
		size_t nPageSize = GetVirtualPageSize();
		size_t nAllocSize = AligenUp( nSize, nPageSize );
		void* pMem = ReserveMemoryPage( NULL, nAllocSize );
		if( !pMem )
			return NULL;
		uint32_t nFlag = VIRTUAL_PAGE_READ|VIRTUAL_PAGE_WRITE;
		if( !CommitMemoryPage( pMem, nAllocSize, nFlag ) )
			return NULL;
		return pMem;
#endif
	}

	void CMemoryMgr::FreeToSystem( void* pMemory, size_t nSize )
	{
#ifdef CHECK_MEMORY_ERROR
		CMemoryBlock* pMem = ( (CMemoryBlock*)pMemory ) - 1;

	#ifdef CHECK_MEMORY_BOUND
		EnterLock( m_hGlobalMemoryLock );
		CMemoryBlock* pAlloc = GetFirst();
		while( pAlloc )
		{
			size_t nSize = pAlloc->m_nBlockSize;
			size_t nAllocSize = XMemory::AligenUp( nSize, m_nPageSize );
			uint8_t* pStart = (uint8_t*)pAlloc;
			for( size_t i = nSize; i < nAllocSize; i++ )
				if( pStart[i] != 0xcd )
					throw( "Error" );
			pAlloc = pAlloc->GetNext();
		}

		pMem->Remove();
		LeaveLock( m_hGlobalMemoryLock );
	#endif

		nSize = pMem->m_nBlockSize;
		size_t nAllocSize = XMemory::AligenUp( nSize, m_nPageSize );
		XMemory::DecommitMemoryPage( pMem, nAllocSize );
		XMemory::FreeMemoryPage( pMem, nAllocSize );
#else				
		size_t nPageSize = GetVirtualPageSize();
		size_t nAllocSize = AligenUp( nSize, nPageSize );
		DecommitMemoryPage( pMemory, nAllocSize );
		FreeMemoryPage( pMemory, nAllocSize );
#endif
	}

	CMemoryMgr& CMemoryMgr::Instance()
	{
		static CMemoryMgr s_Instance;
		return s_Instance;
	}

#ifdef ENABLE_MEMORY_MGR
	//===================================================
	// 打开内存管理
	//===================================================
	
	CMemoryMgr::CMemoryMgr(void)
		: m_bEnable( true )
#ifdef CHECK_MEMORY_ERROR
		, m_nPageSize( GetVirtualPageSize() )
		, m_nCurPage( 0 )
		, m_pAddrStart( 0 )
#endif

#ifdef MEMORY_LEASK
		, m_nAllocInfoSize( 0 )
		, m_nAllocInfoMaxSize( 0 )
		, m_nFreeCount( 0 )
#endif
	{
#ifdef CHECK_MEMORY_ERROR
		size_t nSize = 0x3fffffff;
		if( sizeof(size_t) > 4 )
			nSize = 0xffffffffffLL;
		m_pAddrStart = (uint8_t*)ReserveMemoryPage( NULL, nSize );
#endif
		for( uint32_t i = 0, n = 1; i < eMemoryConst_AllocateCount; i++ )
		{
			while( n*eMemoryConst_Unit <= m_MemoryUnitInfo.m_aryClassSize[i] )
			{
				uint32_t nSize = n++ * eMemoryConst_Unit;
				m_aryClassIndex[( nSize - 1 ) / eMemoryConst_Unit] = (uint8_t)i;
			}
		}
		
		memset( m_aryMemFreeList, 0, sizeof(m_aryMemFreeList) );
		memset( m_nFixManageSize, 0, sizeof(m_nFixManageSize) );
		memset( m_nFixAllocSize, 0, sizeof(m_nFixAllocSize) );
		m_nBigAllocSize = 0;

		for( uint16_t i = 0; i < eMemoryConst_AllocateCount; ++i )
			InitLock( m_hFixMemoryLock[i] );

#ifdef CHECK_MEMORY_ERROR
		InitLock( m_hGlobalMemoryLock );
#endif

#ifdef MEMORY_LEASK
		InitLock( m_hAllMemory );
#endif
	}

	CMemoryMgr::~CMemoryMgr(void)
	{
#ifdef MEMORY_LEASK
		if( m_nAllocInfoSize )
			free( m_aryAllocInfo );
		DestroyLock( m_hAllMemory );
#endif

		// 注意：
		// CMemoryMgr::Free函数会被vc的stl中某些清理函数在CMemoryMgr析构后调用
		// 因此锁销毁后必须置空，否则会在CMemoryMgr::Free被锁死，导致进程无法退出
		for ( uint16_t i = 0; i < eMemoryConst_AllocateCount; ++i )
			DestroyLock( m_hFixMemoryLock[ i ] );
#ifdef CHECK_MEMORY_ERROR
		DestroyLock( m_hGlobalMemoryLock );
#endif
		m_bEnable = false;
	}

	void* CMemoryMgr::Allocate( size_t nSize, void* pCallAddress )
	{
#ifdef CHECK_MEMORY_ERROR
		return AllocFromSystem( nSize );
#endif

		size_t nNeedSize = nSize;
		if( nSize < sizeof(void*) )
			nSize = sizeof(void*);
		nSize += sizeof(SMemHead);

		// 不应该分配过大的内存
		if( nSize >= 0x7fffffff )
			return NULL;

		SMemHead* pMemAlloc = NULL;
		if ( nSize > eMemoryConst_MaxSize )
		{
			pMemAlloc = (SMemHead*)AllocFromSystem( nSize );
			m_nBigAllocSize += nSize;
			pMemAlloc->m_nBlockSize = (uint32_t)nSize;
		}
		else 
		{
			uint8_t nIdx = m_aryClassIndex[ ( ( nSize - 1 )/eMemoryConst_Unit) ];
			nSize = m_MemoryUnitInfo.m_aryClassSize[ nIdx ];

			if ( !m_bEnable )
				return NULL;
			EnterLock( m_hFixMemoryLock[nIdx] );
			pMemAlloc = m_aryMemFreeList[nIdx];

			if( !pMemAlloc )
			{
				char* pBuffer = (char*)AllocFromSystem( eMemoryConst_PageSize );
				m_nFixManageSize[nIdx] += eMemoryConst_PageSize;

				for( size_t i = 0; i < eMemoryConst_PageSize/nSize; i++ )
				{
					pMemAlloc = (SMemHead*)( pBuffer + i*nSize );
					*(SMemHead**)( pMemAlloc + 1 ) = m_aryMemFreeList[nIdx];
					m_aryMemFreeList[nIdx] = pMemAlloc;
				}
			}
			
			m_aryMemFreeList[nIdx] = *(SMemHead**)( pMemAlloc + 1 );
			m_nFixAllocSize[nIdx] += nSize;

			assert( nNeedSize + sizeof(SMemHead) <= nSize );
			size_t nEmptySize = nSize - ( sizeof(SMemHead) + nNeedSize );
			size_t nPreClassSize = nIdx ? m_MemoryUnitInfo.m_aryClassSize[nIdx - 1] : 0;
			size_t nMaxEmptySize = nSize - nPreClassSize;
			assert( nEmptySize <= nMaxEmptySize );

			LeaveLock( m_hFixMemoryLock[ nIdx ] );
			pMemAlloc->m_MgrHead.m_nIndex = nIdx;
			pMemAlloc->m_MgrHead.m_nFlag  = eMemMgr_Use;
			pMemAlloc->m_MgrHead.m_nEmpty = (uint16_t)nEmptySize;
		}

#ifdef MEMORY_LEASK
		//记录堆栈
		EnterLock( m_hAllMemory );
		uint32_t nBegin, nEnd;
		SAllocInfo Info;
		memset( &Info, 0, sizeof(SAllocInfo) );
		if( ALLOC_STACK == 1 )
			Info.m_pAllocAddress[0] = pCallAddress;
		else
			CDebug::Instance().GetStack( Info.m_pAllocAddress, 4, 19 );

		if( !GetBound( m_aryAllocInfo, m_nAllocInfoSize, Info, nBegin, nEnd, SAllocInfoCompare() ) )
			pMemAlloc->m_pAllocInfo = InsertAllocInfo( 0, Info.m_pAllocAddress, nSize );
		else if( !memcmp( m_aryAllocInfo[nBegin]->m_pAllocAddress, Info.m_pAllocAddress, sizeof(Info.m_pAllocAddress) ) )
			( pMemAlloc->m_pAllocInfo = m_aryAllocInfo[nBegin] )->m_nAllocSize += nSize;
		else
			pMemAlloc->m_pAllocInfo = InsertAllocInfo( nEnd, Info.m_pAllocAddress, nSize );
		LeaveLock( m_hAllMemory );
#endif

		return pMemAlloc + 1;
	}

	void CMemoryMgr::Free( void* p )
	{
		if ( p == NULL )
			return;

#ifdef CHECK_MEMORY_ERROR
		FreeToSystem( p, 0 );
		return;
#endif

		SMemHead* pMemAlloc = ( (SMemHead*)p ) - 1;

#ifdef MEMORY_LEASK
		SAllocInfo* pCallInfo = pMemAlloc->m_pAllocInfo;
#endif

		uint32_t nSize;
		if( ( pMemAlloc->m_MgrHead.m_nFlag&eMemMgr_Use ) == 0 )
		{
			nSize = pMemAlloc->m_nBlockSize;
			m_nBigAllocSize -= nSize;
			FreeToSystem( pMemAlloc, nSize );
		}
		else 
		{
			if( pMemAlloc->m_MgrHead.m_nFlag != eMemMgr_Use || 
				pMemAlloc->m_MgrHead.m_nIndex >= eMemoryConst_AllocateCount )
				throw( "Free Invalid Memory!!!!" );

			uint8_t nIdx = pMemAlloc->m_MgrHead.m_nIndex;
			nSize = m_MemoryUnitInfo.m_aryClassSize[nIdx];
			size_t nPreClassSize = nIdx ? m_MemoryUnitInfo.m_aryClassSize[nIdx - 1] : 0;
			size_t nMaxEmptySize = nSize - nPreClassSize;
			if( pMemAlloc->m_MgrHead.m_nEmpty > nMaxEmptySize )
				throw( "Free Invalid Memory!!!!" );

			if ( !m_bEnable )
				return;
			
			EnterLock( m_hFixMemoryLock[ nIdx ] );
			*(SMemHead**)( pMemAlloc + 1 ) = m_aryMemFreeList[nIdx];
			m_aryMemFreeList[nIdx] = pMemAlloc;
			m_nFixAllocSize[nIdx] -= nSize;
			LeaveLock( m_hFixMemoryLock[ nIdx ]  );
		}

#ifdef MEMORY_LEASK
		EnterLock( m_hAllMemory );		
		assert( pCallInfo->m_nAllocSize >= nSize );
		pCallInfo->m_nAllocSize -= nSize;
		LeaveLock( m_hAllMemory );
#endif
	}

#ifdef MEMORY_LEASK
	CMemoryMgr::SAllocInfo* CMemoryMgr::InsertAllocInfo( uint32_t nIndex, 
		void* pAllocAddress[ALLOC_STACK], size_t nAllocSize )
	{
		if( m_nFreeCount == 0 )
		{
			m_pFreeInfo = (SAllocInfo*)malloc( 4000 );
			m_nFreeCount = 4000/sizeof(SAllocInfo);
		}

		SAllocInfo* pInfo = m_pFreeInfo++;
		m_nFreeCount--;
		memcpy( pInfo->m_pAllocAddress, pAllocAddress, sizeof(void*)*ALLOC_STACK );
		pInfo->m_nAllocSize = nAllocSize;

		if( m_nAllocInfoMaxSize == m_nAllocInfoSize )
		{
			m_nAllocInfoMaxSize = ( m_nAllocInfoMaxSize + 1 )*2;
			SAllocInfo** aryAllocInfo = (SAllocInfo**)malloc( sizeof(SAllocInfo*)*m_nAllocInfoMaxSize );
			memcpy( aryAllocInfo, m_aryAllocInfo, sizeof(SAllocInfo*)*nIndex );
			memcpy( aryAllocInfo + nIndex + 1, m_aryAllocInfo + nIndex, sizeof(SAllocInfo*)*( m_nAllocInfoSize - nIndex ) );
			free( m_aryAllocInfo );
			m_aryAllocInfo = aryAllocInfo;
			m_nAllocInfoSize++;
		}
		else
		{
			for( uint32_t i = m_nAllocInfoSize; i > nIndex; i-- )
				m_aryAllocInfo[i] = m_aryAllocInfo[i - 1];
			m_nAllocInfoSize++;
		}

		m_aryAllocInfo[nIndex] = pInfo;
		return pInfo;
	}

	uint32_t CMemoryMgr::GetAllocStack( uint32_t nIndex, void**& pAddress, size_t& nAllocSize )
	{
		if( nIndex >= m_nAllocInfoSize )
			return 0;
		pAddress = m_aryAllocInfo[nIndex]->m_pAllocAddress;
		nAllocSize = m_aryAllocInfo[nIndex]->m_nAllocSize;
		return ALLOC_STACK;
	}
#else
	uint32_t CMemoryMgr::GetAllocStack( uint32_t nIndex, void**& pAddress, uint32_t& nAllocSize )
	{
		return 0;
	}
#endif

	uint64_t CMemoryMgr::GetTotalMgrSize()
	{
		uint64_t nSize = 0;
		for( uint32_t i = 0; i < eMemoryConst_AllocateCount; i++ )
			nSize += m_nFixManageSize[i];
		return nSize;
	}

	uint64_t CMemoryMgr::GetFreeMgrSize()
	{
		uint64_t nSize = 0;
		for( uint32_t i = 0; i < eMemoryConst_AllocateCount; i++ )
			nSize += m_nFixManageSize[i] - m_nFixAllocSize[i];
		return nSize;
	}

	uint64_t CMemoryMgr::GetTotalAlloc()
	{
		uint64_t nTotalCount = 0;
		for( uint32_t i = 0; i < eMemoryConst_AllocateCount; i++ )
			nTotalCount += m_nFixAllocSize[i];
		nTotalCount += m_nBigAllocSize;
		return nTotalCount;
	}

	void CMemoryMgr::DumpMemoryInfo()
	{
#ifdef MEMORY_LEASK
		EnterLock( m_hAllMemory );
		uint32_t nCount = m_nAllocInfoSize;
		SAllocInfo** aryAllocInfo = (SAllocInfo**)alloca( sizeof(SAllocInfo*)*nCount );
		memcpy( aryAllocInfo, m_aryAllocInfo, sizeof(SAllocInfo*)*nCount );
		LeaveLock( m_hAllMemory );

		FILE* fpM = fopen( "memory.txt", "w" );
		FILE* fpS = fopen( "stack.txt", "w" );
		for( uint32_t i = 0; fpM && fpS && i < nCount; i++ )
		{
			if( aryAllocInfo[i]->m_nAllocSize < 100 )
				continue;
			fprintf( fpM, "%08llx\t%lld\n", (uint64_t)aryAllocInfo[i], aryAllocInfo[i]->m_nAllocSize );
			fprintf( fpS, "%08llx\n", (uint64_t)aryAllocInfo[i] );
			for( uint32_t j = 0; j < ALLOC_STACK; j++ )
			{
				char szBuffer[256];
				CDebug::Instance().DebugAddress2Symbol( aryAllocInfo[i]->m_pAllocAddress[j], szBuffer, 255 );
				fprintf( fpS, "\t%s\n", szBuffer );
			}
			fprintf( fpS, "\n" );
		}

		if( fpM )
			fclose( fpM );
		if( fpS )
			fclose( fpS );
#endif
	}

#endif

}	// namespce end
