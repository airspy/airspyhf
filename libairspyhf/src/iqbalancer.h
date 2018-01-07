/*
Copyright (c) 2016-2017, Youssef Touil <youssef@airspy.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
Neither the name of Airspy HF+ nor the names of its contributors may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __IQ_BALANCER_H__
#define __IQ_BALANCER_H__

#include "airspyhf.h"

#define FFTBins 512
#define SkippedBuffers 4
#define MaximumFail 10
#define MaximumStep 1e-2f
#define MinimumStep 1e-7f
#define StepIncrement 1.1f
#define StepDecrement 0.9f
#define MaxPhaseCorrection 0.2f
#define PhaseAlpha 0.01f
#define GainAlpha 0.01f
#define DCAlpha 2.68744225e-5f

typedef struct _iq_balancer_t
{
	float iavg;
	float qavg;
	float phase;
	float last_phase;
	float step;
	int fail;
	double gain;
	double iampavg;
	double qampavg;
} iq_balancer_t;

void iq_balancer_init(iq_balancer_t *iq_balancer);
void iq_balancer_process(iq_balancer_t *iq_balancer, airspyhf_complex_float_t* iq, int length);

#endif
