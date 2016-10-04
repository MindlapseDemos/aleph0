#include <stdlib.h>
#include "music.h"
#include "mikmod.h"

static void update_callback(void);

static UNIMOD *mod;
static int initialized;

static int init(void)
{
	md_mixfreq = 44100;
	md_dmabufsize = 20000;
	md_mode = DMODE_STEREO | DMODE_16BITS | DMODE_INTERP;
	md_device = 0;

	ML_RegisterLoader(&load_m15);
	ML_RegisterLoader(&load_mod);
	ML_RegisterLoader(&load_mtm);
	ML_RegisterLoader(&load_s3m);
	ML_RegisterLoader(&load_stm);
	ML_RegisterLoader(&load_ult);
	ML_RegisterLoader(&load_uni);
	ML_RegisterLoader(&load_xm);

	MD_RegisterDriver(&drv_nos);
	MD_RegisterDriver(&drv_ss);
	MD_RegisterDriver(&drv_sb);
	MD_RegisterDriver(&drv_gus);

	MD_RegisterPlayer(update_callback);

	if(!MD_Init()) {
		fprintf(stderr, "mikmod init failed: %s\n", myerr);
		return -1;
	}
	printf("using mikmod driver %s\n", md_driver->Name);
	printf(" %d bits, %s, %s mixing at %d Hz\n", md_mode & DMODE_16BITS ? 16 : 8,
			md_mode & DMODE_STEREO ? "stereo" : "mono",
			md_mode & DMODE_INTERP ? "interpolated" : "normal",
			md_mixfreq);

	atexit(MD_Exit);
	return 0;
}

int music_open(const char *fname)
{
	if(!initialized) {
		if(init() == -1) {
			return -1;
		}
		initialized = 1;
	}

	if(!(mod = ML_LoadFN((const signed char*)fname))) {
		fprintf(stderr, "failed to load music: %s: %s\n", fname, myerr);
		return -1;
	}

	MP_Init(mod);
	md_numchn = mod->numchn;
	printf("opened module %s (%d channels)\n", fname, md_numchn);
	return 0;
}

void music_close(void)
{
	if(mod) {
		printf("shutting down music playback\n");
		music_stop();
		ML_Free(mod);
		mod = 0;
	}
}

void music_play(void)
{
	MD_PlayStart();
}

void music_stop(void)
{
	MD_PlayStop();
}

void music_update(void)
{
	MD_Update();
}

static void update_callback(void)
{
	MP_HandleTick();
	MD_SetBPM(mp_bpm);
}
