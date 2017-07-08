/*
Copyright (c) 2013, Michael Ossmann <mike@ossmann.com>
Copyright (c) 2012, Jared Boone <jared@sharebrained.com>
Copyright (c) 2013-2017, Youssef Touil <youssef@airspy.com>
Copyright (c) 2013-2017, Ian Gilmour <ian@sdrsharp.com>
Copyright (c) 2013-2017, Benjamin Vernoux <bvernoux@airspy.com>

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

#ifndef __AIRSPYHF_H__
#define __AIRSPYHF_H__

#include <stdint.h>

#define AIRSPYHF_VERSION "1.0.0"
#define AIRSPYHF_VER_MAJOR 1
#define AIRSPYHF_VER_MINOR 0
#define AIRSPYHF_VER_REVISION 0

#define AIRSPYHF_ENDPOINT_IN (1)

#ifdef _WIN32
	 #define ADD_EXPORTS
	 
	/* You should define ADD_EXPORTS *only* when building the DLL. */
	#ifdef ADD_EXPORTS
		#define ADDAPI __declspec(dllexport)
	#else
		#define ADDAPI __declspec(dllimport)
	#endif

	/* Define calling convention in one place, for convenience. */
	#define ADDCALL __cdecl

#else /* _WIN32 not defined. */

	/* Define with no value on non-Windows OSes. */
	#define ADDAPI
	#define ADDCALL

#endif

#ifdef __cplusplus
extern "C"
{
#endif

enum airspyhf_error
{
	AIRSPYHF_SUCCESS = 0,
	AIRSPYHF_ERROR = -1
};

typedef struct {
	float re;
	float im;
} airspyhf_complex_float_t;

typedef struct {
	uint32_t part_id;
	uint32_t serial_no[4];
} airspyhf_read_partid_serialno_t;

enum airspyhf_board_id
{
	AIRSPYHF_BOARD_ID_UNKNOWN_AIRSPYHF = 0,
	AIRSPYHF_BOARD_ID_AIRSPYHF_REV_A = 1,
	AIRSPYHF_BOARD_ID_INVALID = 0xFF,
};

typedef struct airspyhf_device airspyhf_device_t;

typedef struct {
	airspyhf_device_t* device;
	void* ctx;
	airspyhf_complex_float_t* samples;
	int sample_count;
	uint64_t dropped_samples;
} airspyhf_transfer_t;

typedef int (*airspyhf_sample_block_cb_fn) (airspyhf_transfer_t* transfer_fn);

extern ADDAPI int ADDCALL airspyhf_open(airspyhf_device_t** device);
extern ADDAPI int ADDCALL airspyhf_open_sn(airspyhf_device_t** device, uint64_t serial_number);
extern ADDAPI int ADDCALL airspyhf_close(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_start(airspyhf_device_t* device, airspyhf_sample_block_cb_fn callback, void* ctx);
extern ADDAPI int ADDCALL airspyhf_stop(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_is_streaming(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_set_freq(airspyhf_device_t* device, const uint32_t freq_hz);
extern ADDAPI int ADDCALL airspyhf_get_samplerates(airspyhf_device_t* device, uint32_t* buffer, const uint32_t len);
extern ADDAPI int ADDCALL airspyhf_set_samplerate(airspyhf_device_t* device, uint32_t samplerate);
extern ADDAPI int ADDCALL airspyhf_get_calibration(airspyhf_device_t* device, int32_t* ppb);
extern ADDAPI int ADDCALL airspyhf_set_calibration(airspyhf_device_t* device, int32_t ppb);
extern ADDAPI int ADDCALL airspyhf_flash_calibration(airspyhf_device_t* device);
extern ADDAPI int ADDCALL airspyhf_board_partid_serialno_read(airspyhf_device_t* device, airspyhf_read_partid_serialno_t* read_partid_serialno);

#ifdef __cplusplus
}
#endif

#endif//__AIRSPYHF_H__
