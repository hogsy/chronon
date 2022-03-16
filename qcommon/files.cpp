/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

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

#include "qcommon.h"

#include <sys/stat.h>
#include <miniz/miniz.h>

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

typedef struct PackageHeader {
	char		identifier[ 4 ];	/* ADAT */
	uint32_t	tocOffset;			/* table of contents */
	uint32_t	tocLength;			/* table of contents length */
	uint32_t	version;			/* always appears to be 9 */
} PackageHeader;

typedef struct PackageIndex {
	char		name[ 128 ];		/* the name of the file, excludes 'model/' etc. */
	uint32_t	offset;				/* offset into the dat that the file resides */
	uint32_t	length;				/* decompressed length of the file */
	uint32_t	compressedLength;	/* length of the file in the dat */
	uint32_t	u0;
} PackageIndex;

typedef struct Package {
	char			mappedDir[ 32 ];			/* e.g., 'models' */
	char			path[ MAX_QPATH ];
	PackageHeader	header;						/* header data */
	PackageIndex *indices;					/* index data */
	unsigned int	numFiles;					/* number of files within */
	LinkedListNode *nodeIndex;
} Package;

/**
 * Convert back-slashes to forward slashes, to play nice with our packages.
 */
void FS_CanonicalisePath( char *path )
{
	while ( *path != '\0' )
	{
		if ( *path == '\\' )
		{
			*path = '/';
		}

		path++;
	}
}

/**
 * Search for the given file within a package and return it's index.
 */
static const PackageIndex *FS_GetPackageFileIndex( const Package *package, const char *fileName ) {
	for( unsigned int i = 0; i < package->numFiles; ++i ) {
		const PackageIndex *index = &package->indices[ i ];
		if( Q_strncasecmp( index->name, fileName, sizeof( index->name ) ) == 0 ) {
			return index;
		}
	}

	return nullptr;
}

/**
 * Decompress the given file and carry out validation.
 */
static bool FS_DecompressFile( const uint8_t *srcBuffer, size_t srcLength, uint8_t *dstBuffer, size_t *dstLength, size_t expectedLength ) {
	int returnCode = mz_uncompress( dstBuffer, (mz_ulong *)dstLength, srcBuffer, (mz_ulong) srcLength );
	if( returnCode != MZ_OK ) {
		Com_Printf( "Failed to decompress data, return code \"%d\"!\n", returnCode );
		return false;
	}

	if( *dstLength != expectedLength ) {
		Com_Printf( "Unexpected size following decompression, %d vs %d!\n", *dstLength, expectedLength );
		return false;
	}

	return true;
}

/**
 * Load a file from the given package, and decompress the data.
 */
static uint8_t *FS_LoadPackageFile( const Package *package, const char *fileName, uint32_t *fileLength ) {
	/* first, ensure that it's actually in the package file table */
	const PackageIndex *fileIndex = FS_GetPackageFileIndex( package, fileName );
	if( fileIndex == NULL ) {
		return NULL;
	}

	/* now proceed to load that file */

	FILE *filePtr = fopen( package->path, "rb" );
	if( filePtr == NULL ) {
		Com_Printf( "WARNING: Failed to open package \"%s\"!\n", package->path );
		return NULL;
	}

	fseek( filePtr, fileIndex->offset, SEEK_SET );

	/* now load the compressed block in */
	uint8_t *srcBuffer = static_cast<uint8_t *>( Z_Malloc( fileIndex->compressedLength ) );
	fread( srcBuffer, sizeof( uint8_t ), fileIndex->compressedLength, filePtr );

	/* decompress it */

	uint8_t *dstBuffer = static_cast<uint8_t *>( Z_Malloc( fileIndex->length ) );
	size_t dstLength = fileIndex->length;

	bool status = FS_DecompressFile( srcBuffer, fileIndex->compressedLength, dstBuffer, &dstLength, fileIndex->length );

	Z_Free( srcBuffer );

	fclose( filePtr );

	if( status ) {
#if 0 /* write it out so we can make sure it's all good! */
		//char outFileName[ MAX_QPATH ];
		//snprintf( outFileName, sizeof( outFileName ), "debug/%s.file", rand() % 256 );
		FILE *outFile = fopen( fileName, "wb" );
		if ( outFile != NULL ) {
			fwrite( dstBuffer, sizeof( uint8_t ), dstLength, outFile );
			fclose( outFile );
		}
#endif

		*fileLength = dstLength;
		return dstBuffer;
	}

	Z_Free( dstBuffer );

	return NULL;
}

