/*

Name:
LOAD_XM.C

Description:
Fasttracker (XM) module loader

Portability:
All systems - all compilers (hopefully)

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include "mikmod.h"

/**************************************************************************
**************************************************************************/


typedef struct XMHEADER{
	char  id[17];                   /* ID text: 'Extended module: ' */
	char  songname[21];             /* Module name, padded with zeroes and 0x1a at the end */
	char  trackername[20];  /* Tracker name */
	UWORD version;                  /* (word) Version number, hi-byte major and low-byte minor */
	ULONG headersize;               /* Header size */
	UWORD songlength;               /* (word) Song length (in patten order table) */
	UWORD restart;                  /* (word) Restart position */
	UWORD numchn;                   /* (word) Number of channels (2,4,6,8,10,...,32) */
	UWORD numpat;                   /* (word) Number of patterns (max 256) */
	UWORD numins;                   /* (word) Number of instruments (max 128) */
	UWORD flags;                    /* (word) Flags: bit 0: 0 = Amiga frequency table (see below) 1 = Linear frequency table */
	UWORD tempo;                    /* (word) Default tempo */
	UWORD bpm;                              /* (word) Default BPM */
	UBYTE orders[256];              /* (byte) Pattern order table */
} XMHEADER;


typedef struct XMINSTHEADER{
	ULONG size;                             /* (dword) Instrument size */
	char  name[22];                 /* (char) Instrument name */
	UBYTE type;                             /* (byte) Instrument type (always 0) */
	UWORD numsmp;                   /* (word) Number of samples in instrument */
	ULONG ssize;                    /* */
} XMINSTHEADER;


typedef struct XMPATCHHEADER{
	UBYTE what[96];         /* (byte) Sample number for all notes */
	UBYTE volenv[48];       /* (byte) Points for volume envelope */
	UBYTE panenv[48];       /* (byte) Points for panning envelope */
	UBYTE volpts;           /* (byte) Number of volume points */
	UBYTE panpts;           /* (byte) Number of panning points */
	UBYTE volsus;           /* (byte) Volume sustain point */
	UBYTE volbeg;           /* (byte) Volume loop start point */
	UBYTE volend;           /* (byte) Volume loop end point */
	UBYTE pansus;           /* (byte) Panning sustain point */
	UBYTE panbeg;           /* (byte) Panning loop start point */
	UBYTE panend;           /* (byte) Panning loop end point */
	UBYTE volflg;           /* (byte) Volume type: bit 0: On; 1: Sustain; 2: Loop */
	UBYTE panflg;           /* (byte) Panning type: bit 0: On; 1: Sustain; 2: Loop */
	UBYTE vibflg;           /* (byte) Vibrato type */
	UBYTE vibsweep;         /* (byte) Vibrato sweep */
	UBYTE vibdepth;         /* (byte) Vibrato depth */
	UBYTE vibrate;          /* (byte) Vibrato rate */
	UWORD volfade;          /* (word) Volume fadeout */
	UWORD reserved[11];     /* (word) Reserved */
} XMPATCHHEADER;


typedef struct XMWAVHEADER{
	ULONG length;           /* (dword) Sample length */
	ULONG loopstart;        /* (dword) Sample loop start */
	ULONG looplength;       /* (dword) Sample loop length */
	UBYTE volume;           /* (byte) Volume */
	SBYTE finetune;          /* (byte) Finetune (signed byte -128..+127) */
	UBYTE type;                     /* (byte) Type: Bit 0-1: 0 = No loop, 1 = Forward loop, */
/*                                        2 = Ping-pong loop; */
/*                                        4: 16-bit sampledata */
	UBYTE panning;          /* (byte) Panning (0-255) */
	SBYTE  relnote;          /* (byte) Relative note number (signed byte) */
	UBYTE reserved;         /* (byte) Reserved */
	char  samplename[22];   /* (char) Sample name */
} XMWAVHEADER;


