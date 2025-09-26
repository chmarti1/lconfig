/* FILTER.H 
 * 
 * 
 */


#ifndef __TF_H__
#define __TF_H__


#define TF_PID_TAU  2.2
#define TF_ORDER_NDEF   ((unsigned int) -1)

/* TF_T
 * 
 * The transfer function state struct holds a digital filter's 
 * coefficients and state history.
 *            n          n-1
 *  Y     bn z  + bn-1 z     + ... b1 z + b0
 * --- = -----------------------------------
 *  X         n          n-1
 *        an z  + an-1 z     + ... a1 z + a0
 * 
 * The order (n) determines the length of the history and coefficient 
 * arrays.  Traditionally, the transfer function is normalized so that
 * an is 1, but the filter algorithms do not assume that this has been
 * done.
 */
typedef struct __tf_t__ {
    double *a;      // Denominator coefficients
    double *b;      // Numerator coefficients
    double *x;      // Input history
    double *y;      // Output history
    unsigned int order; // Order
} tf_t;


/***********************************************************************
 * 1. Initialization, construction, and destruction                    *
 *      These functions are responsible for managing the dynamic memory*
 *      in the tf_t struct                                             *
 ***********************************************************************/

/* TF_INIT
 * 
 * Initializes the filter with NULL array pointers, so it can safely be
 * identified as "not yet constructed" by TF_IS_FREE and TF_IS_READY.
 * TF_INIT should be called on filters at the beginning of an algorithm,
 * but NEVER AGAIN.  TF_CONSTRUCT and TF_DESTRUCT are solely responsible 
 * for managing allocation and freeing of dynamic memory.
 * 
 * Prerequisite: None
 * Integrity checks: None
 * Returns: 0 always
 */
int tf_init(tf_t *g);

/* TF_CONSTRUCT
 * 
 *  Allocates memory for the filter's arrays.  Calls TF_IS_FREE() to 
 * verify that the struct has not already been constructed.  Passing 
 * structs that have not been initialized by TF_INIT() leads to 
 * undefined behavior and can cause segfaults.
 * 
 * Prerequisite: TF_INIT()
 * Integrity checks: error if not TF_IS_FREE()
 * Returns: 0 on success, -1 on error
 */
int tf_construct(tf_t *g, unsigned int order);

/* TF_DESTRUCT
 * 
 *  Frees memory from the filter's arrays.  Do not call on structs that 
 * have not been initialized by TF_INIT().  Non-NULL values in the 
 * internal array pointers are presumed to point to memory that needs to
 * be freed.  TF_DESTRUCT() can safely be called on a struct that was 
 * only partially initialized for some reason (for example if 
 * TF_CONSTRUCT() were somehow interrupted).
 * 
 * Prerequisite: TF_INIT()
 * Integrity checks: None
 * Returns: 0 always
 */
int tf_destruct(tf_t *g);

/***********************************************************************
 * 2. Diagnostics                                                      *
 *      These functions test various attributes of tfs or tf pairs     *
 ***********************************************************************/

/* TF_IS_FREE
 * 
 * Returns 1 if ALL arrays (a, b, x, and y) are NULL.  This should be the
 * state after a call to TF_INIT() or TF_DESTRUCT().  An uninitialized 
 * transfer function may accidentally have this state, so always be sure
 * to call TF_INIT() before using transfer functions!
 * 
 * Prerequisite: TF_INIT()
 * Integrity checks: None
 * Returns: 1 if free, 0 otherwise
 */
int tf_is_free(const tf_t *g);

/* TF_IS_READY
 * 
 * Returns 1 if ALL arrays (a, b, x, and y) are not NULL.  This should be
 * the state after a call to TF_CONSTRUCT().  An uninitialized transfer 
 * function may accidentally have this state, so always be sure to call
 * TF_INIT() before using transfer functions!
 * 
 * Prerequisite: TF_INIT()
 * Integrity checks: None
 * Returns: 1 if ready, 0 otherwise
 */
int tf_is_ready(const tf_t *g);

/* TF_IS_COMMON
 * 
 * Test two transfer functions for a common denominator.  The algorithm 
 * correctly ignores leading zeros in the denominator so they do not need
 * to be constructed with the same order.  Redundant roots are not 
 * detected, and there is no allowance for floating point error, so 
 * coefficients must be an exact match to be regarded as common.
 * 
 * Prerequisite: TF_CONSTRUCT()
 * Integrity checks: None
 * Returns: 1 if common, 0 otherwise
 */
int tf_is_common(const tf_t *a, const tf_t *b);


/* TF_MINORDER
 * TF_MAXORDER
 * 
 * Returns the minimum (or maximum) of the orders of two transfer 
 * functions.  These are useful for calculating the order expected from
 * arithmetic operations.
 * 
 * These functions DO NOT detect reduced order from leading zeros.  See
 * TF_FINALIZE().
 * 
 * Prerequisite: TF_CONSTRUCT()
 * 
 */
unsigned int tf_minorder(const tf_t *a, const tf_t *b);
unsigned int tf_maxorder(const tf_t *a, const tf_t *b);


