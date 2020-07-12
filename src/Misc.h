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
}