typedef struct XMPATHEADER{
	ULONG size;                             /* (dword) Pattern header length */
	UBYTE packing;                  /* (byte) Packing type (always 0) */
	UWORD numrows;                  /* (word) Number of rows in pattern (1..256) */
	UWORD packsize;                 /* (word) Packed patterndata size */
} XMPATHEADER;

typedef struct MTMNOTE{
	UBYTE a,b,c;
} MTMNOTE;


typedef struct XMNOTE{
	UBYTE note,ins,vol,eff,dat;
}XMNOTE;

XMNOTE *xmpat;

/**************************************************************************
**************************************************************************/



static XMHEADER *mh;

char XM_Version[]="XM";



BOOL XM_Test(void)
{
	char id[17];
	if(!fread(id,17,1,modfp)) return 0;
	if(!memcmp(id,"Extended Module: ",17)) return 1;
	return 0;
}


BOOL XM_Init(void)
{
	mh=NULL;
	if(!(mh=(XMHEADER *)MyCalloc(1,sizeof(XMHEADER)))) return 0;
	return 1;
}


void XM_Cleanup(void)
{
	if(mh!=NULL) free(mh);
}


void XM_ReadNote(XMNOTE *n)
{
	UBYTE cmp;
	memset(n,0,sizeof(XMNOTE));

	cmp=fgetc(modfp);

	if(cmp&0x80){
		if(cmp&1) n->note=fgetc(modfp);
		if(cmp&2) n->ins=fgetc(modfp);
		if(cmp&4) n->vol=fgetc(modfp);
		if(cmp&8) n->eff=fgetc(modfp);
		if(cmp&16) n->dat=fgetc(modfp);
	}
	else{
		n->note=cmp;
		n->ins=fgetc(modfp);
		n->vol=fgetc(modfp);
		n->eff=fgetc(modfp);
		n->dat=fgetc(modfp);
	}
}


UBYTE *XM_Convert(XMNOTE *xmtrack,UWORD rows)
{
	int t;
	UBYTE note,ins,vol,eff,dat;

	UniReset();

	for(t=0;t<rows;t++){

		note=xmtrack->note;
		ins=xmtrack->ins;
		vol=xmtrack->vol;
		eff=xmtrack->eff;
		dat=xmtrack->dat;

                if(note!=0) UniNote(note-1);

                if(ins!=0) UniInstrument(ins-1);

/*              printf("Vol:%d\n",vol); */

		switch(vol>>4){

			case 0x6:					/* volslide down */
				if(vol&0xf){
					UniWrite(UNI_XMEFFECTA);
					UniWrite(vol&0xf);
				}
				break;

			case 0x7:					/* volslide up */
				if(vol&0xf){
					UniWrite(UNI_XMEFFECTA);
					UniWrite(vol<<4);
				}
				break;

			/* volume-row fine volume slide is compatible with protracker
			   EBx and EAx effects i.e. a zero nibble means DO NOT SLIDE, as
			   opposed to 'take the last sliding value'.
			*/

			case 0x8:						/* finevol down */
				UniPTEffect(0xe,0xb0 | (vol&0xf));
				break;

			case 0x9:                       /* finevol up */
				UniPTEffect(0xe,0xa0 | (vol&0xf));
				break;

			case 0xa:                       /* set vibrato speed */
				UniPTEffect(0x4,vol<<4);
				break;

			case 0xb:                       /* vibrato */
				UniPTEffect(0x4,vol&0xf);
				break;

			case 0xc:                       /* set panning */
				UniPTEffect(0x8,vol<<4);
				break;

			case 0xd:                       /* panning slide left */
				/* only slide when data nibble not zero: */

				if(vol&0xf){
					UniWrite(UNI_XMEFFECTP);
					UniWrite(vol&0xf);
				}
				break;

			case 0xe:                       /* panning slide right */
				/* only slide when data nibble not zero: */

				if(vol&0xf){
					UniWrite(UNI_XMEFFECTP);
					UniWrite(vol<<4);
				}
				break;

			case 0xf:                       /* tone porta */
				UniPTEffect(0x3,vol<<4);
				break;

			default:
				if(vol>=0x10 && vol<=0x50){
					UniPTEffect(0xc,vol-0x10);
				}
		}

/*              if(eff>0xf) printf("Effect %d",eff); */

		switch(eff){

			case 'G'-55:                    /* G - set global volume */
				if(dat>64) dat=64;
				UniWrite(UNI_XMEFFECTG);
				UniWrite(dat);
				break;

			case 'H'-55:                    /* H - global volume slide */
				UniWrite(UNI_XMEFFECTH);
				UniWrite(dat);
				break;

			case 'K'-55:                    /* K - keyoff */
				UniNote(96);
				break;

			case 'L'-55:                    /* L - set envelope position */
				break;

			case 'P'-55:                    /* P - panning slide */
				UniWrite(UNI_XMEFFECTP);
				UniWrite(dat);
				break;

			case 'R'-55:                    /* R - multi retrig note */
				UniWrite(UNI_S3MEFFECTQ);
				UniWrite(dat);
				break;

			case 'T'-55:             		/* T - Tremor !! (== S3M effect I) */
				UniWrite(UNI_S3MEFFECTI);
				UniWrite(dat);
				break;

			case 'X'-55:
				if((dat>>4)==1){                /* X1 extra fine porta up */


				}
				else{                                   /* X2 extra fine porta down */

				}
				break;

			default:
				if(eff==0xa){
					UniWrite(UNI_XMEFFECTA);
					UniWrite(dat);
				}
				else if(eff<=0xf) UniPTEffect(eff,dat);
				break;
		}

		UniNewline();
		xmtrack++;
	}
	return UniDup();
}



