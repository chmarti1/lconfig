#include "lcfilter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


// Helper functions

/* TF_SWAP
 * 
 * Swap data between two transfer functions.  This is used in the back-end to 
 * pass data between a temporary result TF and the output prior to destroying
 * the temporary TF 
 */
void tf_swap(tf_t *a, tf_t *b){
    double *ptr;
    unsigned int order;
    
    ptr = a->a;
    a->a = b->a;
    b->a = ptr;
    
    ptr = a->b;
    a->b = b->b;
    b->b = ptr;
    
    ptr = a->x;
    a->x = b->x;
    b->x = ptr;
    
    ptr = a->y;
    a->y = b->y;
    b->y = ptr;
    
    order = a->order;
    a->order = b->order;
    b->order = order;
}



unsigned int tf_minorder(const tf_t *a, const tf_t *b){
    return (a->order < b->order ? a->order : b->order);
}

unsigned int tf_maxorder(const tf_t *a, const tf_t *b){
    return (a->order > b->order ? a->order : b->order);
}

int tf_is_common(const tf_t *a, const tf_t *b){
    int ii;
    // Force a to have the higher order
    if(a->order < b->order)
        return tf_is_common(b, a);
    for(ii=0; ii<=b->order; ii++){
        if(a->a[ii] != b->a[ii])
            return 0;
    }
    for(;ii<=a->order; ii++){
        if(a->a[ii])
            return 0;
    }
    return 1;
}



int tf_init(tf_t *g){
    g->order = TF_ORDER_NDEF;
    g->a = NULL;
    g->b = NULL;
    g->x = NULL;
    g->y = NULL;
}


int tf_is_free(const tf_t *g){
    return !(g->a || g->b || g->x || g->y); 
}


int tf_is_ready(const tf_t *g){
    return (g->a && g->b && g->x && g->y); 
}



int tf_construct(tf_t *g, unsigned int order){
    unsigned int size;
    if(!tf_is_free(g)){
        fprintf(stderr, "TF_CONSTRUCT: Transfer function is not initialized or already constructed.\n");
        return -1;
    }
    g->order = order;
    size = (order+1)*sizeof(double);
    g->a = malloc(size);
    g->b = malloc(size);
    g->x = malloc(size);
    g->y = malloc(size);
    memset(g->x, 0, size);
    memset(g->y, 0, size);
    memset(g->a, 0, size);
    memset(g->b, 0, size);
    return 0;
}


int tf_destruct(tf_t *g){
    if(g->a){
        free(g->a);
        g->a = NULL;
    }
    if(g->b){
        free(g->b);
        g->b = NULL;
    }
    if(g->x){
        free(g->x);
        g->x = NULL;
    }
    if(g->y){
        free(g->y);
        g->y = NULL;
    }
    g->order = 0;
    return 0;
}

int tf_reset(tf_t *g){
    unsigned int size;
    size = (g->order+1)*sizeof(double);
    if( !tf_is_ready(g) ){
        fprintf(stderr, "TF_RESET: struct is not constructed.\n");
        return -1;
    }
    memset(g->x, 0, size);
    memset(g->y, 0, size);
    return 0;
}

int tf_copy(const tf_t *a, tf_t *b){
    int ii;
    if( !tf_is_ready(a) ){
        fprintf(stderr, "TF_COPY: Source TF was not properly constructed.\n");
        return -1;
    }else if(a->order == b->order){
        if(tf_reset(b)){
            fprintf(stderr, "TF_COPY: Failed unexpectedly while resetting target TF state.\n");
            return -1;
        }
    }else{
        tf_destruct(b);
        if(tf_construct(b, a->order)){
            fprintf(stderr, "TF_COPY: Construction of target TF failed unexpectedly.\n");
            return -1;
        }
    }
    // Copy coefficients
    for(ii=0; ii<=a->order; ii++){
        b->a[ii] = a->a[ii];
        b->b[ii] = a->b[ii];
    }
    return 0;
}

/* TF_INVERSE
 * 
 * Trades numerator and denominator.  The operation is in-place.
 */
int tf_inverse(tf_t *g){
    double *temp;
    temp = g->a;
    g->a = g->b;
    g->b = temp;
    return 0;
}