static Package *FS_MountPackage( FILE *filePtr, const char *identity ) {
	if( filePtr == NULL ) {
		Com_Printf( "WARNING: Invalid file handle!\n" );
		return NULL;
	}

	if( identity == NULL || identity[ 0 ] == '\0' ) {
		Com_Printf( "WARNING: Invalid package identity!\n" );
		return NULL;
	}

	/* read in the header */
	PackageHeader header;
	fread( &header, sizeof( PackageHeader ), 1, filePtr );

	/* and now ensure it's as desired! */
	if( strncmp( header.identifier, "ADAT", sizeof( header.identifier ) ) != 0 ) {
		Com_Printf( "WARNING: Invalid identifier, package returned \"%s\" rather than \"ADAT\"!\n", header.identifier );
		return NULL;
	}

	if( header.version != 9 ) {
		Com_Printf( "WARNING: Unexpected package version, \"%d\" (expected \"9\")!\n", header.version );
		return NULL;
	}

	unsigned int numFiles = header.tocLength / sizeof( PackageIndex );
	if( numFiles == 0 ) {
		Com_Printf( "WARNING: Empty package!\n" );
		return NULL;
	}

	Package *package = static_cast<Package *>( Z_Malloc( sizeof( Package ) ) );
	strcpy( package->mappedDir, identity );

	package->header = header;
	package->numFiles = numFiles;

	/* seek to the table of contents */
	fseek( filePtr, package->header.tocOffset, SEEK_SET );

	/* and now read in the table of contents */
	package->indices = static_cast<PackageIndex *>( Z_Malloc( sizeof( PackageIndex ) * package->numFiles ) );
	if( fread( package->indices, sizeof( PackageIndex ), package->numFiles, filePtr ) != package->numFiles ) {
		Z_Free( package->indices );
		Z_Free( package );

		Com_Printf( "WARNING: Failed to read entire table of contents!\n" );
		return NULL;
	}

	/* flip back slash to forward */
	for( unsigned int i = 0; i < package->numFiles; ++i ) {
		FS_CanonicalisePath( package->indices[ i ].name );
	}

	return package;
}

//
// in memory
//

static char fs_gamedir[ MAX_OSPATH ];
static cvar_t *fs_basedir;
static cvar_t *fs_cddir;

cvar_t *fs_gamedirvar;

typedef struct searchpath_s {
	char				filename[ MAX_OSPATH ];
	LinkedList *packDirectories;
	struct searchpath_s *next;
} searchpath_t;

static searchpath_t *fs_searchpaths;
static searchpath_t *fs_base_searchpaths;	// without gamedirs


/*

All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

*/

/**
 * Stat a file to fetch it's size. Only works for local files.
 */
int FS_GetLocalFileLength( const char *path ) {
	struct stat buf;
	if( stat( path, &buf ) != 0 ) {
		return -1;
	}

	return (int)buf.st_size;
}

/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath( char *path ) {
	for( char *ofs = path + 1; *ofs; ofs++ ) {
		if( *ofs == '/' ) {	// create the directory
			*ofs = 0;
			Sys_Mkdir( path );
			*ofs = '/';
		}
	}
}

/**
 * Check if the given file exists locally. Only works for local files.
 */
bool FS_LocalFileExists( const char *path ) {
	struct stat stats;
	if( stat( path, &stats ) == 0 ) {
		return true;
	}

	return false;
}

