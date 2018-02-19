#include <stdlib.h>
#include "music.h"
#include "mikmod.h"

#ifdef __WATCOMC__
#define USE_OLDMIK
typedef UNIMOD MODULE;

static void update_callback(void);
static void MikMod_RegisterAllDrivers(void);
static void MikMod_RegisterAllLoaders(void);
#else
#define USE_NEWMIK
#endif

static MODULE *mod;
static int initialized;

static int init(void)
{
	MikMod_RegisterAllDrivers();
	MikMod_RegisterAllLoaders();

#ifdef USE_NEWMIK
	md_mode |= DMODE_SOFT_MUSIC | DMODE_16BITS | DMODE_STEREO | DMODE_INTERP;
	if(MikMod_Init("") != 0) {
		fprintf(stderr, "mikmod init failed: %s\n",
				MikMod_strerror(MikMod_errno));
		return -1;
	}

#else
	md_mixfreq = 44100;
	md_dmabufsize = 20000;
	md_mode = DMODE_STEREO | DMODE_16BITS | DMODE_INTERP;
	md_device = 0;

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
#endif
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

#ifdef USE_NEWMIK
	if(!(mod = Player_Load(fname, 64, 0))) {
		fprintf(stderr, "failed to load music: %s: %s\n", fname,
				MikMod_strerror(MikMod_errno));
		return -1;
	}
#else
	if(!(mod = ML_LoadFN((char*)fname))) {
		fprintf(stderr, "failed to load music: %s: %s\n", fname, myerr);
		return -1;
	}

	MP_Init(mod);
	md_numchn = mod->numchn;
	printf("opened module %s (%d channels)\n", fname, md_numchn);
#endif
	return 0;
}

void music_close(void)
{
	if(mod) {
		printf("shutting down music playback\n");
		music_stop();
#ifdef USE_NEWMIK
		Player_Free(mod);
#else
		ML_Free(mod);
#endif
		mod = 0;
	}
}

void music_play(void)
{
#ifdef USE_NEWMIK
	Player_Start(mod);
#else
	MD_PlayStart();
#endif
}

void music_stop(void)
{
#ifdef USE_NEWMIK
	Player_Stop();
#else
	MD_PlayStop();
#endif
}

void music_update(void)
{
#ifdef USE_NEWMIK
	if(Player_Active()) {
		MikMod_Update();
	}
#else
	MD_Update();
#endif
}

#ifdef USE_OLDMIK
static void update_callback(void)
{
	MP_HandleTick();
	MD_SetBPM(mp_bpm);
}

static void MikMod_RegisterAllDrivers(void)
{
	MD_RegisterDriver(&drv_nos);
	MD_RegisterDriver(&drv_ss);
	MD_RegisterDriver(&drv_sb);
	MD_RegisterDriver(&drv_gus);
}

static void MikMod_RegisterAllLoaders(void)
{
	ML_RegisterLoader(&load_m15);
	ML_RegisterLoader(&load_mod);
	ML_RegisterLoader(&load_mtm);
	ML_RegisterLoader(&load_s3m);
	ML_RegisterLoader(&load_stm);
	ML_RegisterLoader(&load_ult);
	ML_RegisterLoader(&load_uni);
	ML_RegisterLoader(&load_xm);
}
#endif
