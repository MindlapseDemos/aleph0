/*

Name:
LOAD_ULT.C

Description:
Ultratracker (ULT) module loader

Portability:
All systems - all compilers (hopefully)

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mikmod.h"

#define ULTS_16BITS     4
#define ULTS_LOOP       8
#define ULTS_REVERSE    16


/* Raw ULT header struct: */

typedef struct ULTHEADER{
	char  id[15];
	char  songtitle[32];
	UBYTE reserved;
} ULTHEADER;


/* Raw ULT sampleinfo struct: */

typedef struct ULTSAMPLE{
	char  samplename[32];
	char  dosname[12];
	SLONG  loopstart;
	SLONG  loopend;
	SLONG  sizestart;
	SLONG  sizeend;
	UBYTE volume;
	UBYTE flags;
	SWORD  finetune;
} ULTSAMPLE;


typedef struct ULTEVENT{
	UBYTE note,sample,eff,dat1,dat2;
} ULTEVENT;


char *ULT_Version[]={
	"Ultra Tracker V1.3",
	"Ultra Tracker V1.4",
	"Ultra Tracker V1.5",
	"Ultra Tracker V1.6"
};


BOOL ULT_Test(void)
{
	char id[15];

	if(!fread(&id,15,1,modfp)) return 0;
	return(!strncmp(id,"MAS_UTrack_V00",14));
}


BOOL ULT_Init(void)
{
	return 1;
}


void ULT_Cleanup(void)
{
}

ULTEVENT ev;



int ReadUltEvent(ULTEVENT *event)
{
	UBYTE flag,rep=1;

	flag=_mm_read_UBYTE(modfp);

	if(flag==0xfc){
		fread(&rep,1,1,modfp);
		event->note	=_mm_read_UBYTE(modfp);
	}
	else{
		event->note=flag;
	}

	event->sample	=_mm_read_UBYTE(modfp);
	event->eff		=_mm_read_UBYTE(modfp);
	event->dat1		=_mm_read_UBYTE(modfp);
	event->dat2		=_mm_read_UBYTE(modfp);

	return rep;
}




BOOL ULT_Load(void)
{
	int t,u,tracks=0;
	INSTRUMENT *d;
	SAMPLE *q;
	ULTSAMPLE s;
	ULTHEADER mh;
	UBYTE nos,noc,nop;

	/* try to read module header */

	_mm_read_str(mh.id,15,modfp);
	_mm_read_str(mh.songtitle,32,modfp);
	mh.reserved=_mm_read_UBYTE(modfp);

	if(feof(modfp)){
		myerr=ERROR_LOADING_HEADER;
		return 0;
	}

	if(mh.id[14]<'1' || mh.id[14]>'4'){
		printf("This version is not yet supported\n");
		return 0;
	}

	of.modtype=strdup(ULT_Version[mh.id[14]-'1']);
	of.initspeed=6;
	of.inittempo=125;

	/* read songtext */

	if(!ReadComment((UWORD)mh.reserved*32)) return 0;

	nos=_mm_read_UBYTE(modfp);

	if(feof(modfp)){
		myerr=ERROR_LOADING_HEADER;
		return 0;
	}

	of.songname=DupStr(mh.songtitle,32);
	of.numins=nos;

	if(!AllocInstruments()) return 0;

	d=of.instruments;

	for(t=0;t<nos;t++){

		d->numsmp=1;
		if(!AllocSamples(d)) return 0;
		q=d->samples;

		/* try to read sample info */

		_mm_read_str(s.samplename,32,modfp);
		_mm_read_str(s.dosname,12,modfp);
		s.loopstart	=_mm_read_I_ULONG(modfp);
		s.loopend	=_mm_read_I_ULONG(modfp);
		s.sizestart	=_mm_read_I_ULONG(modfp);
		s.sizeend	=_mm_read_I_ULONG(modfp);
		s.volume	=_mm_read_UBYTE(modfp);
		s.flags		=_mm_read_UBYTE(modfp);
		s.finetune	=_mm_read_I_SWORD(modfp);

		if(feof(modfp)){
			myerr=ERROR_LOADING_SAMPLEINFO;
			return 0;
		}

		d->insname=DupStr(s.samplename,32);

		q->seekpos=0;

		q->c2spd=8363;

		if(mh.id[14]>='4'){
			_mm_read_I_UWORD(modfp);	/* read 1.6 extra info(??) word */
			q->c2spd=s.finetune;
		}

		q->length=s.sizeend-s.sizestart;
		q->volume=s.volume>>2;
		q->loopstart=s.loopstart;
		q->loopend=s.loopend;

		q->flags=SF_SIGNED;

		if(s.flags&ULTS_LOOP){
			q->flags|=SF_LOOP;
		}

		if(s.flags&ULTS_16BITS){
			q->flags|=SF_16BITS;
			q->loopstart>>=1;
			q->loopend>>=1;
		}

/*      printf("Sample %d %s length %ld\n",t,d->samplename,d->length); */
		d++;
	}

	_mm_read_UBYTES(of.positions,256,modfp);

	for(t=0;t<256;t++){
		if(of.positions[t]==255) break;
	}
	of.numpos=t;

	noc=_mm_read_UBYTE(modfp);
	nop=_mm_read_UBYTE(modfp);

	of.numchn=noc+1;
	of.numpat=nop+1;
	of.numtrk=of.numchn*of.numpat;

	if(!AllocTracks()) return 0;
	if(!AllocPatterns()) return 0;

	for(u=0;u<of.numchn;u++){
		for(t=0;t<of.numpat;t++){
			of.patterns[(t*of.numchn)+u]=tracks++;
		}
	}

	/* read pan position table for v1.5 and higher */

	if(mh.id[14]>='3'){
		for(t=0;t<of.numchn;t++) of.panning[t]=_mm_read_UBYTE(modfp)<<4;
	}


	for(t=0;t<of.numtrk;t++){
		int rep,s,done;

		UniReset();
		done=0;

		while(done<64){

			rep=ReadUltEvent(&ev);

			if(feof(modfp)){
				myerr=ERROR_LOADING_TRACK;
				return 0;
			}

/*                      printf("rep %d: n %d i %d e %x d1 %d d2 %d \n",rep,ev.note,ev.sample,ev.eff,ev.dat1,ev.dat2); */


			for(s=0;s<rep;s++){
				UBYTE eff;


				if(ev.sample){
					UniInstrument(ev.sample-1);
				}

				if(ev.note){
					UniNote(ev.note+23);
				}

				eff=ev.eff>>4;


				/*
					ULT panning effect fixed by Alexander Kerkhove :
				*/


				if(eff==0xc) UniPTEffect(eff,ev.dat2>>2);
				else if(eff==0xb) UniPTEffect(8,ev.dat2*0xf);
				else UniPTEffect(eff,ev.dat2);

				eff=ev.eff&0xf;

				if(eff==0xc) UniPTEffect(eff,ev.dat1>>2);
				else if(eff==0xb) UniPTEffect(8,ev.dat1*0xf);
				else UniPTEffect(eff,ev.dat1);

				UniNewline();
				done++;
			}
		}
/*              printf("----------------"); */

		if(!(of.tracks[t]=UniDup())) return 0;
	}

/*      printf("%d channels %d patterns\n",of.numchn,of.numpat); */
/*      printf("Song %32.32s: There's %d samples\n",mh.songtitle,nos); */
	return 1;
}



LOADER load_ult={
	NULL,
	"ULT",
	"Portable ULT loader v0.1",
	ULT_Init,
	ULT_Test,
	ULT_Load,
	ULT_Cleanup
};