/*
===========
FS_FOpenFile

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/
uint8_t *FS_FOpenFile( const char *filename, uint32_t *length ) {
	/* search through the path, one element at a time */
	for( searchpath_t *search = fs_searchpaths; search; search = search->next ) {
		// check a file in the directory tree
		char netpath[ MAX_OSPATH ];
		Com_sprintf( netpath, sizeof( netpath ), "%s/%s", search->filename, filename );

		FS_CanonicalisePath( netpath );

		/* first, attempt to open it locally */
		uint32_t fileLength = FS_GetLocalFileLength( netpath );
		if( fileLength >= 0 ) {
			FILE *filePtr = fopen( netpath, "rb" );
			if( filePtr != NULL ) {
				/* allocate a buffer and read the whole thing into memory */
				uint8_t *buffer = static_cast<uint8_t *>( Z_Malloc( fileLength ) );
				fread( buffer, sizeof( uint8_t ), fileLength, filePtr );

				fclose( filePtr );
				*length = fileLength;

				return buffer;
			}
		}

		/* otherwise, load it from one of the anox packages */

		/* Anachronox does some horrible path munging in order to determine which package
		 * it should be loading from, so we need to grab the first dir here to do the same. */
		char rootFolder[ 32 ];
		memset( rootFolder, 0, sizeof( rootFolder ) );
		const char *p = filename;
		for( unsigned int i = 0; i < sizeof( rootFolder ) - 4; ++i ) {
			if( *p == '\0' || *p == '/' ) {
				break;
			}

			rootFolder[ i ] = *p++;
		}
		p++;

		/* see if we have a match! */
		LinkedListNode *node = LL_GetRootNode( search->packDirectories );
		while( node != NULL ) {
			Package *package = (Package *)LL_GetLinkedListNodeUserData( node );
			if( package == NULL ) {
				break;
			}

			if( strcmp( rootFolder, package->mappedDir ) == 0 ) {
				/* we've got a match! */
				uint8_t *buffer = FS_LoadPackageFile( package, p, length );
				if( buffer != NULL ) {
					return buffer;
				}

				break;
			}

			node = LL_GetNextLinkedListNode( node );
		}
	}

	Com_DPrintf( "FindFile: can't find %s\n", filename );

	return NULL;
}

/*
=================
FS_ReadFile

Properly handles partial reads
=================
*/
void CDAudio_Stop( void );
#define	MAX_READ	0x10000		// read in blocks of 64k
void FS_Read( void *buffer, int len, FILE *f ) {
	int		block, remaining;
	int		read;
	byte *buf;
	int		tries;

	buf = (byte *)buffer;

	// read in chunks for progress bar
	remaining = len;
	tries = 0;
	while( remaining ) {
		block = remaining;
		if( block > MAX_READ )
			block = MAX_READ;
		read = fread( buf, 1, block, f );
		if( read == 0 ) {
			// we might have been trying to read from a CD
			if( !tries ) {
				tries = 1;
				CDAudio_Stop();
			} else
				Com_Error( ERR_FATAL, "FS_Read: 0 bytes read" );
		}

		if( read == -1 )
			Com_Error( ERR_FATAL, "FS_Read: -1 bytes read" );

		// do some progress bar thing here...

		remaining -= read;
		buf += read;
	}
}

/*
============
FS_LoadFile

Filename are reletive to the quake search path
a null buffer will just return the file length without loading
============
*/
int FS_LoadFile( const char *path, void **buffer ) {
	char upath[ MAX_QPATH ];
	snprintf( upath, sizeof( upath ), "%s", path );

	FS_CanonicalisePath( upath );

	// look for it in the filesystem or pack files
	uint32_t length;
	uint8_t *buf = FS_FOpenFile( upath, &length );
	if( buf == nullptr ) {
		if( buffer != nullptr ) {
			*buffer = nullptr;
		}

		return -1;
	}

	/* retain compat for fetching the length, for now */
	if( buffer == nullptr ) {
		Z_Free( buf );

		return length;
	}

	*buffer = buf;

	return length;
}

/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile( void *buffer ) {
	Z_Free( buffer );
}

