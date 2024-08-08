/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include <math.h>

/*
NOTE: the example code here does not check the DCP's engaged flag. It is therefore
not suitable for use in multi-threaded applications or in interrupt service
routines. See the device datasheet for more information.
*/

/// \tag::hello_dcp[]

extern double dcp_dot                          (double*p,double*q,int n);
extern double dcp_dotx                         (float*p,float*q,int n);
extern float  dcp_iirx                         (float x,float*temp,float*coeff,int order);
extern void   dcp_butterfly_radix2             (double*x,double*y);
extern void   dcp_butterfly_radix2_twiddle_dif (double*x,double*y,double*tf);
extern void   dcp_butterfly_radix2_twiddle_dit (double*x,double*y,double*tf);
extern void   dcp_butterfly_radix4             (double*w,double*x,double*y,double*z);

static void dcp_test0() {
    double u[3]={1,2,3};
    double v[3]={4,5,6};
    double w;
    w=dcp_dot(u,v,3);
    printf("(1,2,3).(4,5,6)=%g\n",w);
}

static void dcp_test1() {
    float u[3]={1+pow(2,-20),2,3};
    float v[3]={1-pow(2,-20),5,6};
    double w;
    w=dcp_dotx(u,v,3);
    printf("(1+pow(2,-20),2,3).(1-pow(2,-20),5,6)=%.17g\n",w);
}

static void dcp_test2() {
  int t;
  float w;
// filter coefficients calculated using Octave as follows:
// octave> pkg load signal
// octave> format long
// octave> [b,a]=cheby1(2,1,.5)
// b = 0.307043201259064   0.614086402518128   0.307043201259064
// a = 1.000000000000000e+00   6.406405700380895e-02   3.139684953186774e-01
// and tested as follows:
// octave> filter(b,a,[1 zeros(1,19)])
    float coeff[5]={0.3070432,0.3139685,0.6140864,0.06406406,0.3070432};
    float temp[4]={0};
    printf("IIR filter impulse response:\n");
    for(t=0;t<20;t++) {
        w=dcp_iirx(t?0:1,temp,coeff,2);
        printf("y[%2d]=%g\n",t,w);
    }
}

static void dcp_test3() {
    double x[2]={2,3};
    double y[2]={5,7};
    dcp_butterfly_radix2(x,y);
    printf("Radix-2 butterfly of (2+3j,5+7j)=(%g%+gj,%g%+gj)\n",x[0],x[1],y[0],y[1]);
}

static void dcp_test4() {
    double x[2]={2,3};
    double y[2]={5,7};
    double t[2]={1.5,2.5};
    dcp_butterfly_radix2_twiddle_dif(x,y,t);
    printf("Radix-2 DIF butterfly of (2+3j,5+7j) with twiddle factor (1.5+2.5j)=(%g%+gj,%g%+gj)\n",x[0],x[1],y[0],y[1]);
}

static void dcp_test5() {
    double x[2]={2,3};
    double y[2]={5,7};
    double t[2]={1.5,2.5};
    dcp_butterfly_radix2_twiddle_dit(x,y,t);
    printf("Radix-2 DIT butterfly of (2+3j,5+7j) with twiddle factor (1.5+2.5j)=(%g%+gj,%g%+gj)\n",x[0],x[1],y[0],y[1]);
}

static void dcp_test6() {
    double w[2]={2,3};
    double x[2]={5,7};
    double y[2]={11,17};
    double z[2]={41,43};
    dcp_butterfly_radix4(w,x,y,z);
    printf("Radix-4 butterfly of (2+3j,5+7j,11+17j,41+43j)=(%g%+gj,%g%+gj,%g%+gj,%g%+gj)\n",w[0],w[1],x[0],x[1],y[0],y[1],z[0],z[1]);
}

int main() {
    stdio_init_all();

    printf("Hello, DCP!\n");

    dcp_test0();
    dcp_test1();
    dcp_test2();
    dcp_test3();
    dcp_test4();
    dcp_test5();
    dcp_test6();

    return 0;
}
/// \end::hello_dcp[]
