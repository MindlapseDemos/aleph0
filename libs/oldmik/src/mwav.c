/*

Name:
MWAV.C

Description:
WAV sample loader

Portability:

MSDOS:	BC(y)	Watcom(y)	DJGPP(?)
Win95:	BC(n)
Linux:	n

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mikmod.h"


typedef struct WAV{
	char  rID[4];
	ULONG rLen;
	char  wID[4];
	char  fID[4];
	ULONG fLen;
	UWORD wFormatTag;
	UWORD nChannels;
	ULONG nSamplesPerSec;
	ULONG nAvgBytesPerSec;
	UWORD nBlockAlign;
	UWORD nFormatSpecific;
} WAV;



SAMPLE *MW_LoadWavFP(FILE *fp)
{
	SAMPLE *si;
	static WAV wh;
	static char dID[4];

	_mm_rewind(fp);

	/* read wav header */

        _mm_read_str(wh.rID,4,fp);
	wh.rLen=_mm_read_I_ULONG(fp);
        _mm_read_str(wh.wID,4,fp);
        _mm_read_str(wh.fID,4,fp);
	wh.fLen=_mm_read_I_ULONG(fp);
	wh.wFormatTag=_mm_read_I_UWORD(fp);
	wh.nChannels=_mm_read_I_UWORD(fp);
	wh.nSamplesPerSec=_mm_read_I_ULONG(fp);
	wh.nAvgBytesPerSec=_mm_read_I_ULONG(fp);
	wh.nBlockAlign=_mm_read_I_UWORD(fp);
	wh.nFormatSpecific=_mm_read_I_UWORD(fp);

	/* check it */

	if( feof(fp) ||
		memcmp(wh.rID,"RIFF",4) ||
		memcmp(wh.wID,"WAVE",4) ||
		memcmp(wh.fID,"fmt ",4) ){
		myerr="Not a WAV file";
		return NULL;
	}

	/* skip other crap */

	_mm_fseek(fp,wh.fLen-16,SEEK_CUR);
        _mm_read_str(dID,4,fp);

	if( memcmp(dID,"data",4) ){
		myerr="Not a WAV file";
		return NULL;
	}

	if(wh.nChannels>1){
		myerr="Only mono WAV's are supported";
		return NULL;
	}

/*  printf("wFormatTag: %x\n",wh.wFormatTag); */
/*  printf("blockalign: %x\n",wh.nBlockAlign); */
/*  prinff("nFormatSpc: %x\n",wh.nFormatSpecific); */

        if((si=(SAMPLE *)calloc(1,sizeof(SAMPLE)))==NULL){
		myerr="Out of memory";
		return NULL;
	};

	si->c2spd=8192;
	si->volume=64;

	si->length=_mm_read_I_ULONG(fp);

	if(wh.nBlockAlign==2){
		si->flags=SF_16BITS|SF_SIGNED;
		si->length>>=1;
	}

	si->handle=MD_SampleLoad(fp,si->length,0,si->length,si->flags);

	if(si->handle<0){
		free(si);
		return NULL;
	}

	return si;
}


SAMPLE *MW_LoadWavFN(char *filename)
{
	FILE *fp;
	SAMPLE *si;

	if((fp=fopen(filename,"rb"))==NULL){
		myerr="Couldn't open wav file";
		return NULL;
	}

	si=MW_LoadWavFP(fp);

	fclose(fp);
	return si;
}


void MW_FreeWav(SAMPLE *si)
{
	if(si!=NULL){
		MD_SampleUnLoad(si->handle);
		free(si);
	}
}
