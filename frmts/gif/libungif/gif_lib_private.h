#ifndef GIF_LIB_PRIVATE_H
#define GIF_LIB_PRIVATE_H

#include "gif_lib.h"

#define PROGRAM_NAME	"GIFLIB"

#define LZ_MAX_CODE	4095		/* Biggest code possible in 12 bits. */
#define LZ_BITS		12

#define FLUSH_OUTPUT		4096    /* Impossible code, to signal flush. */
#define FIRST_CODE		4097    /* Impossible code, to signal first. */
#define NO_SUCH_CODE		4098    /* Impossible code, to signal empty. */

typedef struct GifFilePrivateType {
    int FileState,
	FileHandle,			     /* Where all this data goes to! */
	BitsPerPixel,	    /* Bits per pixel (Codes uses at least this + 1). */
	ClearCode,				       /* The CLEAR LZ code. */
	EOFCode,				         /* The EOF LZ code. */
	RunningCode,		    /* The next code algorithm can generate. */
	RunningBits,/* The number of bits required to represent RunningCode. */
	MaxCode1,  /* 1 bigger than max. possible code, in RunningBits bits. */
	LastCode,		        /* The code before the current code. */
	CrntCode,				  /* Current algorithm code. */
	StackPtr,		         /* For character stack (see below). */
	CrntShiftState;		        /* Number of bits in CrntShiftDWord. */
    unsigned long CrntShiftDWord;     /* For bytes decomposition into codes. */
    unsigned long PixelCount;		       /* Number of pixels in image. */
    FILE *File;						  /* File as stream. */
    InputFunc Read;                /* function to read gif input (TVT) */
    GifByteType Buf[256];	       /* Compressed input is buffered here. */
    GifByteType Stack[LZ_MAX_CODE];	 /* Decoded pixels are stacked here. */
    GifByteType Suffix[LZ_MAX_CODE+1];	       /* So we can trace the codes. */
    unsigned int Prefix[LZ_MAX_CODE+1];
} GifFilePrivateType;

extern int _GifError;

#ifdef SYSV
static char *VersionStr =
        "Gif library module,\t\tGershon Elber\n\
	(C) Copyright 1989 Gershon Elber, Non commercial use only.\n";
#else
static char *VersionStr =
	PROGRAM_NAME
	"	IBMPC "
	GIF_LIB_VERSION
	"	Gershon Elber,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1989 Gershon Elber, Non commercial use only.\n";
#endif /* SYSV */
static char *vs_aw() { return( vs_aw() ? ((char *) NULL ) : VersionStr ); }

#endif /* GIF_LIB_PRIVATE_H */