int tf_multiply(const tf_t *a, const tf_t *b, tf_t *c){
    tf_t temp;
    int ai, bi, ci, order;
    
    // Verify a and b have been constructed
    if(a->order == TF_ORDER_NDEF || b->order == TF_ORDER_NDEF){
        fprintf(stderr, "TF_MULTIPLY: Called with multiplicands that were not constructed!\n");
        return -1;
    }
    
    tf_init(&temp);    
    // Construct temp to hold the result in-process
    if(tf_construct(&temp, a->order+b->order)){
        fprintf(stderr, "TF_MULTIPLY: Construction failed unexpectedly.\n");
        return -1;
    }
    for(ai=0; ai<=a->order; ai++){
        for(bi=0; bi<=b->order; bi++){
            ci = ai + bi;
            temp.a[ci] += a->a[ai] * b->a[bi];
            temp.b[ci] += a->b[ai] * b->b[bi];
        }
    }
    // Transfer the coefficient arrays
    tf_swap(&temp, c);

    // Destroy the data originally in c
    tf_destruct(&temp);
    return 0;
}


int tf_add(const tf_t *a, const tf_t *b, tf_t *c){
    tf_t temp;
    int ai, bi, ci;
    unsigned int minorder;

    // Verify a and b are constructed
    if(a->order==TF_ORDER_NDEF || b->order==TF_ORDER_NDEF){
        fprintf(stderr, "TF_ADD: Called with addends that were not constructed!\n");
        return -1;
    }

    tf_init(&temp);
    // Test for common denominators
    if(tf_is_common(a,b)){
        // Detect the minimum order
        if(tf_construct(&temp, tf_minorder(a,b))){
            fprintf(stderr, "TF_ADD: Construction failed unexpectedly.\n");
            return -1;
        }
        for(ai=0; ai<=minorder; ai++){
            // Copy the denominator
            temp.a[ai] = a->a[ai];
            // sum the numerator
            temp.b[ai] = a->b[ai] + b->b[ai];
        }
    }else{
        // Construct the result with an appropriate order
        if(tf_construct(&temp, a->order + b->order)){
            fprintf(stderr, "TF_ADD: Construction failed unexpectedly.\n");
            return -1;
        }
        for(ai=0; ai<=a->order; ai++){
            for(bi=0; bi<=b->order; bi++){
                ci = ai + bi;
                // Test terms that are beyond c.order
                if(ci > c->order &&
                        ( (a->a[ai] && b->a[bi]) ||
                          (a->b[ai] && b->b[bi]))  ){
                    fprintf(stderr, "TF_ADD: Result order is not high enough\n");
                    tf_destruct(&temp);
                    return -1;
                }
                // multiply the denominators
                temp.a[ci] += a->a[ai] * b->a[bi];
                // cross multiply the denominators and numerators
                temp.b[ci] += a->b[ai] * b->a[bi];
                temp.b[ci] += a->a[ai] * b->b[bi];
            }
        }
    }
    
    // Transfer the coefficient arrays
    tf_swap(&temp, c);

    // Destroy the memory originally in c
    tf_destruct(&temp);
    return 0;    
}


/* TF_REDUCE
 * 
 * 
 * 
 */
int tf_reduce(tf_t *g){
    char bnz_f;     // b non-zero flag
    tf_t temp;
    double *ctemp;
    int ii;
    
    bnz_f = 0;
    for(ii=g->order; ii>=0; ii--){
        // Test for a non-zero a-value
        if(g->a[ii])
            break;
        // Test for a non-zero b-value
        bnz_f = bnz_f || g->b[ii];
    }
    // Test for an improper TF
    if(bnz_f){
        fprintf(stderr, "TF_FINALIZE: Improper transfer function.\n");
        return -1;
    }
    // Detect reduction in order
    if(ii < g->order){
        // Create a new transfer function with lower order
        tf_construct(&temp, ii);
        // Copy coefficients to the new TF
        for(ii=0; ii<=temp.order; ii++){
            temp.a[ii] = g->a[ii];
            temp.b[ii] = g->b[ii];
        }
        // Shift the new coefficient arrays to the original TF and
        // free the original memory
        tf_swap(&temp, g);
        // temp order is now incorrect, but it doesn't matter.
        tf_destruct(&temp);
    }
    // Normalize by a[order]
    for(ii=0; ii<=g->order; ii++){
        g->b[ii] /= g->a[g->order];
        g->a[ii] /= g->a[g->order];
    }
    return 0;
}


