/* dynarr - dynamic resizable C array data structure
 * author: John Tsiombikas <nuclear@member.fsf.org>
 * license: public domain
 */
#ifndef DYNARR_H_
#define DYNARR_H_

void *dynarr_alloc(int elem, int szelem);
void dynarr_free(void *da);
void *dynarr_resize(void *da, int elem);

int dynarr_empty(void *da);
int dynarr_size(void *da);

/* stack semantics */
void *dynarr_push(void *da, void *item);
void *dynarr_pop(void *da);


/* usage example:
 * -------------
 * int *arr = dynarr_alloc(0, sizeof *arr);
 *
 * int x = 10;
 * arr = dynarr_push(arr, &x);
 * x = 5;
 * arr = dynarr_push(arr, &x);
 * x = 42;
 * arr = dynarr_push(arr, &x);
 *
 * for(i=0; i<dynarr_size(arr); i++) {
 *     printf("%d\n", arr[i]);
 *  }
 *  dynarr_free(arr);
 */


#endif	/* DYNARR_H_ */
