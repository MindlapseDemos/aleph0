#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mikmod.h"
#include "audio.h"
#include "util.h"
#include "cfgopt.h"
#include "assfile/assfile.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

#define SET_MUS_VOL(vol) \
	do { \
		int mv = (vol) * vol_master >> 9; \
		Player_SetVolume(mv ? mv + 1 : 0); \
	} while(0)

static struct au_module *curmod;
static int vol_master, vol_mus, vol_sfx;

#ifdef _WIN32
static DWORD WINAPI upd_thread(void *cls);
#else
static void *update(void *cls);
#endif

int au_init(void)
{
	curmod = 0;
	vol_master = vol_mus = vol_sfx = 255;

	if(!opt.music) return 0;

#if defined(__linux__) && !defined(__ANDROID__)
	MikMod_RegisterDriver(&drv_alsa);
#elif defined(__FreeBSD__)
	MikMod_RegisterDriver(&drv_oss);
#elif defined(__sgi)
	MikMod_RegisterDriver(&drv_sgi);
#elif defined(__sun)
	MikMod_RegisterDriver(&drv_sun);
#elif defined(_WIN32)
	MikMod_RegisterDriver(&drv_ds);
#elif defined(__APPLE__)
	MikMod_RegisterDriver(&drv_osx);
#elif defined(__ANDROID__)
	MikMod_RegisterDriver(&drv_osles);
#else
	MikMod_RegisterDriver(&drv_nos);
#endif

	/*MikMod_RegisterLoader(&load_it);
	MikMod_RegisterLoader(&load_mod);
	MikMod_RegisterLoader(&load_s3m);*/
	MikMod_RegisterLoader(&load_xm);

	if(MikMod_Init("")) {
		fprintf(stderr, "failed to initialize mikmod: %s\n", MikMod_strerror(MikMod_errno));
		return -1;
	}
	MikMod_InitThreads();

	{
#ifdef _WIN32
		HANDLE thr;
		if((thr = CreateThread(0, 0, upd_thread, 0, 0, 0))) {
			CloseHandle(thr);
		}
#else
		pthread_t upd_thread;
		if(pthread_create(&upd_thread, 0, update, 0) == 0) {
			pthread_detach(upd_thread);
		}
#endif
	}
	return 0;
}

void au_shutdown(void)
{
	if(opt.music) {
		curmod = 0;
		MikMod_Exit();
	}
}

struct au_module *au_load_module(const char *fname)
{
	struct au_module *mod;
	MODULE *mikmod;
	char *name = 0, *end;
	char *filebuf;
	ass_file *fp;
	long filesz;

	if(!(fp = ass_fopen(fname, "rb"))) {
		fprintf(stderr, "au_load_module: failed to open: %s\n", fname);
		return 0;
	}
	filesz = ass_fseek(fp, 0, SEEK_END);
	ass_fseek(fp, 0, SEEK_SET);

	if(!(filebuf = malloc(filesz))) {
		fprintf(stderr, "au_load_module: failed to allocate file buffer\n");
		ass_fclose(fp);
		return 0;
	}
	if(ass_fread(filebuf, 1, filesz, fp) != filesz) {
		fprintf(stderr, "au_load_module: unexpected EOF while reading: %s\n", fname);
		free(filebuf);
		ass_fclose(fp);
		return 0;
	}
	ass_fclose(fp);

	if(!(mod = malloc(sizeof *mod))) {
		fprintf(stderr, "au_load_module: failed to allocate module\n");
		free(filebuf);
		return 0;
	}

	if(!(mikmod = Player_LoadMem(filebuf, filesz, 128, 0))) {
		fprintf(stderr, "au_load_module: failed to load module: %s: %s\n",
				fname, MikMod_strerror(MikMod_errno));
		free(mod);
		free(filebuf);
		return 0;
	}
	free(filebuf);
	mod->impl = mikmod;

	if(mikmod->songname && *mikmod->songname) {
		name = alloca(strlen(mikmod->songname) + 1);
		strcpy(name, mikmod->songname);

		end = name + strlen(name) - 1;
		while(end >= name && isspace(*end)) {
			*end-- = 0;
		}
		if(!*name) name = 0;
	}

	if(!name) {
		/* fallback to using the filename */
		if((name = strrchr(fname, '/')) || (name = strrchr(fname, '\\'))) {
			name++;
		} else {
			name = (char*)fname;
		}
	}

	if(!(mod->name = malloc(strlen(name) + 1))) {
		fprintf(stderr, "au_load_module: mod->name malloc failed\n");
		Player_Free(mod->impl);
		free(mod);
		return 0;
	}
	strcpy(mod->name, name);

	printf("loaded module \"%s\" (%s)\n", name, fname);
	return mod;
}

void au_free_module(struct au_module *mod)
{
	if(!mod) return;

	if(mod == curmod) {
		au_stop_module(curmod);
	}
	Player_Free(mod->impl);
	free(mod->name);
	free(mod);
}

int au_play_module(struct au_module *mod)
{
	if(curmod) {
		if(curmod == mod) return 0;
		au_stop_module(curmod);
	}

	Player_Start(mod->impl);
	SET_MUS_VOL(vol_mus);
	curmod = mod;
	return 0;
}

void au_update(void)
{
#ifndef NDEBUG
	if(!curmod) return;

	if(!Player_Active()) {
		Player_Stop();
		curmod = 0;
	}
#endif
}

#ifdef _WIN32
static DWORD WINAPI upd_thread(void *cls)
#else
static void *update(void *cls)
#endif
{
	for(;;) {
		if(Player_Active()) {
			MikMod_Update();
		}
#ifdef _WIN32
		Sleep(10);
#else
		usleep(10000);
#endif
	}
	return 0;
}

int au_stop_module(struct au_module *mod)
{
	if(mod && curmod != mod) return -1;
	if(!curmod) return -1;

	Player_Stop();
	curmod = 0;
	return 0;
}

int au_module_state(struct au_module *mod)
{
	if(mod) {
		return curmod == mod ? AU_PLAYING : AU_STOPPED;
	}
	return curmod ? AU_PLAYING : AU_STOPPED;
}

int au_player_pos(void)
{
	if(!curmod) return -1;

	return Player_GetOrder() - 1;
}

int au_volume(int vol)
{
	AU_VOLADJ(vol_master, vol);
	if(vol != vol_master) {
		vol_master = vol;

		au_sfx_volume(vol_sfx);
		au_music_volume(vol_mus);
	}
	return vol_master;
}

int au_sfx_volume(int vol)
{
	AU_VOLADJ(vol_sfx, vol);
	vol_sfx = vol;
	/* TODO */
	return vol_sfx;
}

int au_music_volume(int vol)
{
	AU_VOLADJ(vol_mus, vol);
	vol_mus = vol;

	if(curmod) {
		SET_MUS_VOL(vol);
	}
	return vol_mus;
}
