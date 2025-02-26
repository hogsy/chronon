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
// snd_loc.h -- private sound functions

// !!! if this is changed, the asm code must change !!!
typedef struct
{
	int left;
	int right;
} portable_samplepair_t;

typedef struct
{
	int  length;
	int  loopstart;
	int  speed;// not needed, because converted on load?
	int  width;
	int  stereo;
	byte data[ 1 ];// variable sized
} sfxcache_t;

typedef struct sfx_s
{
	char        name[ MAX_STRLENGTH ];
	int         registration_sequence;
	sfxcache_t *cache;
	char       *truename;
} sfx_t;

// a playsound_t will be generated by each call to S_StartSound,
// when the mixer reaches playsound->begin, the playsound will
// be assigned to a channel
typedef struct playsound_s
{
	struct playsound_s *prev, *next;
	sfx_t              *sfx;
	float               volume;
	float               attenuation;
	int                 entnum;
	int                 entchannel;
	bool                fixed_origin;// use origin field instead of entnum's origin
	vec3_t              origin;
	unsigned            begin;// begin on this sample
} playsound_t;

typedef struct
{
	int   channels;
	int   samples;         // mono samples in buffer
	int   submission_chunk;// don't mix less than this #
	int   samplepos;       // in mono samples
	int   samplebits;
	int   speed;
	byte *buffer;
} dma_t;

// !!! if this is changed, the asm code must change !!!
typedef struct
{
	sfx_t *sfx;         // sfx number
	int    leftvol;     // 0-255 volume
	int    rightvol;    // 0-255 volume
	int    end;         // end time in global paintsamples
	int    pos;         // sample position in sfx
	int    looping;     // where to loop, -1 = no looping OBSOLETE?
	int    entnum;      // to allow overriding a specific sound
	int    entchannel;  //
	vec3_t origin;      // only use if fixed_origin is set
	vec_t  dist_mult;   // distance multiplier (attenuation/clipK)
	int    master_vol;  // 0-255 master volume
	bool   fixed_origin;// use origin instead of fetching entnum's origin
	bool   autosound;   // from an entity->sound, cleared each frame
} channel_t;

typedef struct
{
	int rate;
	int width;
	int channels;
	int loopstart;
	int samples;
	int dataofs;// chunk starts this many bytes from file start
} wavinfo_t;


/*
====================================================================

  SYSTEM SPECIFIC FUNCTIONS

====================================================================
*/

// initializes cycling through a DMA buffer and returns information on it
bool SNDDMA_Init( void );

// gets the current DMA position
int SNDDMA_GetDMAPos( void );

// shutdown the DMA xfer.
void SNDDMA_Shutdown( void );

void SNDDMA_BeginPainting( void );

void SNDDMA_Submit( void );

//====================================================================

#define MAX_CHANNELS 32
extern channel_t channels[ MAX_CHANNELS ];

extern int         paintedtime;
extern int         s_rawend;
extern vec3_t      listener_origin;
extern vec3_t      listener_forward;
extern vec3_t      listener_right;
extern vec3_t      listener_up;
extern dma_t       dma;
extern playsound_t s_pendingplays;

#define MAX_RAW_SAMPLES 8192
extern portable_samplepair_t s_rawsamples[ MAX_RAW_SAMPLES ];

extern cvar_t *s_volume;
extern cvar_t *s_nosound;
extern cvar_t *s_loadas8bit;
extern cvar_t *s_khz;
extern cvar_t *s_show;
extern cvar_t *s_mixahead;
extern cvar_t *s_testsound;
extern cvar_t *s_primary;

wavinfo_t GetWavinfo( char *name, byte *wav, int wavlength );

void S_InitScaletable( void );

sfxcache_t *S_LoadSound( sfx_t *s );

void S_IssuePlaysound( playsound_t *ps );

void S_PaintChannels( int endtime );

// picks a channel based on priorities, empty slots, number of channels
channel_t *S_PickChannel( int entnum, int entchannel );

// spatializes a channel
void S_Spatialize( channel_t *ch );