double tf_eval(tf_t *g, double x){
    int ii;
    // Shift all history values
    for(ii=0; ii<g->order; ii++){
        g->x[ii] = g->x[ii+1];
        g->y[ii] = g->y[ii+1];
    }
    // Zero the new output and read in the new input
    g->x[g->order] = x;
    g->y[g->order] = 0.0;
    for(ii=0; ii<g->order; ii++)
        g->y[g->order] += g->b[ii] * g->x[ii] - g->a[ii] * g->y[ii];
    g->y[g->order] += g->b[g->order] * g->x[g->order];
    g->y[g->order] /= g->a[g->order];
    return g->y[g->order];
}



int tf_pid(tf_t *g, double ts, double Kp, double Ki, double Kd){
    int ii;
    double aa;  // intermediate variable for calculations
    // Test gains for legal values
    if(Kp < 0 || Ki < 0 || Kd < 0){
        fprintf(stderr, "TF_PID: Encountered negative gain\n");
        return -1;
    }
    // Ensure that G is free
    tf_destruct(g);
    // Automatically case out the special cases where derivative and/or 
    // integral gain are zero.
    // Ki == 0, Kd == 0
    if(Ki == 0. && Kd == 0.){
        // Order is zero
        tf_construct(g, 0);
        // The coefficients are not difficult to calculate
        g->a[0] = 1;
        g->b[0] = Kp;
    }else if(Ki == 0){
        // First order
        tf_construct(g, 1);
        // PD filter coefficients using reverse rectangular approx
        // Denominator
        g->a[0] = -TF_PID_TAU;
        g->a[1] = TF_PID_TAU+1;
        // Contributions from proportional gain
        g->b[0] = Kp*g->a[0];
        g->b[1] = Kp*g->a[1];
        // Contributions from derivative gain
        aa = Kd/ts;
        g->b[0] += -aa;
        g->b[1] += aa;
    }else if(Kd == 0){
        // First order
        tf_construct(g, 1);
        // PI filter coefficients using reverse rectangular approx
        // Denominator
        g->a[0] = -1;
        g->a[1] = 1;
        // Contributions from proportional gain
        g->b[0] = Kp*g->a[0];
        g->b[1] = Kp*g->a[1];
        // Contributions from integral gain
        g->b[1] += Ki*ts;
    }else{
        // Second order
        tf_construct(g, 2);
        // PID filter coefficients using reverse rectangular approx
        // Denominator
        g->a[0] = TF_PID_TAU; 
        g->a[1] = -(2*TF_PID_TAU+1);
        g->a[2] = TF_PID_TAU+1;
        // Contributions from proportional gain
        g->b[0] = Kp * g->a[0];
        g->b[1] = Kp * g->a[1];
        g->b[2] = Kp * g->a[2];
        // Contributions from integral gain
        aa = ts * Ki;
        g->b[1] += -aa * TF_PID_TAU;
        g->b[2] += aa * (TF_PID_TAU+1);
        // Contributions from derivative gain
        aa = Kd / ts;
        g->b[0] += aa;
        g->b[1] += -2*aa;
        g->b[2] += aa;
    }
    // Finally, whatever the coefficients are, normalize by a[n]
    for(ii=0; ii<=g->order; ii++){
        g->b[ii] /= g->a[g->order];
        g->a[ii] /= g->a[g->order];
    }
    return 0;
}

int tf_butterworth(tf_t *g, unsigned int order, double wc){
    unsigned int k, order_2;
    double ak;
    tf_t temp;
    // Construct a helper transfer function for building the TF 
    // one pole pair at a time.
    tf_init(&temp);
    tf_construct(&temp, 2);
    // Force g to be free
    tf_destruct(g);
    // If there is an odd number of poles
    if(order%2){
        tf_construct(g,1);
        g->b[1] = wc;
        g->b[0] = 0;
        g->a[1] = (1+wc);
        g->a[0] = -1.0;
    // If there is an even number of poles
    }else{
        tf_construct(g,0);
        g->b[0] = 1;
        g->a[0] = 1;
    }
    // Loop over the remaining poles
    order_2 = order / 2;
    temp.b[2] = wc*wc;
    for(k=0;k<order_2;k++){
        ak = 2.0 * cos(M_PI_2/order * (order - 1 - 2*k));
        temp.a[2] = wc*wc + ak*wc + 1.0;
        temp.a[1] = -(2+ak*wc);
        temp.a[0] = 1.0;
        tf_multiply(g, &temp, g);
    }
    tf_destruct(&temp);
    // Normalize by a[order]
    for(k=0; k<=g->order; k++){
        g->b[k] /= g->a[g->order];
        g->a[k] /= g->a[g->order];
    }
    return 0;
}
