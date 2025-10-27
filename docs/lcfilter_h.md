[back to API](api.md)

Version 5.01  
October 2025  
Christopher R. Martin  

# <a name=top></a>lcfilter.h

`lcfilter.h` and its corresponding c-file, `lcfilter.c` were added in version 5.01 to support digital filtering alongside downsampling.  It offers a struct type for a generic digital signal processing (DSP) model and algorithms for initializing and evaluating it.  

- [Theory](#theory)  
- [The TF Struct](#struct)  
- [Basic Functions](#basic)  
- [Diagnostic Functions](#diagnostic)  
- [Arithmetic Functions](#arithmetic)  

## <a name=theory></a>Theory
The generic discrete-time model for a system with input, $x$, and output, $y$, is

$$a_n y_n + a_{n-1} y_{n-1} + \ldots = b_n x_n + a_{n-1} x_{n-1} + \ldots$$

where $y_k$ and $x_k$ represent sample $k$ in a series of $n+1$ values.  In this way, the latest output, $y_n$, can be solved for in terms of the $n$ previous outputs, the current input, $x_n$, and the $n$ previous inputs.  This operation is said to be order $n$.  Often (but not always) the entire formula is normalized so $a_n = 1$, making this operation trivial.

The corresponding discrete-time transfer function is

$$G(z) = \frac{X}{Y} = \frac{b_n z^n + b_{n-1} z^{n-1} + \ldots}{a_n z^n + b_{n-1} z^{n-1} + \ldots} = \frac{\sum_{k=0}^n b_k z^k}{\sum_{k=0}^n a_k z^k}$$.

[top](#ref:top)

## <a name=struct></a>The Transfer Function Struct

The `tf_t` struct implements this model with dynamically declared arrays for the coefficients, $a_k$ and $b_k$, and the histories, $y_k$, $x_k$.  Additionally, optional maximum and minimum values, $y_{max}$ and $y_{min}$, can be used to clamp the output between safe values.  
```C
typedef struct __tf_t__ {
    double *a;      // Denominator coefficients
    double *b;      // Numerator coefficients
    double *x;      // Input history
    double *y;      // Output history
    double ymax;    // Maximum allowed output
    double ymin;    // Minimum allowed output
    unsigned int order; // Order
} tf_t;
```

Care must always be taken to properly initialize these structs prior to use in any other function.  Most of the functions in this suite test the array pointers against `NULL` to verify that they are properly initialized.  If they contain junk data, the program is likely to crash with a segfault error.

[top](#ref:top)

## <a name=basic></a>Basic Functions

Working with transfer function is done in four stages:  
  1. Initialization  
  2. Construction  
  3. Evaluation  
  4. Destruction  

```C
tf_t g;
int tf_init(tf_t *g);
```
__Prerequisites:__ None  
__Integrity Checks:__ None  
__Returns:__ 0 always  

The TF initializer simply assigns safe NULL values to all of the dynamic arrays, marking the transfer function as not yet constructed.  If this step is skipped, junk data in the array pointers will cause the transfer function to be misidentified as already constructed with valid arrays.  This case is likely to cause a segfaut error, and the behavior is undefined.  The operation cannot fail, and is always intended to be called first.  As such, it should NEVER be called on a transfer function once it has been constructed.

```C
int tf_construct(tf_t *g, int order);
```
__Prerequisites:__ `tf_init()`  
__Integrity Checks:__ Error on `!tf_is_free()`, `order < 0`, or `malloc()` fails  
__Returns:__ 0 on success, -1 on failure  

Construction assigns memory to the arrays based on the transfer functions intended order.  If a call to `malloc()` fails, the transfer function returns -1, but returns 0 otherwise.

```C
int tf_destruct(tf_t *g);
```
__Prerequisites:__ `tf_init()`  
__Integrity Checks:__ None  
__Returns:__ 0 always  

Destruction frees the memory assigned to the struct's dynamic arrays.  Any array pointer not NULL is passed to `free()`, so if the struct is somehow only partially destructed, it will still be safely returned to its original initialized (NULL) state.  

[top](#ref:top)

## <a name=basic></a>Diagnostic Functions

```C
int tf_is_free(const tf_t *g);
```
__Prerequisites:__ `tf_init()`  
__Integrity Checks:__ None  
__Returns:__ 1 if free, 0 otherwise  

Returns 1 if all arrays are NULL and 0 otherwise.  

```C
int tf_is_ready(const tf_t *g);
```
__Prerequisites:__ `tf_init()`  
__Integrity Checks:__ None  
__Returns:__ 1 if ready, 0 otherwise  

Returns 1 if all arrays are not NULL and 0 otherwise.  

[top](#ref:top)


## <a name=arithmetic></a>Arithmetic Functions

```C
int tf_copy(const tf_t *a, tf_t *b);
```
__Prerequisites:__ a: `tf_construct()`, b: `tf_init()`  
__Integrity Checks:__ calls `tf_is_ready()` on a  
__Returns:__ 0 on success, -1 on error  

Force b to be a transfer function with the same coefficients as a.  After the copy operation, b is initialized with zero histories.  

```C
int tf_inverse(tf_t *a);
```
__Prerequisites:__ a: `tf_construct()`, b: `tf_init()`  
__Integrity Checks:__ calls `tf_is_ready()` on a  
__Returns:__ 0 on success, -1 on error  

Trade the numerator and denominator of a transfer function.  For efficiency, this operation is in-place, so see `tf_copy()` if the original transfer function needs to be preserved.  

```C
int tf_multiply(const tf_t *a, const tf_t *b, tf_t *c);
```
__Prerequisites:__ a,b: `tf_construct()`, c: `tf_init()`  
__Integrity Checks:__ calls `tf_is_ready()` on a and b  
__Returns:__ 0 on success, -1 on error  

Multiplies two transfer functions.  Both multiplicands are passed to `tf_is_ready()` to verify valid arrays.  If there is an error allocating memory to the result, an error is raised and -1 is returned.  

This operation is in-place safe, meaning it is safe for a, b, and c to all be pointers to the same transfer function.

```C
int tf_add(const tf_t *a, const tf_t *b, tf_t *c);
```
__Prerequisites:__ a: `tf_construct()`, b: `tf_init()`  
__Integrity Checks:__ verifies valid order of a and b.
__Returns:__ 0 on success, -1 on error  

Add two transfer functions with dissimilar denominators.  To force a common denominator, the transfer functions are cross-multiplied and added.  For addition between two transfer functions that already have a common denominator, see `tf_add_common()`.  The result of the cross-multiplication is a transfer function with an order equal to the sum of the orders of a and b.

This operation is in-place safe, meaning it is safe for a, b, and c to all be pointers to the same transfer function.

```C
int tf_add_common(const tf_t *a, const tf_t *b, tf_t *c);
```
__Prerequisites:__ a,b: `tf_construct()`, c: `tf_init()`  
__Integrity Checks:__ verifies valid and equal order of a and b, verifies identical denominator coefficients.   
__Returns:__ 0 on success, -1 on error  

Add two transfer functions with identical denominators.  The numerator coefficients are explicitly added, and the denominator of one of the two is copied.   To add transfer functions with dissimilar denominators, see `tf_add()`.  Proper transfer functions with equal denominators must have equal order., so the result of the addition will also have equal order.  There is no check that the transfer functions are proper, but the denominators are checked for equality.

This operation is in-place safe, meaning it is safe for a, b, and c to all be pointers to the same transfer function.

[top](#ref:top)


## <a name=state></a>State Functions

```C
int tf_reset(tf_t *g);
```
__Prerequisites:__  `tf_construct()`  
__Integrity Checks:__  calls `tf_is_ready()`  
__Returns:__ 0 on success, -1 on error  

Clear the histories by writing 0 to all values.  Returns an error if `tf_is_ready()` returns false.

```C
double tf_eval(tf_t *g, double x);
```
__Prerequisites:__  `tf_construct()`  
__Integrity Checks:__ None  
__Returns:__ latest output  

When x is the next input sample, returns the corresponding output sample while appropriately updating all histories.  Since this function is intended to be called in "real time" in a live system, no integrity checks are performed (they would be heavily redundant in a well constructed code).  As a result, this code cannot return an error, but passing a transfer function that has not been initialized will cause a segfault error.

[top](#ref:top)

## <a name=state></a>Constructors

```C
int tf_pid(tf_t *g, double ts, double Kp, double Ki, double Kd);
```
__Prerequisites:__  `tf_init()`  
__Integrity Checks:__ None  
__Returns:__ 0 on success, -1 on error  

Generates a discrete-time PID controller transfer function with a sample rate, $t_s$, and proportional, $K_p$, integral, $K_i$, and differential $K_d$ gains.  The continuous-time PID controller has a transfer function,
$$
G(s) = K_p + \frac{K_i}{s} + \frac{K_d s}{\tau s + 1}.
$$
The first-order pole on the derivative term limits the term's magnitude at high frequency.  This is especially important for a discrete-time controller, whose sample rate limits the rate of response of the system.  Using the reverse-rectangular discrete time approximation with a sample interval, $t_s$,
$$
s \approx \frac{z-1}{t_s z}.
$$
so,
$$
G(z) = K_p + K_i t_s \frac{z}{z-1} + \frac{K_d}{t_s} \frac{z-1}{(\tau/t_s +1)z - \tau/t_s}
$$
The ratio, $\tau / t_s$, is defined by the constant, `TF_PID_TAU`, which is set to 2.2.  This value is chosen so the cutoff frequency on the derivative term is safely below the Nyquist frequency of the discrete time system.  For simplicity of notation, we will adopt the shorthand that 
$$
\hat{\tau} = \tau / t_s = 2.2
$$

The order of the resulting transfer function is automatically adjusted based on which of the three constants is nonzero.

__Proportional Gain Only__ results in a zero-order transfer function
$$
G(z) = \frac{K_p}{1}
$$
In all the remaining transfer functions, proportional gain may be zero, but it will not change the system order.

__Zero Integral Gain__ results in a first-order transfer function
$$
G(z) = \frac{K_p [(\hat{\tau} +1)z - \hat{\tau}] + K_d / t_s (z-1)}{(\hat{\tau} +1)z - \hat{\tau}}
$$

__Zero Derivative Gain__ results in a first-order transfer function
$$
G(z) = \frac{K_p (z-1) + K_i t_s z}{z - 1}
$$

__The General PID Controller__ is a second-order transfer function
$$
G(z) =  \frac{K_p D(z) + K_i t_s [(\hat{\tau} +1)z^2 - \hat{\tau}z] + K_d/t_s (z^2 - 2z + 1)}{D(z)}
$$
with the denominator, 
$$
D(z) = (\hat{\tau} +1)z^2 - (2\hat{\tau} + 1) z + \hat{\tau}
$$

As a final step, `tf_pid()` normalizes the entire transfer function by the highest order term in the denominator.

[top](#ref:top)

```C
int tf_butterworth(tf_t *g, unsigned int order, double wc);
```
__Prerequisites:__  `tf_init()`  
__Integrity Checks:__ None  
__Returns:__ 0 on success, -1 on error  

Butterworth filters have poles evenly spaced on a circle about the origin with a radius, $\omega_c$.  A filter with order, $n$, has poles
$$
p_k = \omega_c \exp\left( j \pi \frac{2k+n-1}{2n} \right)\ \forall k = 1,2\ldots,n-1
$$

If there is an even number of poles, they may be grouped into complex conjugate pairs, $k$ and $n-1-k$, forming $n/2$ second-order groups,
$$
G(s) = \prod_{k=1}^{n/2} \frac{\omega_c{^2}}{s^2 + 2\omega_c \zeta_k s + \omega_c{^2} }
$$
where 
$$
\zeta_k = \cos\left( \pi \frac{2k-1}{2n}  \right)
$$
When the number of poles is odd, there are still $(n-1)/2$ complex conjugate pole pairs, with a single purely real pole.
$$
G(s) = \frac{\omega_c}{s+\omega_c}\prod_k^{(n-1)/2} \frac{\omega_c{^2}}{s^2 + 2\omega_c \zeta_k s + \omega_c{^2} }
$$

Using the reverse rectangular approximation,
$$
s \approx \frac{z-1}{t_s z}
$$
when the number of poles is even,
$$
G(z) = \prod_k^{n/2} \frac{(\omega_c t_s){^2} z^2}{a_k z^2 +b_k z + 1}
$$
where the coefficients, $a_k$ and $b_k$ are
$$
a_k = 1 + 2\zeta_k \omega_c t_s + (\omega_c t_s)^2
$$

$$
b_k = -2 - 2\zeta_k \omega_c t_s
$$

When the number of poles is odd,
$$
G(z) = \frac{\omega_c t_s z}{(1+\omega_c t_s)z - 1}\prod_k^{n/2} \frac{(\omega_c t_s){^2} z^2}{a_k z^2 +b_k z + 1}
$$

__Cutoff Frequency__ is specified by the non-dimensional parameter, `wc`, which is calculated as $\omega_c t_s$.  

[top](#ref:top)