/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
static void FS_AddGameDirectory( const char *dir ) {
	strcpy( fs_gamedir, dir );

	//
	// add the directory to the search path
	//
	searchpath_t *search = static_cast<searchpath_t *>( Z_Malloc( sizeof( searchpath_t ) ) );
	strcpy( search->filename, dir );
	search->next = fs_searchpaths;
	fs_searchpaths = search;

	/* now go ahead and mount all the default packages under that dir */

	static const char *defaultPacks[] = {
		"battle",
		"gameflow",
		"graphics",
		"maps",
		"models",
		"objects",
		"particles",
		"scripts",
		"sound",
		"sprites",
		"textures",
	};

	search->packDirectories = LL_CreateLinkedList();
	for( uint8_t i = 0; i < Q_ARRAY_LENGTH( defaultPacks ); ++i ) {
		/* check a file in the directory tree, e.g. 'anoxdata/battle.dat' */
		std::string packPath = search->filename;
		packPath += "/" + std::string( defaultPacks[ i ] ) + ".dat";

		FILE *filePtr = fopen( packPath.c_str(), "rb" );
		if( filePtr == nullptr ) {
			continue;
		}

		Package *package = FS_MountPackage( filePtr, defaultPacks[ i ] );

		fclose( filePtr );

		if( package == nullptr ) {
			continue;
		}

		/* if it loaded successfully, add it onto the list */
		package->nodeIndex = LL_InsertLinkedListNode( search->packDirectories, package );
		strcpy( package->path, packPath.c_str() );
	}
}

/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
const char *FS_Gamedir( ) {
	if( *fs_gamedir )
		return fs_gamedir;
	else
		return BASEDIRNAME;
}

/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec( void ) {
	char *dir;
	char name[ MAX_QPATH ];

	dir = Cvar_VariableString( "gamedir" );
	if( *dir )
		Com_sprintf( name, sizeof( name ), "%s/%s/autoexec.cfg", fs_basedir->string, dir );
	else
		Com_sprintf( name, sizeof( name ), "%s/%s/autoexec.cfg", fs_basedir->string, BASEDIRNAME );
	if( Sys_FindFirst( name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM ) )
		Cbuf_AddText( "exec autoexec.cfg\n" );
	Sys_FindClose();
}

/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir( const char *dir ) {
	searchpath_t *next;

	if( strstr( dir, ".." ) || strstr( dir, "/" ) || strstr( dir, "\\" ) || strstr( dir, ":" ) ) {
		Com_Printf( "Gamedir should be a single filename, not a path\n" );
		return;
	}

	//
	// free up any current game dir info
	//
	while( fs_searchpaths != fs_base_searchpaths ) {
		if( fs_searchpaths->packDirectories != NULL ) {
			/* go through the list and free up the user data */
			LinkedListNode *node = LL_GetRootNode( fs_searchpaths->packDirectories );
			while( node != NULL ) {
				Package *package = (Package *)LL_GetLinkedListNodeUserData( node );

				Z_Free( package->indices );
				Z_Free( package );

				node = LL_GetNextLinkedListNode( node );
			}

			LL_DestroyLinkedList( fs_searchpaths->packDirectories );
		}

		next = fs_searchpaths->next;
		Z_Free( fs_searchpaths );
		fs_searchpaths = next;
	}

	//
	// flush all data, so it will be forced to reload
	//
	if( dedicated && !dedicated->value )
		Cbuf_AddText( "vid_restart\nsnd_restart\n" );

	Com_sprintf( fs_gamedir, sizeof( fs_gamedir ), "%s/%s", fs_basedir->string, dir );

	if( !strcmp( dir, BASEDIRNAME ) || ( *dir == 0 ) ) {
		Cvar_FullSet( "gamedir", "", CVAR_SERVERINFO | CVAR_NOSET );
		Cvar_FullSet( "game", "", CVAR_LATCH | CVAR_SERVERINFO );
	} else {
		Cvar_FullSet( "gamedir", dir, CVAR_SERVERINFO | CVAR_NOSET );
		if( fs_cddir->string[ 0 ] ) {
			FS_AddGameDirectory( va( "%s/%s", fs_cddir->string, dir ) );
		}

		FS_AddGameDirectory( va( "%s/%s", fs_basedir->string, dir ) );
	}
}

