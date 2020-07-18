//=========================================================================================
// Misc.h 
// ����
// �´���
// 2008-02-27
//=========================================================================================
#ifndef __XMEMORY_MISC_H__
#define __XMEMORY_MISC_H__

#include <stdint.h>
#include <stdlib.h>
#include <memory.h>

namespace XMemory
{
	template<class IntType>
	inline IntType AligenUp( IntType n, IntType nAligen )
	{
		return n ? ( ( n - 1 )/nAligen + 1 )*nAligen : 0;
	}
	
	//========================================================================
	// ���������϶���
	//========================================================================
	template<size_t n, size_t nAligen>
	class TAligenUp
	{
	public:
		enum { eValue = n ? ( ( n - 1 )/nAligen + 1 )*nAligen : 0 };
	};

	//========================================================================
	// ���������¶���
	//========================================================================
	template<size_t n, size_t nAligen>
	class TAligenDown
	{
	public:
		enum { eValue = ( n / nAligen )*nAligen };
	};
	
	//========================================================================
	// ���������ֵ
	//========================================================================
	template<size_t n1, size_t n2>
	class TMax
	{
	public:
		enum { eValue = n1 > n2 ? n1 : n2 };
	};

	//=====================================================================
	//   void GetBound( const VectorType& vecKey, const KeyType& nPos, 
	//                    IntType& nBegin, IntType& nEnd )
	//   ���Ѿ��ź����vector����ǰ���������ڵ�ֵ��ʹ��nPos��������ֵ֮���nBegin, nEnd��
	//   ���ص���������ҿ�
	//	 ��nPos������vector���κ�����ֵ�䣨vectorΪ�ջ���nPosС��vector�ĵ�һ��Ԫ�أ�ʱ����false
	//	 ��nPos��vector��ĳ����ֵ����ߴ��ڵ���vector���һ��Ԫ�ط���true��
	//   nPos���ڵ���vector���һ��Ԫ��ʱ�����ص�nBegin = nVectorSize - 1�� nEnd = nVectorSize
	//=====================================================================
	template<class VectorType, class KeyType, class IntType, class CompareType >
	bool GetBound( const VectorType& vecKey, size_t nVectorSize, const KeyType& nPos, IntType& nBegin, IntType& nEnd, const CompareType& Compare )
	{
		nBegin = nEnd = 0;
		if( nVectorSize == 0 )
			return false;

		if( Compare( nPos, vecKey[0] ) )
			return false;

		nEnd = (IntType)nVectorSize;
		while( nBegin != nEnd )
		{
			IntType nCur = (IntType)( ( nBegin + nEnd ) >> 1 );
			if( nCur == nBegin )
				return true;

			if( Compare( nPos, vecKey[nCur] ) )
				nEnd = nCur;
			else
			{
				nBegin = nCur;
				if( !Compare( vecKey[nCur], nPos ) )
				{
					nEnd = (IntType)( nCur + 1 );
					return true;
				}
			}
		}

		return true;
	}

	template<typename _Type>
	uint32_t strcpy_safe( _Type* pDes, const _Type* pSrc, uint32_t nSize )
	{
		if( !nSize )
			return 0;
		if( !pSrc )
		{
			pDes[0] = 0;
			return 0;
		}

		uint32_t i = 0;
		--nSize;
		while( i < nSize && *pSrc )
			pDes[i++] = *pSrc++;
		pDes[i] = 0;
		return i;
	}
}

#endif