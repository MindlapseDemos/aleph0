#include <stdio.h>

#ifndef NO_SOUND
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "audio.h"
#include "midasdll.h"
#include "util.h"
#include "cfgopt.h"
#include "assfile/assfile.h"

#ifdef __DJGPP__
void allegro_irq_init(void);
void allegro_irq_exit(void);
#endif

#define SET_MUS_VOL(vol) \
	do { \
		int mv = (vol) * vol_master >> 10; \
		MIDASsetMusicVolume(modplay, mv ? mv + 1 : 0); \
	} while(0)

static MIDASmodulePlayHandle modplay;
static struct au_module *curmod;

static int vol_master, vol_mus, vol_sfx;

int au_init(void)
{
	modplay = 0;
	curmod = 0;
	vol_master = vol_mus = vol_sfx = 255;

#ifdef __DJGPP__
	allegro_irq_init();
#endif

	MIDASstartup();

	if(opt.sndsetup || (!MIDASloadConfig("sound.cfg") && !MIDASdetectSoundCard())) {
		if(MIDASconfig()) {
			if(!MIDASsaveConfig("sound.cfg")) {
				fprintf(stderr, "failed to save sound card configuration\n");
			}
		}
	}

	MIDASinit();

	MIDASstartBackgroundPlay(0);
	return 0;
}

void au_shutdown(void)
{
	printf("au_shutdown\n");
	if(curmod) {
		au_stop_module(curmod);
	}
	MIDASstopBackgroundPlay();
	MIDASclose();

#ifdef __DJGPP__
	allegro_irq_exit();
#endif
}

#define TMPMUSFILE	"music.tmp"
static int unpack_music(const char *fname)
{
	ass_file *fp;
	FILE *tmpfile;
	size_t sz;
	char buf[1024];

	if(!(fp = ass_fopen(fname, "rb"))) {
		fprintf(stderr, "au_load_module: failed open: %s\n", fname);
		return -1;
	}
	if(!(tmpfile = fopen(TMPMUSFILE, "wb"))) {
		fprintf(stderr, "au_load_module: failed to create temporary music file\n");
		ass_fclose(fp);
		return -1;
	}
	while((sz = ass_fread(buf, 1, sizeof buf, fp)) > 0) {
		if(fwrite(buf, 1, sz, tmpfile) < sz) {
			fprintf(stderr, "au_load_module: failed to unpack temporary music file\n");
			ass_fclose(fp);
			fclose(tmpfile);
			return -1;
		}
	}
	fclose(tmpfile);
	ass_fclose(fp);
	return 0;
}

struct au_module *au_load_module(const char *fname)
{
	static MIDASmoduleInfo info;
	struct au_module *mod;
	char *name, *end;
	FILE *fp;
	int unpacked = 0;

	if(!(fp = fopen(fname, "rb"))) {
		if(unpack_music(fname) == -1) {
			return 0;
		}
		unpacked = 1;
	} else {
		fclose(fp);
	}

	if(!(mod = malloc(sizeof *mod))) {
		fprintf(stderr, "au_load_module: failed to allocate module\n");
		if(unpacked) remove(TMPMUSFILE);
		return 0;
	}

	if(!(mod->impl = MIDASloadModule(unpacked ? "music.tmp" : (char*)fname))) {
		fprintf(stderr, "au_load_module: failed to load module: %s\n", fname);
		free(mod);
		if(unpacked) remove(TMPMUSFILE);
		return 0;
	}
	if(unpacked) remove(TMPMUSFILE);

