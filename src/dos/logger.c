#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "logger.h"

static int logfd = -1, orig_fd1 = -1;

int init_logger(const char *fname)
{
	if(logfd != -1) return -1;

	if((logfd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		fprintf(stderr, "init_logger: failed to open %s: %s\n", fname, strerror(errno));
		return -1;
	}

	orig_fd1 = dup(1);
	close(1);
	close(2);
	dup(logfd);
	dup(logfd);
	return 0;
}

void stop_logger(void)
{
	if(logfd >= 0) {
		close(logfd);
		logfd = -1;
	}
	if(orig_fd1 >= 0) {
		close(1);
		close(2);
		dup(orig_fd1);
		dup(orig_fd1);
		orig_fd1 = -1;
	}
}

int print_tail(const char *fname)
{
	FILE *fp;
	char buf[64];
	long lineoffs[16];
	int wr, rd, c;

	if(!(fp = fopen(fname, "r"))) {
		return -1;
	}
	wr = rd = 0;
	lineoffs[wr++] = 0;
	while(fgets(buf, sizeof buf, fp)) {
		lineoffs[wr] = ftell(fp);
		wr = (wr + 1) & 0xf;
		if(wr == rd) {
			rd = (rd + 1) & 0xf;
		}
	}

	fseek(fp, lineoffs[rd], SEEK_SET);
	while((c = fgetc(fp)) != -1) {
		fputc(c, stdout);
	}
	fclose(fp);
	return 0;
}
