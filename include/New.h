//=====================================================================
// CNew.h 
// 重载new
// 柯达昭
// 2010-07-26
//=======================================================================

#ifndef _GAMMA_NEW_H_
#define _GAMMA_NEW_H_

#include "Memory.h"

void* operator new( size_t nSize )	
{ 
	return XMemory::GammaAlloc( nSize, ( (void**)&nSize )[-1] );
}

void operator delete( void* p )	throw()
{
	XMemory::GammaFree( p );
}

void* operator new[] ( size_t nSize )
{ 
	return XMemory::GammaAlloc( nSize, ( (void**)&nSize )[-1] );
}

void operator delete[] ( void* p ) throw()
{ 
	XMemory::GammaFree( p ); 
}

#endif