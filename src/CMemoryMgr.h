#pragma once
#include "Misc.h"
#include "TList.h"
#include "Memory.h"
#include "LockHelp.h"

//linux下和v8不兼容，暂时屏蔽
#define ENABLE_MEMORY_MGR

// 检查内存泄露
#define MEMORY_LEASK	

// linux下必须
// echo 655300000 > /proc/sys/vm/max_map_count
// 否则mmap的次数会受限
#define CHECK_MEMORY_ERROR
#define CHECK_MEMORY_BOUND

namespace XMemory
{
	// 通过模板计算分配器类型数量
	template<int32_t UnitStep, int32_t PageSize, int32_t MaxUnitSize>
	struct TMemoryUnitInfo
	{
		template<int32_t UnitSize>
		struct GetIndexSize
		{
			enum
			{
				eUnitCount = PageSize/UnitSize,
				eNextMinCount = eUnitCount + 1,
				eNextCountBySize = PageSize/( UnitSize - UnitStep),
				eNextUnitCount = TMax<eNextMinCount, eNextCountBySize>::eValue,
				eNextUnitRawSize = PageSize/eNextUnitCount,
				eNextUnitSize = TAligenDown<eNextUnitRawSize, UnitStep>::eValue,
				eUnitIndex = GetIndexSize<eNextUnitSize>::eUnitIndex + 1
			};

			static void BuildClassSize( uint32_t aryClassSize[] ) 
			{
				GetIndexSize<eNextUnitSize>::BuildClassSize( aryClassSize );
				aryClassSize[eUnitIndex] = UnitSize;
			}
		};

		template<>
		struct GetIndexSize<UnitStep>
		{
			enum
			{
				eUnitCount = PageSize/UnitStep,
				eUnitIndex = 0
			};

			static void BuildClassSize( uint32_t aryClassSize[] )
			{
				aryClassSize[eUnitIndex] = UnitStep;
			}
		};

		enum 
		{ 
			eUnitStep = UnitStep,
			eUnitClassCount = GetIndexSize<MaxUnitSize>::eUnitIndex + 1
		};
		uint32_t m_aryClassSize[eUnitClassCount];

		TMemoryUnitInfo() { GetIndexSize<MaxUnitSize>::BuildClassSize( m_aryClassSize ); }
	};

	class CMemoryBlock;

	class CMemoryMgr 
#ifdef CHECK_MEMORY_BOUND
		: TList<CMemoryBlock>
#endif
	{
		enum
		{
			eMemoryConst_PageSize = 64 * 1024,
			eMemoryConst_MaxSize = 16384,											// 最大管理内存大小，超过该值则不进行管理
			eMemoryConst_Unit = sizeof( void* )*2
		};

		enum
		{
			eMemMgr_Use		= 0x80,
			eMemMgr_Freed 	= 0xff,
		};

		struct SMgrMemHead
		{
			uint16_t	m_nEmpty;
			uint8_t		m_nIndex;
			uint8_t		m_nFlag;
		};

		struct SAllocInfo;
		struct SMemHead 
		{
			union
			{
				SMgrMemHead	m_MgrHead;
				uint32_t	m_nBlockSize;
#ifdef _IOS
				uint64_t	m_nPackHead;
#elif( defined __linux__ )
				void*		m_pPackHead[2];
#else
				void*		m_pPackHead;
#endif
			};

#ifdef MEMORY_LEASK
			SAllocInfo*		m_pAllocInfo;
#endif
		};

		typedef 
			TMemoryUnitInfo
			<eMemoryConst_Unit, eMemoryConst_PageSize, eMemoryConst_MaxSize> 
			SMemoryUnitInfo;

		enum 
		{
			eMemoryConst_IndexCount = eMemoryConst_MaxSize / SMemoryUnitInfo::eUnitStep,	// 索引数量
			eMemoryConst_AllocateCount = SMemoryUnitInfo::eUnitClassCount,					// 分配器个数
		};

	public:
		static CMemoryMgr& Instance();

		void*				AllocFromSystem( size_t nSize );
		void				FreeToSystem( void* pMemory, size_t nSize );

#ifndef ENABLE_MEMORY_MGR
		void*				Allocate( size_t nSize, void* )	{ return malloc( nSize );	}
		void				Free( void * p )				{ return free( p );			}
		uint64_t			GetTotalAlloc()					{ return -1;				}
		uint64_t			GetTotalMgrSize()				{ return -1;				}
		uint64_t			GetFreeMgrSize()				{ return -1;				}
		uint32_t			GetAllocStack( uint32_t, void**&, size_t& ){ return false;	}
		void				DumpMemoryInfo()				{							}
#else
		CMemoryMgr(void);
		~CMemoryMgr(void);

		void*				Allocate( size_t nSize, void* pCallAddress );
		void				Free( void* p );
		uint64_t			GetTotalAlloc();
		uint64_t			GetTotalMgrSize();
		uint64_t			GetFreeMgrSize();
		uint32_t			GetAllocStack( uint32_t nIndex, void**& pAddress, size_t& nAllocSize );
		void				DumpMemoryInfo();

	//////////////////////////////////////////////////////////////////////////
	private:
		SMemoryUnitInfo		m_MemoryUnitInfo;

		bool				m_bEnable;
		uint8_t				m_aryClassIndex[eMemoryConst_IndexCount];
		SLock				m_hFixMemoryLock[eMemoryConst_AllocateCount];

		uint64_t			m_nFixManageSize[eMemoryConst_AllocateCount];
		uint64_t			m_nFixAllocSize[eMemoryConst_AllocateCount];
		uint64_t			m_nBigAllocSize;

		SMemHead*			m_aryMemFreeList[eMemoryConst_AllocateCount];

#ifdef CHECK_MEMORY_ERROR
		SLock				m_hGlobalMemoryLock;
		size_t				m_nCurPage;
		uint8_t*			m_pAddrStart;
		size_t				m_nPageSize;
#endif

#ifdef MEMORY_LEASK
#define ALLOC_STACK 15
		struct SAllocInfo
		{
			void*			m_pAllocAddress[ALLOC_STACK];
			size_t			m_nAllocSize;
		};

		struct SAllocInfoCompare
		{
			inline bool operator()( const SAllocInfo& AllocInfo, SAllocInfo* pInfo ) const 
			{ 
				return memcmp( AllocInfo.m_pAllocAddress, pInfo->m_pAllocAddress, sizeof(void*)*ALLOC_STACK ) < 0; 
			}

			inline bool operator()( SAllocInfo* pInfo, const SAllocInfo& AllocInfo ) const 
			{ 
				return memcmp( pInfo->m_pAllocAddress, AllocInfo.m_pAllocAddress, sizeof(void*)*ALLOC_STACK ) < 0; 
			}
		};

		SLock				m_hAllMemory;
		SAllocInfo**		m_aryAllocInfo;
		uint32_t			m_nAllocInfoMaxSize;
		uint32_t			m_nAllocInfoSize;
		SAllocInfo*			m_pFreeInfo;
		uint32_t			m_nFreeCount;

		SAllocInfo*			InsertAllocInfo( uint32_t nIndex, void* pAllocAddress[ALLOC_STACK], size_t nAllocSize );
#endif

#endif
	};

}
