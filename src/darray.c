#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "darray.h"
#include "util.h"


/* The array descriptor keeps auxilliary information needed to manipulate
 * the dynamic array. It's allocated adjacent to the array buffer.
 */
struct arrdesc {
	int nelem, szelem;
	int max_elem;
	int bufsz;	/* not including the descriptor */
};

#define DESC(x)		((struct arrdesc*)((char*)(x) - sizeof(struct arrdesc)))

void *darr_alloc(int elem, int szelem)
{
	struct arrdesc *desc;

	desc = malloc_nf(elem * szelem + sizeof *desc);
	desc->nelem = desc->max_elem = elem;
	desc->szelem = szelem;
	desc->bufsz = elem * szelem;
	return (char*)desc + sizeof *desc;
}

void darr_free(void *da)
{
	if(da) {
		free(DESC(da));
	}
}

void *darr_resize_impl(void *da, int elem)
{
	int newsz;
	struct arrdesc *desc;

	if(!da) return 0;
	desc = DESC(da);

	newsz = desc->szelem * elem;
	desc = realloc_nf(desc, newsz + sizeof *desc);

	desc->nelem = desc->max_elem = elem;
	desc->bufsz = newsz;
	return (char*)desc + sizeof *desc;
}

int darr_empty(void *da)
{
	return DESC(da)->nelem ? 0 : 1;
}

int darr_size(void *da)
{
	return DESC(da)->nelem;
}


void *darr_clear_impl(void *da)
{
	return darr_resize_impl(da, 0);
}

/* stack semantics */
void *darr_push_impl(void *da, void *item)
{
	struct arrdesc *desc;
	int nelem;

	desc = DESC(da);
	nelem = desc->nelem;

	if(nelem >= desc->max_elem) {
		/* need to resize */
		int newsz = desc->max_elem ? desc->max_elem * 2 : 1;

		da = darr_resize_impl(da, newsz);
		desc = DESC(da);
		desc->nelem = nelem;
	}

	if(item) {
		memcpy((char*)da + desc->nelem * desc->szelem, item, desc->szelem);
	}
	desc->nelem++;
	return da;
}

void *darr_pop_impl(void *da)
{
	struct arrdesc *desc;
	int nelem;

	desc = DESC(da);
	nelem = desc->nelem;

	if(!nelem) return da;

	if(nelem <= desc->max_elem / 3) {
		/* reclaim space */
		int newsz = desc->max_elem / 2;

		da = darr_resize_impl(da, newsz);
		desc = DESC(da);
		desc->nelem = nelem;
	}
	desc->nelem--;

	return da;
}

void *darr_finalize(void *da)
{
	struct arrdesc *desc = DESC(da);
	memmove(desc, da, desc->bufsz);
	return desc;
}
