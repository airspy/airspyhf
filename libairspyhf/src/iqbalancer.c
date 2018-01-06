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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iqbalancer.h"

#define MATH_PI 3.14159265359

static uint8_t __window_initialized = 0;
static float __window[FFTBins];

static void __init_window()
{
	int i;

	if (__window_initialized)
	{
		return;
	}

	const int length = FFTBins - 1;

	for (i = 0; i <= length; i++)
	{
		__window[i] = (float) (
			+ 0.27105140069342
			- 0.43329793923448 * cos(2.0 * MATH_PI * i / length)
			+ 0.21812299954311 * cos(4.0 * MATH_PI * i / length)
			- 0.06592544638803 * cos(6.0 * MATH_PI * i / length)
			+ 0.01081174209837 * cos(8.0 * MATH_PI * i / length)
			- 0.00077658482522 * cos(10.0 * MATH_PI * i / length)
			+ 0.00001388721735 * cos(12.0 * MATH_PI * i / length)
			);
	}

	__window_initialized = 1;
}

static void window(airspyhf_complex_float_t *buffer, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		buffer[i].re *= __window[i];
		buffer[i].im *= __window[i];
	}
}

static void fft(airspyhf_complex_float_t *buffer, int length)
{
	int nm1 = length - 1;
	int nd2 = length / 2;
	int i, j, jm1, k, l, m, le, le2, ip;
	airspyhf_complex_float_t u, t, r;

	m = 0;
	i = length;
	while (i > 1)
	{
		++m;
		i = (i >> 1);
	}

	j = nd2;

	for (i = 1; i < nm1; ++i)
	{
		if (i < j)
		{
			t = buffer[j];
			buffer[j] = buffer[i];
			buffer[i] = t;
		}

		k = nd2;

		while (k <= j)
		{
			j = j - k;
			k = k / 2;
		}

		j += k;
	}

	for (l = 1; l <= m; ++l)
	{
		le = 1 << l;
		le2 = le / 2;

		u.re = 1.0f;
		u.im = 0.0f;

		r.re = (float) cos(MATH_PI / le2);
		r.im = (float) -sin(MATH_PI / le2);

		for (j = 1; j <= le2; ++j)
		{
			jm1 = j - 1;

			for (i = jm1; i <= nm1; i += le)
			{
				ip = i + le2;

				t.re = u.re * buffer[ip].re - u.im * buffer[ip].im;
				t.im = u.im * buffer[ip].re + u.re * buffer[ip].im;

				buffer[ip].re = buffer[i].re - t.re;
				buffer[ip].im = buffer[i].im - t.im;

				buffer[i].re += t.re;
				buffer[i].im += t.im;
			}

			t.re = u.re * r.re - u.im * r.im;
			t.im = u.im * r.re + u.re * r.im;

			u.re = t.re;
			u.im = t.im;
		}
	}
}

static void cancel_dc(iq_balancer_t *iq_balancer, airspyhf_complex_float_t* iq, int length)
{
	int i;
	float iavg = iq_balancer->iavg;
	float qavg = iq_balancer->qavg;

	for (i = 0; i < length; i++)
	{
		iavg += DCAlpha * (iq[i].re - iavg);
		qavg += DCAlpha * (iq[i].im - qavg);

		iq[i].re -= iavg;
		iq[i].im -= qavg;
	}

	iq_balancer->iavg = iavg;
	iq_balancer->qavg = qavg;
}

static void adjust_phase(iq_balancer_t *iq_balancer, airspyhf_complex_float_t *iq, float phase)
{
	int i;
	float gain = (float) iq_balancer->gain;
	for (i = 0; i < FFTBins; i++)
	{
		iq[i].re += phase * iq[i].im;
		iq[i].im *= gain;
	}
}

static void multiply_complex_complex(airspyhf_complex_float_t *a, const airspyhf_complex_float_t *b)
{
	float re = a->re * b->re - a->im * b->im;
	a->im = a->im * b->re + a->re * b->im;
	a->re = re;
}

static float fsign(const float x)
{
	return x >= 0 ? 1.0f : -1.0f;
}