	name = 0;
	if(MIDASgetModuleInfo(mod->impl, &info)) {
		name = info.songName;
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
		MIDASfreeModule(mod->impl);
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
	MIDASfreeModule(mod->impl);
	free(mod->name);
	free(mod);
}

int au_play_module(struct au_module *mod)
{
	if(curmod) {
		if(curmod == mod) return 0;
		au_stop_module(curmod);
	}

	if(!(modplay = MIDASplayModule(mod->impl, 1))) {
		fprintf(stderr, "au_play_module: failed to play module: %s\n", mod->name);
		return -1;
	}
	SET_MUS_VOL(vol_mus);
	curmod = mod;
	return 0;
}

void au_update(void)
{
}

int au_stop_module(struct au_module *mod)
{
	if(mod && curmod != mod) return -1;
	if(!curmod) return -1;

	MIDASstopModule(modplay);
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
	MIDASplayStatus st;

	if(!curmod) return -1;

	MIDASgetPlayStatus(modplay, &st);
	return st.pattern;
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

	/* TODO set sfx volume */
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

/* when using MIDAS, we can't access the PIT directly, so we don't build timer.c
 * and implement all the timer functions here, using MIDAS callbacks
 */
static volatile unsigned long ticks;
static unsigned long tick_interval, toffs;

static void MIDAS_CALL midas_timer(void)
{
	ticks++;
}

/* macro to divide and round to the nearest integer */
#define DIV_ROUND(a, b) \
	((a) / (b) + ((a) % (b)) / ((b) / 2))

void init_timer(int res_hz)
{
	MIDASsetTimerCallbacks(res_hz * 1000, 0, midas_timer, 0, 0);
	tick_interval = DIV_ROUND(1000, res_hz);
}

void reset_timer(unsigned long ms)
{
	ticks = 0;
	toffs = ms;
}

unsigned long get_msec(void)
{
	return (ticks * tick_interval) + toffs;
}

void sleep_msec(unsigned long msec)
{
	unsigned long wakeup_time = ticks + msec / tick_interval;
	while(ticks < wakeup_time) {
#ifdef USE_HLT
		halt();
#endif
	}
}

#ifdef __DJGPP__
#if 0
#include <dpmi.h>
#include <go32.h>

static _go32_dpmi_seginfo orig_intr[16];

/* MIDAS for GCC needs allegro's _install_irq/_remove_irq calls */
int _install_irq(int num, int (*handler)(void))
{
	_go32_dpmi_seginfo intr;

	printf("install_irq(%d): %p\n", num, (void*)handler);

	if(!orig_intr[num].pm_offset) {
		_go32_dpmi_get_protected_mode_interrupt_vector(num, orig_intr + num);
	}

	intr.pm_selector = _go32_my_cs();
	intr.pm_offset = (uintptr_t)handler;
	_go32_dpmi_allocate_iret_wrapper(&intr);
	_go32_dpmi_set_protected_mode_interrupt_vector(num, &intr);
	return 0;
}

void _remove_irq(int num)
{
	if(orig_intr[num].pm_offset) {
		printf("restore irq %d\n", num);
		_go32_dpmi_set_protected_mode_interrupt_vector(num, orig_intr + num);
		_go32_dpmi_free_iret_wrapper(orig_intr + num);
		orig_intr[num].pm_offset = 0;
	}
}
#endif
#endif	/* __DJGPP__ */

#else	/* NO_SOUND */
#include "audio.h"

static int vol_master, vol_mus, vol_sfx;

int au_init(void)
{
	vol_master = vol_mus = vol_sfx = 255;
	return 0;
}

void au_shutdown(void)
{
	printf("au_shutdown\n");
}

struct au_module *au_load_module(const char *fname)
{
	return 0;
}

void au_free_module(struct au_module *mod)
{
}

int au_play_module(struct au_module *mod)
{
	return -1;
}

void au_update(void)
{
}

int au_stop_module(struct au_module *mod)
{
	return -1;
}

int au_module_state(struct au_module *mod)
{
	return AU_STOPPED;
}

int au_volume(int vol)
{
	AU_VOLADJ(vol_master, vol);
	return vol_master;
}

int au_sfx_volume(int vol)
{
	AU_VOLADJ(vol_sfx, vol);
	return vol_sfx;
}


int au_music_volume(int vol)
{
	AU_VOLADJ(vol_mus, vol);
	return vol_mus;
}
#endif	/* NO_SOUND */
