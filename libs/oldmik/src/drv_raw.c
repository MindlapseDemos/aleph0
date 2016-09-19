/*

Name:
DRV_RAW.C

Description:
Mikmod driver for output to a file called MUSIC.RAW

!! DO NOT CALL MD_UPDATE FROM A INTERRUPT IF YOU USE THIS DRIVER !!

Portability:

MSDOS:	BC(y)	Watcom(y)	DJGPP(y)
Win95:	BC(y)
Linux:	y

(y) - yes
(n) - no (not possible or not useful)
(?) - may be possible, but not tested

*/
#include <stdio.h>
#include <stdlib.h>

#include "mikmod.h"

#define RAWBUFFERSIZE 8192

static FILE *rawout;

static char RAW_DMABUF[RAWBUFFERSIZE];


static BOOL RAW_IsThere(void)
{
	return 1;
}


static BOOL RAW_Init(void)
{
	if(!(rawout=fopen("music.raw","wb"))){
		myerr="Couldn't open output file 'music.raw'";
		return 0;
	}

	if(!VC_Init()){
		fclose(rawout);
		return 0;
	}

	return 1;
}



static void RAW_Exit(void)
{
	VC_Exit();
	fclose(rawout);
}


static void RAW_Update(void)
{
	VC_WriteBytes(RAW_DMABUF,RAWBUFFERSIZE);
	fwrite(RAW_DMABUF,RAWBUFFERSIZE,1,rawout);
}


DRIVER drv_raw={
	NULL,
	"music.raw file",
	"MikMod music.raw file output driver v1.0",
	RAW_IsThere,
	VC_SampleLoad,
	VC_SampleUnload,
	RAW_Init,
	RAW_Exit,
	VC_PlayStart,
	VC_PlayStop,
	RAW_Update,
	VC_VoiceSetVolume,
	VC_VoiceSetFrequency,
	VC_VoiceSetPanning,
	VC_VoicePlay
};
