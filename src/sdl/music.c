#include "music.h"
#include "mikmod.h"

static MODULE *mod;
static int initialized;


static int init(void)
{
	MikMod_RegisterAllDrivers();
	MikMod_RegisterAllLoaders();

	md_mode |= DMODE_SOFT_MUSIC;
	if(MikMod_Init("") != 0) {
		fprintf(stderr, "mikmod init failed: %s\n",
				MikMod_strerror(MikMod_errno));
		return -1;
	}
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

	if(!(mod = Player_Load(fname, 64, 0))) {
		fprintf(stderr, "failed to load music: %s: %s\n", fname,
				MikMod_strerror(MikMod_errno));
		return -1;
	}
	return 0;
}

void music_close(void)
{
	if(mod) {
		music_stop();
		Player_Free(mod);
		mod = 0;
	}
}

void music_play(void)
{
	Player_Start(mod);
}

void music_stop(void)
{
	Player_Stop();
}

void music_update(void)
{
	if(Player_Active()) {
		MikMod_Update();
	}
}