static float utility(iq_balancer_t *iq_balancer, airspyhf_complex_float_t* iq, float phase)
{
	int i, j;
	airspyhf_complex_float_t acc;
	airspyhf_complex_float_t prod;

	airspyhf_complex_float_t fftPtr[FFTBins * sizeof(airspyhf_complex_float_t)];

	memcpy(fftPtr, iq, FFTBins * sizeof(airspyhf_complex_float_t));

	adjust_phase(iq_balancer, fftPtr, phase);

	window(fftPtr, FFTBins);
	fft(fftPtr, FFTBins);

	acc.re = 0.0f;
	acc.im = 0.0f;

	for (i = 1, j = FFTBins - 1; i < FFTBins / 2; i++, j--)
	{
		prod = fftPtr[i];
		multiply_complex_complex(&prod, fftPtr + j);
		acc.re += prod.re;
		acc.im += prod.im;
	}

	return acc.re * acc.re + acc.im * acc.im;
}

static void estimate_phase_imbalance(iq_balancer_t *iq_balancer, airspyhf_complex_float_t* iq)
{
	float u = utility(iq_balancer, iq, iq_balancer->phase);

	float phase = iq_balancer->phase + iq_balancer->step;
	if (phase > MaxPhaseCorrection)
	{
		phase = MaxPhaseCorrection;
	}
	if (phase < -MaxPhaseCorrection)
	{
		phase = -MaxPhaseCorrection;
	}

	float candidateUtility = utility(iq_balancer, iq, phase);

	if (candidateUtility < u)
	{
		iq_balancer->phase += PhaseAlpha * (phase - iq_balancer->phase);
		iq_balancer->step *= StepIncrement;
	}
	else
	{
		iq_balancer->step *= -StepDecrement;
	}

	float stepmag = fabsf(iq_balancer->step);
	float stepsign = fsign(iq_balancer->step);

	if (stepmag < MinimumStep)
	{
		iq_balancer->step = MinimumStep * stepsign;
	}
	else if (stepmag > MaximumStep)
	{
		iq_balancer->step = MaximumStep * stepsign;
	}
}

static void adjust_phase_amplitude(iq_balancer_t *iq_balancer, airspyhf_complex_float_t* iq, int length)
{
	int i;
	float scale = 1.0f / (length - 1);

	for (i = 0; i < length; i++)
	{
		float phase = (i * iq_balancer->last_phase + (length - 1 - i) * iq_balancer->phase) * scale;

		iq[i].re += phase * iq[i].im;

		float re = iq[i].re * iq[i].re;
		float im = iq[i].im * iq[i].im;

		iq_balancer->iampavg += DCAlpha * (re - iq_balancer->iampavg);
		iq_balancer->qampavg += DCAlpha * (im - iq_balancer->qampavg);

		if (iq_balancer->qampavg != 0)
		{
			double gain = sqrt(iq_balancer->iampavg / iq_balancer->qampavg);
			iq_balancer->gain = iq_balancer->gain + GainAlpha * (gain - iq_balancer->gain);
		}

		iq[i].im *= (float)iq_balancer->gain;
	}

	iq_balancer->last_phase = iq_balancer->phase;
}

void iq_balancer_process(iq_balancer_t *iq_balancer, airspyhf_complex_float_t* iq, int length)
{
	cancel_dc(iq_balancer, iq, length);

	uint8_t i = 0;
	while (length >= FFTBins)
	{
		if (++i == SkippedBuffers)
		{
			estimate_phase_imbalance(iq_balancer, iq);
			i = 0;
		}
		adjust_phase_amplitude(iq_balancer, iq, FFTBins);
		iq += FFTBins;
		length -= FFTBins;
	}

	adjust_phase_amplitude(iq_balancer, iq, length);
}

void iq_balancer_init(iq_balancer_t *iq_balancer)
{
	iq_balancer->iavg = 0.0f;
	iq_balancer->qavg = 0.0f;
	iq_balancer->phase = 0.0f;
	iq_balancer->last_phase = 0.0f;
	iq_balancer->step = MinimumStep;
	iq_balancer->gain = 1.0;
	iq_balancer->iampavg = 1.0;
	iq_balancer->qampavg = 1.0;

	__init_window();
}
