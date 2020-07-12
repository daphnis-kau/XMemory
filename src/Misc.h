namespace XMemory
{
	template<class IntType>
	inline IntType AligenUp( IntType n, IntType nAligen )
	{
		return n ? ( ( n - 1 )/nAligen + 1 )*nAligen : 0;
	}

	//=====================================================================
	//   void GetBound( const VectorType& vecKey, const KeyType& nPos, 
	//                    IntType& nBegin, IntType& nEnd )
	//   在已经排好序的vector查找前后两个相邻的值，使得nPos在这两个值之间［nBegin, nEnd）
	//   返回的区间左闭右开
	//	 当nPos不存在vector中任何两个值间（vector为空或者nPos小于vector的第一个元素）时返回false
	//	 当nPos在vector中某两个值间或者大于等于vector最后一个元素返回true。
	//   nPos大于等于vector最后一个元素时，返回的nBegin = nVectorSize - 1， nEnd = nVectorSize
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
}