//=====================================================================
// CNew.h 
// 重载new
// 柯达昭
// 2010-07-26
//=======================================================================

#ifndef _XMEMORY_NEW_H_
#define _XMEMORY_NEW_H_

#include "Memory.h"

void* operator new( size_t nSize )	
{ 
	return XMemory::Alloc( nSize, ( (void**)&nSize )[-1] );
}

void operator delete( void* p )	throw()
{
	XMemory::Free( p );
}

void* operator new[] ( size_t nSize )
{ 
	return XMemory::Alloc( nSize, ( (void**)&nSize )[-1] );
}

void operator delete[] ( void* p ) throw()
{ 
	XMemory::Free( p ); 
}

#endif