/*
** FS_ListFiles
*/
char **FS_ListFiles( char *findname, int *numfiles, unsigned musthave, unsigned canthave ) {
	char *s;
	int nfiles = 0;
	char **list = 0;

	s = Sys_FindFirst( findname, musthave, canthave );
	while( s ) {
		if( s[ strlen( s ) - 1 ] != '.' )
			nfiles++;
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose();

	if( !nfiles )
		return NULL;

	nfiles++; // add space for a guard
	*numfiles = nfiles;

	list = static_cast<char **>( malloc( sizeof( char * ) * nfiles ) );
	memset( list, 0, sizeof( char * ) * nfiles );

	s = Sys_FindFirst( findname, musthave, canthave );
	nfiles = 0;
	while( s ) {
		if( s[ strlen( s ) - 1 ] != '.' ) {
			list[ nfiles ] = Q_strdup( s );
			Q_strtolower( list[ nfiles ] );
			nfiles++;
		}
		s = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose();

	return list;
}

/*
** FS_Dir_f
*/
void FS_Dir_f( void ) {
	char *path = NULL;
	char	findname[ 1024 ];
	char	wildcard[ 1024 ] = "*.*";
	char **dirnames;
	int		ndirs;

	if( Cmd_Argc() != 1 ) {
		strcpy( wildcard, Cmd_Argv( 1 ) );
	}

	while( ( path = FS_NextPath( path ) ) != NULL ) {
		char *tmp = findname;

		Com_sprintf( findname, sizeof( findname ), "%s/%s", path, wildcard );

		while( *tmp != 0 ) {
			if( *tmp == '\\' )
				*tmp = '/';
			tmp++;
		}
		Com_Printf( "Directory of %s\n", findname );
		Com_Printf( "----\n" );

		if( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 ) {
			int i;

			for( i = 0; i < ndirs - 1; i++ ) {
				if( strrchr( dirnames[ i ], '/' ) )
					Com_Printf( "%s\n", strrchr( dirnames[ i ], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[ i ] );

				free( dirnames[ i ] );
			}
			free( dirnames );
		}
		Com_Printf( "\n" );
	};
}

/*
============
FS_Path_f

============
*/
void FS_Path_f( void ) {
	Com_Printf( "Current search path:\n" );
	for( searchpath_t *s = fs_searchpaths; s; s = s->next ) {
		if( s == fs_base_searchpaths )
			Com_Printf( "----------\n" );

		Com_Printf( "%s\n", s->filename );

		/* list out all the mounted packages */

		Com_Printf( "Packages:\n" );

		LinkedListNode *node = LL_GetRootNode( s->packDirectories );
		while( node != NULL ) {
			Package *package = (Package *)LL_GetLinkedListNodeUserData( node );

			Com_Printf( " %s\n", package->mappedDir );

			node = LL_GetNextLinkedListNode( node );
		}
	}
}

/*
================
FS_NextPath

Allows enumerating all of the directories in the search path
================
*/
char *FS_NextPath( char *prevpath ) {
	if( !prevpath ) {
		return fs_gamedir;
	}

	char *prev = fs_gamedir;
	for( searchpath_t *s = fs_searchpaths; s; s = s->next ) {
		if( prevpath == prev ) {
			return s->filename;
		}

		prev = s->filename;
	}

	return NULL;
}

/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem( void ) {
	Cmd_AddCommand( "path", FS_Path_f );
	Cmd_AddCommand( "dir", FS_Dir_f );

	//
	// basedir <path>
	// allows the game to run from outside the data tree
	//
	fs_basedir = Cvar_Get( "basedir", ".", CVAR_NOSET );

	//
	// cddir <path>
	// Logically concatenates the cddir after the basedir for
	// allows the game to run from outside the data tree
	//
	fs_cddir = Cvar_Get( "cddir", "", CVAR_NOSET );
	if( fs_cddir->string[ 0 ] )
		FS_AddGameDirectory( va( "%s/" BASEDIRNAME, fs_cddir->string ) );

	//
	// start up with baseq2 by default
	//
	FS_AddGameDirectory( va( "%s/" BASEDIRNAME, fs_basedir->string ) );

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths;

	// check for game override
	fs_gamedirvar = Cvar_Get( "game", "", CVAR_LATCH | CVAR_SERVERINFO );
	if( fs_gamedirvar->string[ 0 ] )
		FS_SetGamedir( fs_gamedirvar->string );
}
