/*

Name:
LOAD_M15.C

Description:
15 instrument MOD loader

Portability:
All systems - all compilers (hopefully)

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mikmod.h"

/*************************************************************************
*************************************************************************/


typedef struct MSAMPINFO{       /* sample header as it appears in a module */
        char  samplename[22];
	UWORD length;
	UBYTE finetune;
	UBYTE volume;
	UWORD reppos;
	UWORD replen;
} MSAMPINFO;


typedef struct MODULEHEADER{                 /* verbatim module header */
        char       songname[20];                /* the songname.. */
	MSAMPINFO  samples[15];                         /* all sampleinfo */
	UBYTE      songlength;                          /* number of patterns used */
	UBYTE      magic1;                                      /* should be 127 */
	UBYTE      positions[128];                      /* which pattern to play at pos */
} MODULEHEADER;


typedef struct MODNOTE{
	UBYTE a,b,c,d;
} MODNOTE;


/*************************************************************************
*************************************************************************/

static MODULEHEADER *mh;        /* raw as-is module header */
static MODNOTE *patbuf;


static BOOL LoadModuleHeader(MODULEHEADER *mh)
{
	int t;

	_mm_read_str(mh->songname,20,modfp);

	for(t=0;t<15;t++){
		MSAMPINFO *s= &mh->samples[t];
		_mm_read_str(s->samplename,22,modfp);
		s->length	=_mm_read_M_UWORD(modfp);
		s->finetune	=_mm_read_UBYTE(modfp);
		s->volume	=_mm_read_UBYTE(modfp);
		s->reppos	=_mm_read_M_UWORD(modfp);
		s->replen	=_mm_read_M_UWORD(modfp);
	}

	mh->songlength	=_mm_read_UBYTE(modfp);
	mh->magic1		=_mm_read_UBYTE(modfp);                                      /* should be 127 */
	_mm_read_UBYTES(mh->positions,128,modfp);

	return(!feof(modfp));
}



BOOL M15_Test(void)
{
	int t;
	MODULEHEADER mh;

	if(!LoadModuleHeader(&mh)) return 0;

	for(t=0;t<15;t++){

		/* all finetunes should be zero */
		if(mh.samples[t].finetune!=0) return 0;

		/* all volumes should be <=64 */
		if(mh.samples[t].volume>64) return 0;
	}
	if(mh.magic1>127) return 0;    /* and magic1 should be <128 */

	return 1;
}


BOOL M15_Init(void)
{
	patbuf=NULL;
	if(!(mh=(MODULEHEADER *)MyCalloc(1,sizeof(MODULEHEADER)))) return 0;
	return 1;
}


void M15_Cleanup(void)
{
	if(mh!=NULL) free(mh);
	if(patbuf!=NULL) free(patbuf);
}


/*

Old (amiga) noteinfo:

 _____byte 1_____   byte2_    _____byte 3_____   byte4_
/                \ /      \  /                \ /      \
0000          0000-00000000  0000          0000-00000000

Upper four    12 bits for    Lower four    Effect command.
bits of sam-  note period.   bits of sam-
ple number.                  ple number.


*/


UWORD M15_npertab[60]={

/* -> Tuning 0 */

	1712,1616,1524,1440,1356,1280,1208,1140,1076,1016,960,906,
	856,808,762,720,678,640,604,570,538,508,480,453,
	428,404,381,360,339,320,302,285,269,254,240,226,
	214,202,190,180,170,160,151,143,135,127,120,113,
	107,101,95,90,85,80,75,71,67,63,60,56
};


void M15_ConvertNote(MODNOTE *n)
{
	UBYTE instrument,effect,effdat,note;
	UWORD period;

	/* extract the various information from the 4 bytes that
	   make up a single note */

	instrument=(n->a&0x10)|(n->c>>4);
	period=(((UWORD)n->a&0xf)<<8)+n->b;
	effect=n->c&0xf;
	effdat=n->d;

	/* Convert the period to a note number */

	note=0;
	if(period!=0){
		for(note=0;note<60;note++){
			if(period>=M15_npertab[note]) break;
		}
		note++;
		if(note==61) note=0;
	}

	if(instrument!=0){
		UniInstrument(instrument-1);
	}

	if(note!=0){
		UniNote(note+23);
	}

	UniPTEffect(effect,effdat);
}



