#ifndef MUSIC_H_
#define MUSIC_H_

int music_open(const char *fname);
void music_close(void);
void music_play(void);
void music_stop(void);
void music_update(void);

#endif	/* MUSIC_H_ */
