/*

Name:
DRV_NOS.C

Description:
Mikmod driver for no output on any soundcard, monitor, keyboard, or whatever :)

Portability:
All systems - All compilers

*/
#include <stdio.h>
#include "mikmod.h"


BOOL NS_IsThere(void)
{
	return 1;
}


SWORD NS_SampleLoad(FILE *fp,ULONG s,ULONG a,ULONG b,UWORD f)
{
	return 1;
}


void NS_SampleUnload(SWORD h)
{
}


BOOL NS_Init(void)
{
	return 1;
}


void NS_Exit(void)
{
}


void NS_PlayStart(void)
{
}


void NS_PlayStop(void)
{
}


void NS_Update(void)
{
}


void NS_VoiceSetVolume(UBYTE voice,UBYTE vol)
{
}


void NS_VoiceSetFrequency(UBYTE voice,ULONG frq)
{
}


void NS_VoiceSetPanning(UBYTE voice,UBYTE pan)
{
}


void NS_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags)
{
}


DRIVER drv_nos={
	NULL,
	"No Sound",
	"MikMod Nosound Driver v2.0 - (c) Creative Silence",
	NS_IsThere,
	NS_SampleLoad,
	NS_SampleUnload,
	NS_Init,
	NS_Exit,
	NS_PlayStart,
	NS_PlayStop,
	NS_Update,
	NS_VoiceSetVolume,
	NS_VoiceSetFrequency,
	NS_VoiceSetPanning,
	NS_VoicePlay
};
