#ifndef _WIN32
#include <unistd.h>
#else
#include <direct.h>
#endif
#include <stdio.h>
#include "sampler.h"

//打开下面的注释，会在*c = 0;那一行crash
#include "New.h"

int main( int argc, const char* argv[] )
{
	char* c = new char;
	delete c;
	*c = 0;
	return 0;
}

