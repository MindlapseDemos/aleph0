#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* expects the required timer resolution in hertz
 * if res_hz is 0, the current resolution is retained
 */
void init_timer(int res_hz);

/* reset timer to start counting from ms (usually 0) */
void reset_timer(unsigned long ms);
unsigned long get_msec(void);

void sleep_msec(unsigned long msec);

#ifdef __cplusplus
}
#endif

#endif	/* TIMER_H_ */
