#pragma once
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

#if( defined __linux__ )
	enum EMemoryConst { eMemoryConst_Unit = 16 };
	static const uint32_t s_aryClassSize[] =
	{ 
		16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 256,
		272, 288, 304, 320, 336, 352, 368, 384, 400, 416, 432, 448, 464, 480, 496, 
		512, 528, 544, 560, 576, 592, 608, 624, 640, 656, 672, 688, 704, 720, 736, 
		752, 768, 784, 800, 816, 832, 848, 864, 880, 896, 912, 928, 944, 960, 976, 
		992, 1008, 1024, 1040, 1056, 1072, 1088, 1104, 1120, 1136, 1168, 1184, 1200, 
		1232, 1248, 1280, 1296, 1328, 1360, 1392, 1424, 1456, 1488, 1520, 1552, 1584, 
		1632, 1680, 1712, 1760, 1808, 1872, 1920, 1984, 2048, 2112, 2176, 2256, 2336, 
		2416, 2512, 2608, 2720, 2848, 2976, 3120, 3264, 3440, 3632, 3840, 4096, 4368, 
		4672, 5040, 5456, 5952, 6544, 7280, 8192, 9360, 10912, 13104, 16384
	};
#else
	enum EMemoryConst { eMemoryConst_Unit = 8 };
	static const uint32_t s_aryClassSize[] =
	{
		8,    16,    24,    32,   40,   48,   56,   64,   72, 80,
		88,   96,   104,   112,  120,  128,  136,  144,  152, 160,
		168,  176,  184,   192,  200,  208,  216,  224,  232, 240,
		248,  256,  264,   272,  280,  288,  296,  304,  312, 320,
		328,  336,  344,   352,  360,  368,  376,  384,  392, 400,
		408,  416,  424,   432,  440,  448,  456,  464,  472, 480,
		488,  496,  504,   512,  520,  528,  536,  544,  552, 560,
		568,  576,  584,   592,  600,  608,  616,  624,  632, 640,
		648,  656,  664,   672,  680,  688,  696,  704,  712, 720,
		728,  736,  744,   752,  760,  768,  776,  784,  792, 808,
		816,  824,  840,   848,  856,  872,  880,  896,  904, 920,
		936,  944,  960,   976,  992, 1008, 1024, 1040, 1056, 1072,
		1088, 1104, 1128, 1144, 1168, 1184, 1208, 1232, 1256, 1280,
		1304, 1336, 1360, 1392, 1424, 1456, 1488, 1520, 1560, 1592,
		1632, 1680, 1720, 1768, 1816, 1872, 1920, 1984, 2048, 2112,
		2184, 2256, 2336, 2424, 2520, 2616, 2728, 2848, 2976, 3120,
		3272, 3448, 3640, 3848, 4096, 4368, 4680, 5040, 5456, 5952,
		6552, 7280, 8192, 9360, 10920, 13104, 16384,
	};
#endif

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
			eMemoryConst_IndexCount = eMemoryConst_MaxSize / eMemoryConst_Unit,		// 索引数量
			eMemoryConst_AllocateCount = sizeof(s_aryClassSize)/sizeof(uint32_t),	// 分配器个数
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
				uint32_t		m_nBlockSize;
#ifdef _IOS
				uint64_t		m_nPackHead;
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