BOOL XM_Load(void)
{
	INSTRUMENT *d;
	SAMPLE *q;
	int t,u,v,p,numtrk;
	long next;

	/* try to read module header */

	_mm_read_str(mh->id,17,modfp);
	_mm_read_str(mh->songname,21,modfp);
	_mm_read_str(mh->trackername,20,modfp);
	mh->version		=_mm_read_I_UWORD(modfp);
	mh->headersize	=_mm_read_I_ULONG(modfp);
	mh->songlength	=_mm_read_I_UWORD(modfp);
	mh->restart		=_mm_read_I_UWORD(modfp);
	mh->numchn		=_mm_read_I_UWORD(modfp);
	mh->numpat		=_mm_read_I_UWORD(modfp);
	mh->numins		=_mm_read_I_UWORD(modfp);
	mh->flags		=_mm_read_I_UWORD(modfp);
	mh->tempo		=_mm_read_I_UWORD(modfp);
	mh->bpm			=_mm_read_I_UWORD(modfp);
	_mm_read_UBYTES(mh->orders,256,modfp);

	if(feof(modfp)){
		myerr = ERROR_LOADING_HEADER;
		return 0;
	}

	/* set module variables */

	of.initspeed=mh->tempo;
	of.inittempo=mh->bpm;
	of.modtype=DupStr(mh->trackername,20);
	of.numchn=mh->numchn;
	of.numpat=mh->numpat;
	of.numtrk=(UWORD)of.numpat*of.numchn;   /* get number of channels */
	of.songname=DupStr(mh->songname,20);    /* make a cstr of songname */
	of.numpos=mh->songlength;                       /* copy the songlength */
	of.reppos=mh->restart;
	of.numins=mh->numins;
	of.flags|=UF_XMPERIODS;
	if(mh->flags&1) of.flags|=UF_LINEAR;

	memcpy(of.positions,mh->orders,256);

/*
        WHY THIS CODE HERE?? I CAN'T REMEMBER!

        of.numpat=0;
	for(t=0;t<of.numpos;t++){
		if(of.positions[t]>of.numpat) of.numpat=of.positions[t];
	}
	of.numpat++;
*/

/*	printf("Modtype :%s\n",of.modtype);
	printf("Version :%x\n",mh->version);
	printf("Song    :%s\n",of.songname);
	printf("Speed   :%d,%d\n",of.initspeed,of.inittempo);
	printf("Channels:%d\n",of.numchn);
	printf("Numins  :%d\n",mh->numins);
*/
	if(!AllocTracks()) return 0;
	if(!AllocPatterns()) return 0;

	numtrk=0;
	for(t=0;t<mh->numpat;t++){
		XMPATHEADER ph;

/*		printf("Reading pattern %d\n",t); */

		ph.size		=_mm_read_I_ULONG(modfp);
		ph.packing	=_mm_read_UBYTE(modfp);
		ph.numrows	=_mm_read_I_UWORD(modfp);
		ph.packsize	=_mm_read_I_UWORD(modfp);

/*		printf("headln:  %ld\n",ph.size); */
/*		printf("numrows: %d\n",ph.numrows); */
/*		printf("packsize:%d\n",ph.packsize); */

                of.pattrows[t]=ph.numrows;

		/*
			Gr8.. when packsize is 0, don't try to load a pattern.. it's empty.
			This bug was discovered thanks to Khyron's module..
		*/

		if(!(xmpat=(XMNOTE *)MyCalloc(ph.numrows*of.numchn,sizeof(XMNOTE)))) return 0;

		if(ph.packsize>0){
			for(u=0;u<ph.numrows;u++){
				for(v=0;v<of.numchn;v++){
					XM_ReadNote(&xmpat[(v*ph.numrows)+u]);
				}
			}
		}

		for(v=0;v<of.numchn;v++){
			of.tracks[numtrk++]=XM_Convert(&xmpat[v*ph.numrows],ph.numrows);
		}

		free(xmpat);
	}

	if(!AllocInstruments()) return 0;

	d=of.instruments;

	for(t=0;t<of.numins;t++){
		XMINSTHEADER ih;

		/* read instrument header */

		ih.size		=_mm_read_I_ULONG(modfp);
		_mm_read_str (ih.name, 22, modfp);
		ih.type		=_mm_read_UBYTE(modfp);
		ih.numsmp	=_mm_read_I_UWORD(modfp);
		ih.ssize	=_mm_read_I_ULONG(modfp);

/*      printf("Size: %ld\n",ih.size);
		printf("Name: 	%22.22s\n",ih.name);
		printf("Samples:%d\n",ih.numsmp);
		printf("sampleheadersize:%ld\n",ih.ssize);
*/
		d->insname=DupStr(ih.name,22);
		d->numsmp=ih.numsmp;

		if(!AllocSamples(d)) return 0;

		if(ih.numsmp>0){
			XMPATCHHEADER pth;
			XMWAVHEADER wh;

			_mm_read_UBYTES (pth.what, 96, modfp);
			_mm_read_UBYTES (pth.volenv, 48, modfp);
			_mm_read_UBYTES (pth.panenv, 48, modfp);
			pth.volpts		=_mm_read_UBYTE(modfp);
			pth.panpts		=_mm_read_UBYTE(modfp);
			pth.volsus		=_mm_read_UBYTE(modfp);
			pth.volbeg		=_mm_read_UBYTE(modfp);
			pth.volend		=_mm_read_UBYTE(modfp);
			pth.pansus		=_mm_read_UBYTE(modfp);
			pth.panbeg		=_mm_read_UBYTE(modfp);
			pth.panend		=_mm_read_UBYTE(modfp);
			pth.volflg		=_mm_read_UBYTE(modfp);
			pth.panflg		=_mm_read_UBYTE(modfp);
			pth.vibflg		=_mm_read_UBYTE(modfp);
			pth.vibsweep	=_mm_read_UBYTE(modfp);
			pth.vibdepth	=_mm_read_UBYTE(modfp);
			pth.vibrate		=_mm_read_UBYTE(modfp);
			pth.volfade		=_mm_read_I_UWORD(modfp);
			_mm_read_I_UWORDS(pth.reserved, 11, modfp);

			memcpy(d->samplenumber,pth.what,96);

			d->volfade=pth.volfade;

/*			printf("Volfade %x\n",d->volfade); */

			memcpy(d->volenv,pth.volenv,24);
			d->volflg=pth.volflg;
			d->volsus=pth.volsus;
			d->volbeg=pth.volbeg;
			d->volend=pth.volend;
			d->volpts=pth.volpts;

/*			printf("volume points	: %d\n"
				   "volflg			: %d\n"
				   "volbeg			: %d\n"
				   "volend			: %d\n"
				   "volsus			: %d\n",
				   d->volpts,
				   d->volflg,
				   d->volbeg,
				   d->volend,
				   d->volsus);
*/
			/* scale volume envelope: */

			for(p=0;p<12;p++){
				d->volenv[p].val<<=2;
/*				printf("%d,%d,",d->volenv[p].pos,d->volenv[p].val); */
			}

			memcpy(d->panenv,pth.panenv,24);
			d->panflg=pth.panflg;
			d->pansus=pth.pansus;
			d->panbeg=pth.panbeg;
			d->panend=pth.panend;
			d->panpts=pth.panpts;

/*					  printf("Panning points	: %d\n"
				   "panflg			: %d\n"
				   "panbeg			: %d\n"
				   "panend			: %d\n"
				   "pansus			: %d\n",
				   d->panpts,
				   d->panflg,
				   d->panbeg,
				   d->panend,
				   d->pansus);
*/
			/* scale panning envelope: */

			for(p=0;p<12;p++){
				d->panenv[p].val<<=2;
/*				printf("%d,%d,",d->panenv[p].pos,d->panenv[p].val); */
			}

/*                      for(u=0;u<256;u++){ */
/*                              printf("%2.2x ",fgetc(modfp)); */
/*                      } */

			next=0;

			for(u=0;u<ih.numsmp;u++){
				q=&d->samples[u];

				wh.length		=_mm_read_I_ULONG (modfp);
				wh.loopstart	=_mm_read_I_ULONG (modfp);
				wh.looplength	=_mm_read_I_ULONG (modfp);
				wh.volume		=_mm_read_UBYTE (modfp);
				wh.finetune		=_mm_read_SBYTE (modfp);
				wh.type			=_mm_read_UBYTE (modfp);
				wh.panning		=_mm_read_UBYTE (modfp);
				wh.relnote		=_mm_read_SBYTE (modfp);
				wh.reserved		=_mm_read_UBYTE (modfp);
				_mm_read_str(wh.samplename, 22, modfp);

/*              printf("wav %d:%22.22s\n",u,wh.samplename); */

				q->samplename   =DupStr(wh.samplename,22);
				q->length       =wh.length;
				q->loopstart    =wh.loopstart;
				q->loopend      =wh.loopstart+wh.looplength;
				q->volume       =wh.volume;
				q->c2spd		=wh.finetune+128;
				q->transpose    =wh.relnote;
				q->panning      =wh.panning;
				q->seekpos		=next;

				if(wh.type&0x10){
					q->length>>=1;
					q->loopstart>>=1;
					q->loopend>>=1;
				}

				next+=wh.length;

/*                              printf("Type %u\n",wh.type); */
/*				printf("Trans %d\n",wh.relnote); */

				q->flags|=SF_OWNPAN;
				if(wh.type&0x3) q->flags|=SF_LOOP;
				if(wh.type&0x2) q->flags|=SF_BIDI;

				if(wh.type&0x10) q->flags|=SF_16BITS;
				q->flags|=SF_DELTA;
				q->flags|=SF_SIGNED;
			}

			for(u=0;u<ih.numsmp;u++) d->samples[u].seekpos+=_mm_ftell(modfp);

			_mm_fseek(modfp,next,SEEK_CUR);
		}

		d++;
	}


	return 1;
}


LOADER load_xm={
	NULL,
	"XM",
	"Portable XM loader v0.4 - for your ears only / MikMak",
	XM_Init,
	XM_Test,
	XM_Load,
	XM_Cleanup
};
