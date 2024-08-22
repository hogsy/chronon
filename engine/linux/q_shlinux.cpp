/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <sys/stat.h>
#include <sys/mman.h>
#include <cstdio>
#include <dirent.h>

#include "../linux/glob.h"

#include "../qcommon/qcommon.h"

//===============================================================================

int Sys_Mkdir( char *path )
{
	int status = mkdir( path, 0777 );
	if ( status == -1 )
	{
		if ( errno == EEXIST )// directory already exists
			status = 0;
		else
			status = -1;
	}

	return status;
}

//============================================

static char findbase[ MAX_OSPATH ];
static char findpath[ MAX_OSPATH ];
static char findpattern[ MAX_OSPATH ];
static DIR *fdir;

static bool CompareAttributes( char *path, char *name,
                               unsigned musthave, unsigned canthave )
{
	struct stat st;
	char        fn[ MAX_OSPATH ];

	// . and .. never match
	if ( strcmp( name, "." ) == 0 || strcmp( name, ".." ) == 0 )
		return false;

	return true;

	if ( stat( fn, &st ) == -1 )
		return false;// shouldn't happen

	if ( ( st.st_mode & S_IFDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;

	if ( ( musthave & SFF_SUBDIR ) && !( st.st_mode & S_IFDIR ) )
		return false;

	return true;
}

char *Sys_FindFirst( char *path, unsigned musthave, unsigned canhave )
{
	struct dirent *d;
	char          *p;

	if ( fdir )
		Sys_Error( "Sys_BeginFind without close" );

	//	COM_FilePath (path, findbase);
	strcpy( findbase, path );

	if ( ( p = strrchr( findbase, '/' ) ) != NULL )
	{
		*p = 0;
		strcpy( findpattern, p + 1 );
	}
	else
		strcpy( findpattern, "*" );

	if ( strcmp( findpattern, "*.*" ) == 0 )
		strcpy( findpattern, "*" );

	if ( ( fdir = opendir( findbase ) ) == NULL )
		return NULL;
	while ( ( d = readdir( fdir ) ) != NULL )
	{
		if ( !*findpattern || glob_match( findpattern, d->d_name ) )
		{
			//			if (*findpattern)
			//				printf("%s matched %s\n", findpattern, d->d_name);
			if ( CompareAttributes( findbase, d->d_name, musthave, canhave ) )
			{
				sprintf( findpath, "%s/%s", findbase, d->d_name );
				return findpath;
			}
		}
	}
	return NULL;
}

char *Sys_FindNext( unsigned musthave, unsigned canhave )
{
	struct dirent *d;

	if ( fdir == NULL )
		return NULL;
	while ( ( d = readdir( fdir ) ) != NULL )
	{
		if ( !*findpattern || glob_match( findpattern, d->d_name ) )
		{
			//			if (*findpattern)
			//				printf("%s matched %s\n", findpattern, d->d_name);
			if ( CompareAttributes( findbase, d->d_name, musthave, canhave ) )
			{
				sprintf( findpath, "%s/%s", findbase, d->d_name );
				return findpath;
			}
		}
	}
	return NULL;
}

void Sys_FindClose( void )
{
	if ( fdir != NULL )
		closedir( fdir );
	fdir = NULL;
}


//============================================
