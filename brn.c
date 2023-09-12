/* brn is a bulk rename program for the shell.
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
#include <libgen.h>
#include <limits.h>

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

typedef struct {
	Identifier_t identifier;
	char *basename;
	bool remove_ext;
} UserData_t;

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
    -e                 Remove extension from the files\n\
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

UserData_t _UserData;

static bool is_dir(const char *path) {

	struct stat st;

	if (stat(path, &st) < 0)
		return -1;

	return S_ISDIR(st.st_mode);
}

static char *construct_new_filename(const char *origpath, const char *newnamepart) {

	char *filepath_copy;
	char *basepath;
	char *newpath = NULL;
	const char *ext;
	int basepathlen;
	int extlen = 0;
	int newnamepartlen;
	char sep[2];
	char dot[2];
	size_t newpathlen;

	/* As we use strcat with these, they must be strings */
	sep[0] = '/';
	dot[0] = '.';
	sep[1] = '\0';
	dot[1] = '\0';

	filepath_copy = strdup(origpath);
	basepath = dirname(filepath_copy);
	ext = strrchr(origpath, '.');

	if (!ext || ext == origpath)
		ext = NULL;
	else
		ext = ext + 1;
	
	basepathlen = strlen(basepath) + 2;
	newnamepartlen = strlen(newnamepart);

	if (ext != NULL)
		extlen = strlen(ext);

	/* +1 for the dot (eg. .png) */
	newpathlen = (basepathlen + newnamepartlen + extlen + 1) * sizeof(char);
	
	newpath = malloc(newpathlen);

	if (newpath == NULL) {
		free(filepath_copy);
		fprintf(stderr, "Malloc failed. Abort.\n");
		exit(EXIT_FAILURE);
	}
	
	strncpy(newpath, basepath, strlen(basepath) + 1);
	strncat(newpath, sep, strlen(sep) + 1);
	strncat(newpath, newnamepart, strlen(newnamepart) + 1);

	if (ext != NULL && !_UserData.remove_ext) {
		strncat(newpath, dot, strlen(dot)+1);
		strncat(newpath, ext, strlen(ext)+1);
	}

	free(filepath_copy);
	
	return newpath;
}

static bool random_identifier(const char *filepath) {

	bool retval = true;
	char *newnamepart = NULL;
	const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const size_t chars_size = sizeof(chars) - 1;
	char *newpath = NULL;

	newnamepart = malloc((strlen(_UserData.basename) + 9) * sizeof(char));

	if (newnamepart == NULL) {
		fprintf(stderr, "Malloc failed. Abort.\n");
		exit(EXIT_FAILURE);
	}

	strncpy(newnamepart, _UserData.basename, strlen(_UserData.basename) + 1);

	for (int n = 0; n < 8; n++) {
		char c;
		int index = rand() % chars_size;

		c = chars[index];
		strncat(newnamepart, &c, 1);
	}

	newpath = construct_new_filename(filepath, newnamepart);
	
	if (rename(filepath, newpath) == -1) {
		perror("random_identifier");
		retval = false;
	}

	free(newpath);
	free(newnamepart);
	
	return retval;
}

static bool select_identifier(const char *filepath) {

	bool retval = false;

	switch(_UserData.identifier) {
		case DEFAULT:
			break;
		case ORIGINAL:
			break;
		case RANDOM:
			retval = random_identifier(filepath);
			break;
		case SHA256:
			break;
	}
	
	return retval;
}

static int nftw_cb(const char *filepath, const struct stat *st,
					int tflag, struct FTW *ftwbuffer) {

	if (tflag == FTW_F) {
		if (access(filepath, F_OK) == -1) {
			fprintf(stderr, "%s does not exist, skipping...\n", filepath);
		}
		else {
			if (!select_identifier(filepath))
				fprintf(stderr, "Renaming %s failed\n", filepath);
		}
	}
	
	return 0;						
}

static void walk_path(const char *path, int fd_limit) {

	int fflags = 0;

	fflags |= FTW_DEPTH;
	fflags |= FTW_PHYS;
	
	if (nftw(path, nftw_cb, fd_limit, fflags) == -1) {
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

	_UserData.remove_ext = false;
	_UserData.identifier = DEFAULT;
	
	while (optind < argc) {
		if ((c = getopt(argc, argv, "b:ehorsV")) != -1) {
			switch (c) {
				case 'b': //basename
					_UserData.basename = optarg;			
					break;
				case 'e':
					_UserData.remove_ext = true;
					break;	
				case 'h':
					usage();
					return 0;
					break;
				case 'o': //use original name as id
					if (iflag_set == 0) {
						_UserData.identifier = ORIGINAL;
						iflag_set = 1;
						printf("set o\n");
					}
					else {
						fprintf(stderr, "Another flag already set, ignoring -o\n");
					}
					break;
				case 'r': //use generate random id (8 chars)
					if (iflag_set == 0) {
						srand(time(NULL));
						_UserData.identifier = RANDOM;
						iflag_set = 1;
					}
					else {
						fprintf(stderr, "Another flag already set, ignoring -r\n");
					}
					break;
				case 's': //use sha 256 as the new name, ignore b
					if (iflag_set == 0) {
						_UserData.identifier = SHA256;
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

	if (_UserData.basename == NULL)
		fprintf(stderr, "You must set the basename (-b) for the files.\n");
	else
		walk_path(path, 15);

	return 0;
}
