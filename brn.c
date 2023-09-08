/* brn is a batch rename tool for the shell.
 *
 * Copyright (C) 2023 Niko Rosvall <niko@byteptr.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * If you port brn to another platform, please let me know.
 * It would be nice to know where brn runs on.
 *
 * brn should be fairly portable to any POSIX system.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glob.h>

#define VERSION "0.4"

static void usage()
{
#define HELP "\
SYNOPSIS\n\
\n\
    brn <path> [options]\n\
\n\
OPTIONS\n\
\n\
    -b <name>          Set basename for the files\n\
    -o                 Use original filename as an identifier\n\
    -r                 Generate random, 8 characters long identifier\n\
    -s                 Use SHA256 of the file as an identifier\n\
\n\
    -h                 Show short help and exit. This page\n\
    -V                 Show version number of the program\n\
\n\
For more information and examples see man brn(1).\n\
\n\
AUTHORS\n\
    Copyright (C) 2023 Niko Rosvall <niko@byteptr.com>\n\
\n\
    Released under license GPL-3+. For more information, see\n\
    http://www.gnu.org/licenses\n\
"

	printf(HELP);
}


int main (int argc, char *argv[]) {

	int index;
	int c;
	char *basename = NULL;
	int oflag = 0;
	int rflag = 0;
	int sflag = 0;

	if (argc == 1) {
		/* We don't have any arguments. Show usage and exit. */
		usage();
		return 0;
	}
	
	while ((c = getopt(argc, argv, "b:horsV")) != -1) {
		switch (c) {
			case 'b': //basename
				break;
			case 'h':
				usage();
				break;
			case 'o': //use original name as id
				oflag = 1;
				break;
			case 'r': //use generate random id (8 chars)
				rflag = 1;
				break;
			case 's': //use sha 256 as an id
				sflag = 1;
				break;
			case 'V':
				printf("brn version %s\n", VERSION);
				break;
			case '?':
				usage();
				return 0;
		}
	}

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);

	return 0;
}
