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

#define _XOPEN_SOURCE 700
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <ftw.h>
#include <time.h>

#define VERSION "0.4"

typedef enum {
	/* append (n) to the basename. Eg. myphoto(1).jpg */
	DEFAULT = 1,
	/* use original filename as identifier */
	ORIGINAL,
	/* generate 8 chars long random identifier */
	RANDOM,
	/* take sha256 from the file and use it as the name */
	SHA256
	
} Identifier_t;


static Identifier_t _identifier = DEFAULT;
static char *_basename = NULL;

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
    -s                 Use SHA256 of the file as a new name. Ignores -b\n\
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

static bool is_dir(const char *path) {

	struct stat st;

	if (stat(path, &st) < 0)
		return -1;

	return S_ISDIR(st.st_mode);
}

static void set_default_basename() {

	time_t rawtime;
	struct tm *time_info;
	char strdate[11];

	time( &rawtime );
	time_info = localtime( &rawtime );

	strftime(strdate,11,"%Y-%m-%d", time_info);
}

static bool rename_file(const char *filepath) {

	switch(_identifier) {
		case DEFAULT:
			break;
		case ORIGINAL:
			break;
		case RANDOM:
			break;
		case SHA256:
			break;
	}
	
	return true;
}

static int do_work(const char *filepath, const struct stat *st,
					int tflag, struct FTW *ftwbuffer) {

	if (tflag == FTW_F) {
		if (access(filepath, F_OK) == -1)
			fprintf(stderr, "%s does not exist, skipping...\n", filepath);
		else
			rename_file(filepath);
	}
	
	return 0;						
}

static void walk_path(const char *path, int fd_limit) {

	int fflags = 0;

	fflags |= FTW_DEPTH;
	fflags |= FTW_PHYS;
	
	if (nftw(path, do_work, fd_limit, fflags) == -1) {
		perror("nftw");
		exit(EXIT_FAILURE);
	}
}

int main (int argc, char *argv[]) {

	int c;
	char *path = NULL;
	
	int iflag_set = 0;

	if (argc == 1) {
		/* We don't have any arguments. Show usage and exit. */
		usage();
		return 0;
	}

	//TODO: Set default _basename to be date

	while (optind < argc) {
		if ((c = getopt(argc, argv, "b:horsV")) != -1) {
			switch (c) {
				case 'b': //basename
					_basename = optarg;			
					break;
				case 'h':
					usage();
					return 0;
					break;
				case 'o': //use original name as id
					if (iflag_set == 0) {
						_identifier = ORIGINAL;
						iflag_set = 1;
						printf("set o\n");
					}
					else {
						fprintf(stderr, "Another flag already set, ignoring -o\n");
					}
					break;
				case 'r': //use generate random id (8 chars)
					if (iflag_set == 0) {
						_identifier = RANDOM;
						iflag_set = 1;
						printf("set r\n");
					}
					else {
						fprintf(stderr, "Another flag already set, ignoring -r\n");
					}
					break;
				case 's': //use sha 256 as the new name, ignore b
					if (iflag_set == 0) {
						_identifier = SHA256;
						iflag_set = 1;
						printf("set s\n");
					}
					else {
						fprintf(stderr, "Another flag already set, ignoring -s\n");
					}
					break;
				case 'V':
					printf("brn version %s\n", VERSION);
					return 0;
				case '?':
					usage();
					return 0;
			}
		}
		else {
			path = argv[1];

			if (!is_dir(path)) {
				fprintf(stderr, "%s is not a valid directory. Abort.\n", path);
				return 0;
			}
			
			optind++;
		}
	}

	walk_path(path, 15);

	return 0;
}
