#include <assert.h>
#include <fcntl.h>
#include <android/asset_manager.h>
#include "assfile.h"
#include "anglue.h"

extern struct android_app *app;	/* defined in android/main.c */

static int putback_buf = -1;

ass_file *ass_fopen(const char *fname, const char *mode)
{
	AAsset *ass;
	unsigned int flags = 0;
	char prev = 0;

	while(*mode) {
		switch(*mode) {
		case 'r':
			flags |= O_RDONLY;
			break;

		case 'w':
			flags |= O_WRONLY;
			break;

		case 'a':
			flags |= O_APPEND;
			break;

		case '+':
			if(prev == 'w' || prev == 'a') {
				flags |= O_CREAT;
			}
			break;

		default:
			break;
		}
		prev = *mode++;
	}

	assert(app);
	assert(app->activity);
	assert(app->activity->assetManager);
	if(!(ass = AAssetManager_open(app->activity->assetManager, fname, flags))) {
		return 0;
	}
	return (ass_file*)ass;
}

void ass_fclose(ass_file *fp)
{
	AAsset_close((AAsset*)fp);
}

long ass_fseek(ass_file *fp, long offs, int whence)
{
	return AAsset_seek((AAsset*)fp, offs, whence);
}

long ass_ftell(ass_file *fp)
{
	return AAsset_seek((AAsset*)fp, 0, SEEK_CUR);
}

int ass_feof(ass_file *fp)
{
	return AAsset_getRemainingLength((AAsset*)fp) == 0 ? 1 : 0;
}

size_t ass_fread(void *buf, size_t size, size_t count, ass_file *fp)
{
	size_t nbytes = size * count;
	size_t rdbytes = 0;

	if(putback_buf >= 0) {
		*(unsigned char*)buf = (unsigned char)putback_buf;
		putback_buf = -1;
		--nbytes;
		++rdbytes;
		buf = (unsigned char*)buf + 1;
	}

	return (rdbytes + AAsset_read((AAsset*)fp, buf, nbytes)) / size;
}

int ass_ungetc(int c, ass_file *fp)
{
	putback_buf = c;
	return 0;
}

/* --- convenience functions --- */

int ass_fgetc(ass_file *fp)
{
	unsigned char c;

	if(ass_fread(&c, 1, 1, fp) < 1) {
		return -1;
	}
	return (int)c;
}

char *ass_fgets(char *s, int size, ass_file *fp)
{
	int i, c;
	char *ptr = s;

	if(!size) return 0;

	for(i=0; i<size - 1; i++) {
		if((c = ass_fgetc(fp)) == -1) {
			break;
		}
		*ptr++ = c;

		if(c == '\n') break;
	}
	*ptr = 0;
	return ptr == s ? 0 : s;
}
