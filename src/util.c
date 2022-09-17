#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "util.h"
#include "demo.h"

uint32_t perf_start_count, perf_interval_count;

void *malloc_nf_impl(size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = malloc(sz))) {
		fprintf(stderr, "%s:%d failed to allocate %lu bytes\n", file, line, (unsigned long)sz);
		demo_abort();
	}
	return p;
}

void *calloc_nf_impl(size_t num, size_t sz, const char *file, int line)
{
	void *p;
	if(!(p = calloc(num, sz))) {
		fprintf(stderr, "%s:%d failed to allocate %lu bytes\n", file, line, (unsigned long)(num * sz));
		demo_abort();
	}
	return p;
}

void *realloc_nf_impl(void *p, size_t sz, const char *file, int line)
{
	if(!(p = realloc(p, sz))) {
		fprintf(stderr, "%s:%d failed to realloc %lu bytes\n", file, line, (unsigned long)sz);
		demo_abort();
	}
	return p;
}

char *strdup_nf_impl(const char *s, const char *file, int line)
{
	int len;
	char *res;

	len = strlen(s);
	if(!(res = malloc(len + 1))) {
		fprintf(stderr, "%s:%d failed to duplicate string\n", file, line);
		demo_abort();
	}
	memcpy(res, s, len + 1);
	return res;
}
