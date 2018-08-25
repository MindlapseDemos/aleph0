#ifndef TILEMAZE_H_
#define TILEMAZE_H_

struct tilemaze;

struct tilemaze *load_tilemaze(const char *fname);
void destroy_tilemaze(struct tilemaze *tmz);

void update_tilemaze(struct tilemaze *tmz);
void draw_tilemaze(struct tilemaze *tmz);

#endif	/* TILEMAZE_H_ */
