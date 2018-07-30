/*
Copyright (c) 2016-2018, Youssef Touil <youssef@airspy.com>
Copyright (c) 2018, Leif Asbrink <leif@sm5bsz.com>

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

#ifndef MATH_PI
#define MATH_PI 3.14159265359
#endif

#define EPSILON 0.01f

struct iq_balancer_t
{
	float phase;
	float last_phase;
	float phase_step;

	float amplitude;
	float last_amplitude;
	float amplitude_step;

	float iavg;
	float qavg;

	float raw_phases[MaxLookback];
	float raw_amplitudes[MaxLookback];
	int no_of_avg;
	int no_of_raw;
	int raw_ptr;
	int initial_skip;

	int optimal_bin;

	complex_t *corr;
	complex_t *corr_plus;
	float *boost;

	int estimation_count;
};

static uint8_t __lib_initialized = 0;
static float __fft_window[FFTBins];
static float __boost_window[FFTBins];

static void __init_library()
{
	int i;

	if (__lib_initialized)
	{
		return;
	}

	const int length = FFTBins - 1;

	for (i = 0; i <= length; i++)
	{
		__fft_window[i] = (float) (
			+ 0.35875f
			- 0.48829f * cos(2.0 * MATH_PI * i / length)
			+ 0.14128f * cos(4.0 * MATH_PI * i / length)
			- 0.01168f * cos(6.0 * MATH_PI * i / length)
			);
		__boost_window[i] = (float) (1.0 / BoostFactor + 1.0 / exp(pow(i * 2.0 / BinsToOptimize, 2.0)));
	}

	__lib_initialized = 1;
}

static void window(complex_t *buffer, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		buffer[i].re *= __fft_window[i];
		buffer[i].im *= __fft_window[i];
	}
}

static void fft(complex_t *buffer, int length)
{
	int nm1 = length - 1;
	int nd2 = length / 2;
	int i, j, jm1, k, l, m, le, le2, ip;
	complex_t u, t, r;

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

	for (i = 0; i < nd2; i++)
	{
		j = nd2 + i;
		t = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = t;
	}
}

static void cancel_dc(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	int i;
	float iavg = iq_balancer->iavg;
	float qavg = iq_balancer->qavg;

	for (i = 0; i < length; i++)
	{
		iavg += DcTimeConst * (iq[i].re - iavg);
		qavg += DcTimeConst * (iq[i].im - qavg);

		iq[i].re -= iavg;
		iq[i].im -= qavg;
	}

	iq_balancer->iavg = iavg;
	iq_balancer->qavg = qavg;
}

static void adjust_benchmark(complex_t *iq, float phase, float amplitude)
{
	int i;
	for (i = 0; i < FFTBins; i++)
	{
		float re = iq[i].re;
		float im = iq[i].im;

		iq[i].re += phase * im;
		iq[i].im += phase * re;

		iq[i].re *= 1 + amplitude;
		iq[i].im *= 1 - amplitude;
	}
}

static complex_t multiply_complex_complex(complex_t *a, const complex_t *b)
{
	complex_t result;
	result.re = a->re * b->re - a->im * b->im;
	result.im = a->im * b->re + a->re * b->im;
	return result;
}

static void compute_corr(struct iq_balancer_t *iq_balancer, complex_t* iq, complex_t* ccorr, int length, int step)
{
	complex_t cc, fftPtr[FFTBins];
	int n, m;
	int i, j;
	float phase = iq_balancer->phase + step * iq_balancer->phase_step;
	float amplitude = iq_balancer->amplitude + step * iq_balancer->amplitude_step;
	memset(fftPtr, 0, FFTBins * sizeof(complex_t));
	for (n = 0, m = 0; n <= length - FFTBins && m < FFTIntegration; n += FFTBins / 2, m++)
	{
		memcpy(fftPtr, iq + n, FFTBins * sizeof(complex_t));
		adjust_benchmark(fftPtr, phase, amplitude);
		window(fftPtr, FFTBins);
		fft(fftPtr, FFTBins);
		for (i = 1, j = FFTBins - 1; i < FFTBins / 2; i++, j--)
		{
			cc = multiply_complex_complex(fftPtr + i, fftPtr + j);
			ccorr[i].re += cc.re;
			ccorr[i].im += cc.im;

			ccorr[j].re = ccorr[i].re;
			ccorr[j].im = ccorr[i].im;
		}
		if (step == 0)
		{
			for (i = 0; i < FFTBins; i++)
			{
				iq_balancer->boost[i] += fftPtr[i].re * fftPtr[i].re + fftPtr[i].im * fftPtr[i].im;
			}
		}
	}
}

static complex_t utility(struct iq_balancer_t *iq_balancer, complex_t* ccorr)
{
	int i;
	int j;
	float invskip = 1.0f / BinsToSkip;
	complex_t acc = { 0, 0 };
	for (i = 1, j = FFTBins - 1; i < FFTBins; i++, j--)
	{
		int distance = abs(i - FFTBins / 2);
		if (distance > 0)
		{
			float weight = (distance > BinsToSkip) ? 1.0f : (distance * invskip);
			weight *= __boost_window[abs(iq_balancer->optimal_bin - i)];
			weight *= iq_balancer->boost[j] / (iq_balancer->boost[i] + EPSILON);
			acc.re += ccorr[i].re * weight;
			acc.im += ccorr[i].im * weight;
		}
	}
	return acc;
}

static void estimate_imbalance(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	int i, j;
	float amplitude, phase, mu;
	complex_t a, b;

	if (iq_balancer->no_of_avg == 0)
	{
		memset(iq_balancer->boost, 0, FFTBins * sizeof(float));
		memset(iq_balancer->corr, 0, FFTBins * sizeof(complex_t));
		memset(iq_balancer->corr_plus, 0, FFTBins * sizeof(complex_t));
	}
	compute_corr(iq_balancer, iq, iq_balancer->corr, length, 0);
	compute_corr(iq_balancer, iq, iq_balancer->corr_plus, length, 1);
	iq_balancer->no_of_avg++;
	if (iq_balancer->no_of_avg <= CorrelationIntegration)
		return;
	iq_balancer->no_of_avg = 0;
	iq_balancer->no_of_raw++;
	if (iq_balancer->no_of_raw > MaxLookback - 1)
		iq_balancer->no_of_raw = MaxLookback - 1;
	iq_balancer->initial_skip++;
	if (iq_balancer->initial_skip > MaxLookback - 1)
		iq_balancer->initial_skip = MaxLookback - 1;
	a = utility(iq_balancer, iq_balancer->corr);
	b = utility(iq_balancer, iq_balancer->corr_plus);

	mu = a.im - b.im;
	mu = a.im / mu;
	if (fabs(mu) < 1)
	{
		iq_balancer->phase_step /= StepIncrement;
		if (iq_balancer->phase_step < MinPhaseStep)
		{
			iq_balancer->phase_step = MinPhaseStep;
		}
	}
	else if (fabs(mu) > 10)
	{
		iq_balancer->phase_step *= StepIncrement;
		if (iq_balancer->phase_step > MaxPhaseStep)
		{
			iq_balancer->phase_step = MaxPhaseStep;
		}
	}
	if (mu < -20.f)
		mu = -20.f;
	else if (mu > 20.f)
		mu = 20.f;
	phase = iq_balancer->phase + iq_balancer->phase_step * mu;

	mu = a.re - b.re;
	mu = a.re / mu;
	if (fabs(mu) < 1)
	{
		iq_balancer->amplitude_step /= StepIncrement;
		if (iq_balancer->amplitude_step < MinAmplitudeStep)
		{
			iq_balancer->amplitude_step = MinAmplitudeStep;
		}
	}
	else if (fabs(mu) > 10)
	{
		iq_balancer->amplitude_step *= StepIncrement;
		if (iq_balancer->amplitude_step > MaxAmplitudeStep)
		{
			iq_balancer->amplitude_step = MaxAmplitudeStep;
		}
	}
	if (mu < -20.f)
		mu = -20.f;
	else if (mu > 20.f)
		mu = 20.f;
	amplitude = iq_balancer->amplitude + iq_balancer->amplitude_step * mu;
	
	if (iq_balancer->initial_skip == 5)
		iq_balancer->no_of_raw = 1;

	if (iq_balancer->initial_skip >= 5)
	{
		iq_balancer->raw_amplitudes[iq_balancer->raw_ptr] = amplitude;
		iq_balancer->raw_phases[iq_balancer->raw_ptr] = phase;
		i = iq_balancer->raw_ptr;
		for (j = 1; j <= iq_balancer->no_of_raw; j++)
		{
			phase += iq_balancer->raw_phases[i];
			amplitude += iq_balancer->raw_amplitudes[i];
			i = (i + MaxLookback - 1) & (MaxLookback - 1);
		}
		phase /= (iq_balancer->no_of_raw + 1);
		amplitude /= (iq_balancer->no_of_raw + 1);
		iq_balancer->raw_ptr = (iq_balancer->raw_ptr + 1) & (MaxLookback - 1);
	}

	iq_balancer->phase = phase;
	iq_balancer->amplitude = amplitude;
}

static void adjust_phase_amplitude(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	int i;
	float scale = 1.0f / (length - 1);

	for (i = 0; i < length; i++)
	{
		float phase = (i * iq_balancer->last_phase + (length - 1 - i) * iq_balancer->phase) * scale;
		float amplitude = (i * iq_balancer->last_amplitude + (length - 1 - i) * iq_balancer->amplitude) * scale;

		float re = iq[i].re;
		float im = iq[i].im;

		iq[i].re += phase * im;
		iq[i].im += phase * re;

		iq[i].re *= 1 + amplitude;
		iq[i].im *= 1 - amplitude;
	}

	iq_balancer->last_phase = iq_balancer->phase;
	iq_balancer->last_amplitude = iq_balancer->amplitude;
}

void ADDCALL  iq_balancer_process(struct iq_balancer_t *iq_balancer, complex_t* iq, int length)
{
	cancel_dc(iq_balancer, iq, length);
	if (++iq_balancer->estimation_count >  BuffersToSkip)
	{
		estimate_imbalance(iq_balancer, iq, length);
		iq_balancer->estimation_count = 0;
	}
	adjust_phase_amplitude(iq_balancer, iq, length);
}

void ADDCALL iq_balancer_set_optimal_point(struct iq_balancer_t *iq_balancer, float w)
{
	if (w < 0.0f)
	{
		w = -w;
	}
	
	if (w > 0.5f)
	{
		w = 0.5f;
	}

	iq_balancer->optimal_bin = (int) floor(FFTBins * (0.5 + w));
	iq_balancer->initial_skip = 0;
}

struct iq_balancer_t * ADDCALL iq_balancer_create()
{
	struct iq_balancer_t *instance = (struct iq_balancer_t *) malloc(sizeof(struct iq_balancer_t));
	memset(instance, 0, sizeof(struct iq_balancer_t));

	instance->corr = (complex_t *) malloc(FFTBins * sizeof(complex_t));
	instance->corr_plus = (complex_t *) malloc(FFTBins * sizeof(complex_t));
	instance->boost = (float *) malloc(FFTBins * sizeof(float));

	instance->phase_step = InitialPhaseStep;
	instance->amplitude_step = InitialAmplitudeStep;

	__init_library();

	return instance;
}

void ADDCALL iq_balancer_destroy(struct iq_balancer_t *iq_balancer)
{
	free(iq_balancer->corr);
	free(iq_balancer->corr_plus);
	free(iq_balancer->boost);
	free(iq_balancer);
}
