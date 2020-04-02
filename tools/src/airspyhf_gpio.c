/*
 * Copyright 2018 Andrea Montefusco IW0HDV <andrew@montefusco.com>
 *
 * This file is part of AirSpyHF+ (based on HackRF project).
 *
 * Compile with:
 *
 * gcc -Wall airspyhf_gpio.c  -lairspyhf -o airspyhf_gpio -lm
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <airspyhf.h>

static void usage(void)
{
	fprintf(stderr,
	"Usage:\n"
	"\t-s serial number: open receiver with specified 64 bits serial number.\n"
	"\t-0 on|off\n"
	"\t-1 on|off\n"
	"\t-2 on|off\n"
	"\t-3 on|off\n"
	);
}

typedef struct {
	airspyhf_user_output_t			gpio;
	airspyhf_user_output_state_t	state;
} GPIO;


int main (const int argc, char **argv)
{
	int opt;
	unsigned n;
	airspyhf_device_t* dev = 0;
	airspyhf_lib_version_t libv;
	unsigned serial_number = 0;
	unsigned long long sn;
	unsigned ndev;
	GPIO gpio [] =
	{
		{ AIRSPYHF_USER_OUTPUT_0 , AIRSPYHF_USER_OUTPUT_LOW },
		{ AIRSPYHF_USER_OUTPUT_1 , AIRSPYHF_USER_OUTPUT_LOW },
		{ AIRSPYHF_USER_OUTPUT_2 , AIRSPYHF_USER_OUTPUT_LOW },
		{ AIRSPYHF_USER_OUTPUT_3 , AIRSPYHF_USER_OUTPUT_LOW }
	};


	while( (opt = getopt(argc, argv, "s:0:1:2:3:h?")) != EOF ) {

		uint32_t sn_msb;
		uint32_t sn_lsb;

		switch( opt )
		{
			case '0':
			case '1':
			case '2':
			case '3':
			{
				if (strcmp (optarg, "on") == 0)
					gpio [opt - '0'].state = AIRSPYHF_USER_OUTPUT_HIGH;
				else
				if (strcmp (optarg, "off") == 0)
					gpio [opt - '0'].state = AIRSPYHF_USER_OUTPUT_LOW;
				else {
					fprintf (stderr, "Bad GPIO status: %s.\n", optarg);
					goto exit_usage;
				}
			}
			break;

			case 's':
				if (sscanf(optarg, "0x%llx", &sn) == 1) {
					sn_msb = (uint32_t)(sn >> 32);
					sn_lsb = (uint32_t)(sn & 0xFFFFFFFF);
					printf("Receiver serial number to be opened: 0x%08X%08X\n",
							sn_msb, sn_lsb);
					serial_number = 1;
				} else {
					fprintf(stderr, "argument error: '-%c %s'\n", opt, optarg);
					goto exit_usage;
				}
			break;

			default:
				fprintf(stderr,"unknown argument '-%c %s'\n", opt, optarg);

			case 'h':
			case '?':
				goto exit_usage;
		}
	}
	// print the library version (this is a value built into libairspyhf library)
	airspyhf_lib_version (&libv);
	fprintf(stderr, "AirSpy HF library version: %d.%d.%d\n",
			libv.major_version, libv.minor_version, libv.revision);

	// scan all devices, return how many are attached
	ndev = airspyhf_list_devices(0, 0);

	if (ndev <= 0) {
		fprintf (stderr, "No devices attached.\n");
		return EXIT_FAILURE;
	}

	if(serial_number) {
		if (airspyhf_open_sn(&dev, sn) != AIRSPYHF_SUCCESS) {
			fprintf (stderr, "Unable to open device with S/N 0x%16llX\n", sn);
			goto exit_error;
		}
	} else {
		uint64_t *serials = 0;
		unsigned n;

		// setup space for serials reading
		serials = malloc (ndev*sizeof(*serials));
		// read all the serials and scan the receiver(s)
		if (serials && airspyhf_list_devices(serials, ndev) > 0) {
			uint64_t *ps = serials;

			for (n=1; n < ndev+1; ++n, ++ps) {
				if (airspyhf_open_sn(&dev, *ps) != AIRSPYHF_SUCCESS) {
					fprintf(stderr, "airspyhf_open() receiver #%d failed\n", n);
					goto exit_error;
				}
			}
			free (serials);
		}
	}
	// update GPIO
	for (n = 0; n < ((sizeof(gpio))/(sizeof(gpio[0]))); ++n) {
		if (airspyhf_set_user_output(dev,
									 gpio[n].gpio,
									 gpio[n].state
									) != AIRSPYHF_SUCCESS) {
			fprintf (stderr, "Error setting GPIO #%d\n",n);
			goto exit_error;
		} else {
			fprintf (stderr, "GPIO #%d: %d\n", n, gpio[n].state);
		}
	}
	airspyhf_close(dev);
	return EXIT_SUCCESS;

exit_error:
	airspyhf_close(dev);
	return EXIT_FAILURE;

exit_usage:
	usage();
	return EXIT_FAILURE;
}
