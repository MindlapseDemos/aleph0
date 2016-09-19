/*

Name:
LOAD_UNI.C

Description:
UNIMOD (mikmod's internal format) module loader.

Portability:
All systems - all compilers (hopefully)

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mikmod.h"


BOOL UNI_Test(void)
{
	char id[4];
	if(!fread(id,4,1,modfp)) return 0;
	if(!memcmp(id,"UN05",4)) return 1;
	return 0;
}


BOOL UNI_Init(void)
{
	return 1;
}


void UNI_Cleanup(void)
{
	;
}


char *StrRead(void)
{
	char *s;
	UWORD len;

	len=_mm_read_I_UWORD(modfp);
	if(!len) return NULL;

	s=(char *)malloc(len+1);
	fread(s,len,1,modfp);
	s[len]=0;

	return s;
}


UBYTE *TrkRead(void)
{
	UBYTE *t;
	UWORD len;

	len=_mm_read_I_UWORD(modfp);
	t=(UBYTE *)malloc(len);
	fread(t,len,1,modfp);
	return t;
}



BOOL UNI_Load(void)
{
	int t,u;

	_mm_fseek(modfp,4,SEEK_SET);

	/* try to read module header */

	of.numchn	=_mm_read_UBYTE(modfp);
	of.numpos	=_mm_read_I_UWORD(modfp);
	of.reppos	=_mm_read_I_UWORD(modfp);
	of.numpat	=_mm_read_I_UWORD(modfp);
	of.numtrk	=_mm_read_I_UWORD(modfp);
	of.numins	=_mm_read_I_UWORD(modfp);
	of.initspeed=_mm_read_UBYTE(modfp);
	of.inittempo=_mm_read_UBYTE(modfp);
	_mm_read_UBYTES(of.positions,256,modfp);
	_mm_read_UBYTES(of.panning,32,modfp);
	of.flags	=_mm_read_UBYTE(modfp);

	if(feof(modfp)){
		myerr=ERROR_LOADING_HEADER;
		return 0;
	}

	of.songname=StrRead();
	of.modtype=StrRead();
	of.comment=StrRead();   /* <- new since UN01 */

/*	printf("Song: %s\nModty: %s\n",of.songname,of.modtype);
*/

	if(!AllocInstruments()) return 0;
	if(!AllocTracks()) return 0;
	if(!AllocPatterns()) return 0;

	/* Read sampleinfos */

	for(t=0;t<of.numins;t++){

		INSTRUMENT *i=&of.instruments[t];

		i->numsmp=_mm_read_UBYTE(modfp);
		_mm_read_UBYTES(i->samplenumber,96,modfp);

		i->volflg=_mm_read_UBYTE(modfp);
		i->volpts=_mm_read_UBYTE(modfp);
		i->volsus=_mm_read_UBYTE(modfp);
		i->volbeg=_mm_read_UBYTE(modfp);
		i->volend=_mm_read_UBYTE(modfp);

		for(u=0;u<12;u++){
			i->volenv[u].pos=_mm_read_I_SWORD(modfp);
			i->volenv[u].val=_mm_read_I_SWORD(modfp);
		}

		i->panflg=_mm_read_UBYTE(modfp);
		i->panpts=_mm_read_UBYTE(modfp);
		i->pansus=_mm_read_UBYTE(modfp);
		i->panbeg=_mm_read_UBYTE(modfp);
		i->panend=_mm_read_UBYTE(modfp);

		for(u=0;u<12;u++){
			i->panenv[u].pos=_mm_read_I_SWORD(modfp);
			i->panenv[u].val=_mm_read_I_SWORD(modfp);
		}

		i->vibtype	=_mm_read_UBYTE(modfp);
		i->vibsweep	=_mm_read_UBYTE(modfp);
		i->vibdepth	=_mm_read_UBYTE(modfp);
		i->vibrate	=_mm_read_UBYTE(modfp);

		i->volfade	=_mm_read_I_UWORD(modfp);
		i->insname	=StrRead();

/*		printf("Ins: %s\n",i->insname);
*/
		if(!AllocSamples(i)) return 0;

		for(u=0;u<i->numsmp;u++){

			SAMPLE *s=&i->samples[u];

			s->c2spd	= _mm_read_I_UWORD(modfp);
			s->transpose= _mm_read_SBYTE(modfp);
			s->volume	= _mm_read_UBYTE(modfp);
			s->panning	= _mm_read_UBYTE(modfp);
			s->length	= _mm_read_I_ULONG(modfp);
			s->loopstart= _mm_read_I_ULONG(modfp);
			s->loopend	= _mm_read_I_ULONG(modfp);
			s->flags	= _mm_read_I_UWORD(modfp);
			s->seekpos	= 0;

			s->samplename=StrRead();
		}
	}

	/* Read patterns */

	_mm_read_I_UWORDS(of.pattrows,of.numpat,modfp);
	_mm_read_I_UWORDS(of.patterns,of.numpat*of.numchn,modfp);

	/* Read tracks */

	for(t=0;t<of.numtrk;t++){
		of.tracks[t]=TrkRead();
	}

	return 1;
}


LOADER load_uni={
	NULL,
	"UNI",
	"Portable UNI loader v0.3",
	UNI_Init,
	UNI_Test,
	UNI_Load,
	UNI_Cleanup
};

