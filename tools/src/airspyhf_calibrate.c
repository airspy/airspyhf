/*
 * Copyright 2012 Jared Boone <jared@sharebrained.com>
 * Copyright 2013 Michael Ossmann <mike@ossmann.com>
 * Copyright 2013/2014 Benjamin Vernoux <bvernoux@airspy.com>
 * Copyright 2018 Andrea Montefusco IW0HDV <andrew@montefusco.com>
 * Copyright 2020 Kenji Rikitake JJ1BDX <kenji.rikitake@acm.org>
 *
 * This file is part of AirSpyHF+ (based on HackRF project).
 *
 * Compile with:
 *
 * gcc -Wall airspyhf_info.c  -lairspyhf -o airspyhf_info -lm
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

/*
 * print all the receiver data available from libairspyhf
 */
void print_receiver_data (struct airspyhf_device* pd)
{
	if (pd) {
		airspyhf_read_partid_serialno_t read_partid_serialno;
		unsigned nsrates;
		char vstr[255]; // the size of buffer length has to be restricted to 1 byte
		int32_t ppb;

		if (airspyhf_board_partid_serialno_read(pd, &read_partid_serialno) == AIRSPYHF_SUCCESS) {
			printf("S/N: 0x%08X%08X / ",
					read_partid_serialno.serial_no[0],
					read_partid_serialno.serial_no[1]
			);
			printf("Part ID: 0x%08X / ", read_partid_serialno.part_id);
		} else {
			fprintf(stderr, "airspyhf_board_partid_serialno_read() failed\n");
		}
		if (airspyhf_version_string_read(pd, vstr, sizeof(vstr)) == AIRSPYHF_SUCCESS)
			printf("Firmware Version: %s\n", vstr);
		else
			fprintf(stderr, "airspyhf_version_string_read() failed\n");

		if (airspyhf_get_calibration(pd, &ppb) == AIRSPYHF_SUCCESS) {
			printf("Calibration = %d\n", ppb);
		} else {
			fprintf(stderr, "airspyhf_get_calibration() failed\n");
		}
	}
}

static void usage(void)
{
	printf("Usage:\n");
	printf("\t-s serial number: open receiver with specified 64-bit serial number (required).\n");
}



int main(const int argc, char * const *argv)
{
	int opt;
	airspyhf_lib_version_t libv;
	unsigned serial_number = 0;
	unsigned long long sn;
	unsigned ndev;

	// scan command line options
	while( (opt = getopt(argc, argv, "?hs:")) != EOF ) {

		uint32_t sn_msb;
		uint32_t sn_lsb;

		switch (opt) {
		case 's':
			if (sscanf(optarg, "0x%llx", &sn) == 1) {
				sn_msb = (uint32_t)(sn >> 32);
				sn_lsb = (uint32_t)(sn & 0xFFFFFFFF);
				serial_number = 1;
			} else {
				fprintf(stderr, "argument error: '-%c %s'\n", opt, optarg);
				usage();
				return EXIT_FAILURE;
			}
			break;

		default:
			fprintf(stderr, "unknown argument '-%c'\n", opt);

		case 'h':
		case '?':
			usage();
			return EXIT_FAILURE;
		}
	}

	// scan all devices, return how many are attached
	ndev = airspyhf_list_devices(0, 0);

	if (ndev <= 0) {
		fprintf (stderr, "No devices attached.\n");
		return EXIT_FAILURE;
	}

	if (serial_number) {
		struct airspyhf_device *dev;

		if (airspyhf_open_sn(&dev, sn) == AIRSPYHF_SUCCESS) {
			print_receiver_data (dev);
			return EXIT_SUCCESS;
		} else {
			fprintf (stderr, "Unable to open device with S/N 0x%16llX\n", sn);
			return EXIT_FAILURE;
		}
	} else {
		usage();
		return EXIT_FAILURE;
	}
}
