#include <dlfcn.h>
#include <iostream>
#include <stdarg.h>

#include <set>
#include <string>
#include <memory>

#include "IECore/Export.h"

typedef int (*orig_open_f_type)(const char *pathname, int flags );
typedef FILE* (*orig_fopen_f_type)(const char *pathname, const char *mode);
typedef int (*orig_close_f_type) (int fildes);

namespace
{
	void addFile(const char* ctx, const char* path)
	{
		std::cerr << ctx << " '" << path << "'" << std::endl;
	}
}

extern "C"
{

IECORE_API int close(int fildes)
{
	static orig_close_f_type orig_close = nullptr;
	if ( !orig_close )
	{
		orig_close = (orig_close_f_type) dlsym( RTLD_NEXT, "close" );
	}

	return orig_close(fildes);
}

IECORE_API int open64(const char *pathname, int flags)
{
	addFile( "open64", pathname );
	static orig_open_f_type orig_open = nullptr;
	if (!orig_open)
	{
		orig_open = (orig_open_f_type) dlsym( RTLD_NEXT, "open64" );
	}

	int r = orig_open( pathname, flags );
	return r;
}

IECORE_API int open( const char *pathname, int flags )
{
	static int index = 0;

	index++;
	if (index > 5)
		addFile( "open", pathname );

	static orig_open_f_type orig_open = nullptr;
	if( !orig_open )
	{
		orig_open = (orig_open_f_type) dlsym( RTLD_NEXT, "open" );
	}

	int r = orig_open( pathname, flags );
	return r;
}

IECORE_API FILE *fopen(const char *pathname, const char *mode)
{

	addFile( "fopen", pathname );

	static orig_fopen_f_type orig_fopen = nullptr;
	if ( !orig_fopen )
	{
		orig_fopen = (orig_fopen_f_type) dlsym( RTLD_NEXT, "fopen" );
	}

	FILE* f = orig_fopen( pathname, mode );

	return f;
}

IECORE_API FILE *fopen64(const char *pathname, const char *mode)
{

	addFile( "fopen64", pathname );

	static orig_fopen_f_type orig_fopen = nullptr;
	if ( !orig_fopen )
	{
		orig_fopen = (orig_fopen_f_type) dlsym( RTLD_NEXT, "fopen64" );
	}

	FILE* f = orig_fopen( pathname, mode );

	return f;
}


}