/***********************************************************************
 * 3. Transfer function arithmetic                                     *
 *      These functions are intended to help with the calculation of   *
 *      transfer function coefficients by providing basic arithmetic   *
 *      tools.                                                         *
 ***********************************************************************/

/* TF_COPY
 * 
 * Copy transfer function A into B.  If B is initialized but not yet 
 * constructed or if the order of B does not match the order of A, B
 * will be destructed and re-constructed with the appropriate order.
 * 
 * Prerequisite: TF_CONSTRUCT() (a), TF_INIT() (b)
 * Integrity checks: Error if not TF_IS_READY(a), checks order. 
 * Returns: 0 on success, -1 on failure
 */
int tf_copy(const tf_t *a, tf_t *b);

/* TF_INVERSE
 * 
 * Trades numerator and denominator.  The array pointers are exchanged,
 * so this operation is fast.  This should be used in conjunction with
 * copy to calculate the inverse of a transfer function without 
 * destroying the original.
 * 
 * Prerequisite: TF_INIT()
 * Integrity checks: None
 * Returns: 0 always
 */
int tf_inverse(tf_t *a);

/* TF_MULTIPLY
 * 
 * Multipy two transfer functions, so c = a*b.  The order of c will 
 * always be constructed to the sum of the orders of a and b.  If c has
 * existing data, it will be destroyed without warning or error.
 * 
 * Prerequisite: TF_CONSTRUCT() (a,b)  TF_INIT() (c)
 * Integrity checks: tests valid order for a and b
 * Returns: 0 on success, -1 if order==TF_ORDER_NDEF
 */
int tf_multiply(const tf_t *a, const tf_t *b, tf_t *c);

/* TF_ADD
 * 
 * Add two transfer functions c = a + b.  
 * 
 * If a and b are found to share a common denominator using TF_COMMON(), 
 * the numerators are added and the order of c will be min(a.order, 
 * b.order).  It is still possible for a and b to be different orders if
 * one or both have leading zeros in the denominator.
 * 
 * If a and b do not have a common denominator (see TF_COMMON()), they 
 * will be cross-multiplied to force one.  In this case, the order of c 
 * will be a.order * b.order.
 * 
 * If c has existing data, it will be destroyed without warning or 
 * error.
 * 
 * Prerequisite: TF_CONSTRUCT() (a,b)  TF_INIT() (c)
 * Integrity checks: tests valid order for a and b
 * Returns: 0 on success, -1 if order==TF_ORDER_NDEF
 */
int tf_add(const tf_t *a, const tf_t *b, tf_t *c);




/* TF_REDUCE
 * 
 * 
 * 
 */
int tf_reduce(tf_t *a);


/***********************************************************************
 * 4. State Control                                                    *
 *      These functions deal with the transfer function's state and    *
 *      history.  In this model, input samples are fed one-at-a-time,  *
 *      and output samples are returned (see TF_EVAL()).               *
 ***********************************************************************/

/* TF_RESET
 * 
 * Writes zero to all input and output histories.  This returns the 
 * system to its initial state without needing to re-calculate the 
 * transfer function coefficients.  
 * 
 * Prerequisite: TF_CONSTRUCT()
 * Integrity checks: error if not TF_IS_READY()
 * Returns: 0 on success, -1 on failure
 */
int tf_reset(tf_t *g);

/* TF_EVAL
 * 
 * Evaluate the filter at its next input sample and return the cor-
 * responding output sample.  The input, x, is retained in the struct's 
 * internal history array, and so is the output.  There is no need for
 * the application to manage them.
 * 
 * The TF_EVAL() needs to be evaluated repeatedly and often in real time
 * so integrity checks are excluded for efficiency.  If there is doubt,
 * the application should implement a call to TF_IS_READY() outside of
 * loops that make repeated calls to TF_EVAL().
 * 
 * Prerequisite: TF_CONSTRUCT()
 * Integrity checks: None
 * Returns: next output sample
 */
double tf_eval(tf_t *g, double x);


/***********************************************************************
 * 5. Initializers                                                     *
 *      These functions are for automatically building common useful   *
 *      transfer functions.                                            *
 ***********************************************************************/

int tf_pid(tf_t *g, double ts, double Kp, double Ki, double Kd);

/* TF_BUTTERWORTH
 * 
 * Set the coefficients of G to form a lowpass butterworth filter.  The 
 * order is determined by the order of G.  The dimensionless cuttoff 
 * frequency, wc, is the multiple of the cutoff frequency in rad/sec and
 * the sampe period in seconds.
 *      wc = omega_c * T_s
 * 
 * Any existing data in g will be destroyed by this process, and g will
 * be constructed to the specified order.  There are no integrity checks
 * as part of this process, since existing data will be discarded 
 * anyway.
 * 
 * Prerequisite: TF_INIT()
 * Integrity checks: None
 * Returns: 0 always
 */
int tf_butterworth(tf_t *g, unsigned int order, double wc);



#endif
