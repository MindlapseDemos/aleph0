#ifndef DYNAMIC_ARRAY_H_
#define DYNAMIC_ARRAY_H_

void *darr_alloc(int elem, int szelem);
void darr_free(void *da);
void *darr_resize_impl(void *da, int elem);
#define darr_resize(da, elem)	do { (da) = darr_resize_impl(da, elem); } while(0)

int darr_empty(void *da);
int darr_size(void *da);

void *darr_clear_impl(void *da);
#define darr_clear(da)			do { (da) = darr_clear_impl(da); } while(0)

/* stack semantics */
void *darr_push_impl(void *da, void *item);
#define darr_push(da, item)		do { (da) = darr_push_impl(da, item); } while(0)
void *darr_pop_impl(void *da);
#define darr_pop(da)			do { (da) = darr_pop_impl(da); } while(0)

/* Finalize the array. No more resizing is possible after this call.
 * Use free() instead of dynarr_free() to deallocate a finalized array.
 * Returns pointer to the finalized array.
 * Complexity: O(n)
 */
void *darr_finalize(void *da);

/* utility macros to push characters to a string. assumes and maintains
 * the invariant that the last element is always a zero
 */
#define darr_strpush(da, c) \
	do { \
		char cnull = 0, ch = (char)(c); \
		(da) = dynarr_pop_impl(da); \
		(da) = dynarr_push_impl((da), &ch); \
		(da) = dynarr_push_impl((da), &cnull); \
	} while(0)

#define darr_strpop(da) \
	do { \
		char cnull = 0; \
		(da) = dynarr_pop_impl(da); \
		(da) = dynarr_pop_impl(da); \
		(da) = dynarr_push_impl((da), &cnull); \
	} while(0)


#endif	/* DYNAMIC_ARRAY_H_ */
