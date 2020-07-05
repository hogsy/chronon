/*
Copyright (C) 2019 Mark Sowden <markelswo@gmail.com>

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

#pragma once

typedef unsigned long AFSFileHandle;
typedef void *UnknownType;

#ifdef __cplusplus
extern "C" {
#endif

	void afsDumpLoadedFiles( void );
	char *afsGetLastError( void );
	char *afsGetFileAttr( char *path );
	AFSFileHandle afsOpenFile( char *s1, char *s2 );
	int afsFreeFile( AFSFileHandle *fh );
	int afsWriteFile( AFSFileHandle fh, UnknownType param_2, UnknownType param_3 );

	int auxPerlHash( char *hash );
	int auxSprintf( char *str, size_t size, char *format, ... );

#ifdef __cplusplus
};
#endif