UBYTE *M15_ConvertTrack(MODNOTE *n)
{
	int t;

	UniReset();
	for(t=0;t<64;t++){
		M15_ConvertNote(n);
		UniNewline();
		n+=of.numchn;
	}
	return UniDup();
}



BOOL M15_LoadPatterns(void)
/*
	Loads all patterns of a modfile and converts them into the
	3 byte format.
*/
{
	int t,s,tracks=0;

	if(!AllocPatterns()) return 0;
	if(!AllocTracks()) return 0;

	/* Allocate temporary buffer for loading
	   and converting the patterns */

	if(!(patbuf=(MODNOTE *)MyCalloc(64U*of.numchn,sizeof(MODNOTE)))) return 0;

	for(t=0;t<of.numpat;t++){

		/* Load the pattern into the temp buffer
		   and convert it */

		for(s=0;s<(64U*of.numchn);s++){
			patbuf[s].a=_mm_read_UBYTE(modfp);
			patbuf[s].b=_mm_read_UBYTE(modfp);
			patbuf[s].c=_mm_read_UBYTE(modfp);
			patbuf[s].d=_mm_read_UBYTE(modfp);
		}

		for(s=0;s<of.numchn;s++){
			if(!(of.tracks[tracks++]=M15_ConvertTrack(patbuf+s))) return 0;
		}
	}

	return 1;
}



BOOL M15_Load(void)
{
	int t;
	INSTRUMENT *d;          /* new sampleinfo structure */
	SAMPLE *q;
	MSAMPINFO *s;           /* old module sampleinfo */

	/* try to read module header */

	if(!LoadModuleHeader(mh)){
		myerr=ERROR_LOADING_HEADER;
		return 0;
	}

	/* set module variables */

	of.initspeed=6;
	of.inittempo=125;
	of.numchn=4;                                                    /* get number of channels */
	of.modtype=strdup("15-instrument");             /* get ascii type of mod */
	of.songname=DupStr(mh->songname,20);        /* make a cstr of songname */
	of.numpos=mh->songlength;                       /* copy the songlength */
	memcpy(of.positions,mh->positions,128);     /* copy the position array */

	/* Count the number of patterns */

	of.numpat=0;

	for(t=0;t<128;t++){             /* <-- BUGFIX... have to check ALL positions */
		if(of.positions[t] > of.numpat){
			of.numpat=of.positions[t];
		}
	}
	of.numpat++;
	of.numtrk=of.numpat*of.numchn;

	/* Finally, init the sampleinfo structures */

	of.numins=15;
	if(!AllocInstruments()) return 0;

	s=mh->samples;          /* init source pointer */
	d=of.instruments;       /* init dest pointer */

	for(t=0;t<of.numins;t++){

		d->numsmp=1;
		if(!AllocSamples(d)) return 0;

		q=d->samples;

		/* convert the samplename */

		d->insname=DupStr(s->samplename,22);

		/* init the sampleinfo variables and
		   convert the size pointers to longword format */

		q->c2spd=finetune[s->finetune&0xf];
		q->volume=s->volume;
		q->loopstart=s->reppos;
		q->loopend=q->loopstart+(s->replen<<1);
		q->length=s->length<<1;
		q->seekpos=0;

		q->flags=SF_SIGNED;
		if(s->replen>1) q->flags|=SF_LOOP;

		/* fix replen if repend>length */

		if(q->loopend>q->length) q->loopend=q->length;

		s++;    /* point to next source sampleinfo */
		d++;    /* point to next destiny sampleinfo */
	}

	if(!M15_LoadPatterns()) return 0;
	return 1;
}



LOADER load_m15={
	NULL,
	"15-instrument module",
	"Portable MOD-15 loader v0.1",
	M15_Init,
	M15_Test,
	M15_Load,
	M15_Cleanup
};
