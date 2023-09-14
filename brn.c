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
#include <math.h>

#define VERSION "0.4"

typedef enum {
	/* append (n) to the basename. Eg. myphoto(1).jpg */
	DEFAULT = 1,
	/* use last modified date of the file as an identifier */
	FILE_DATE,
	/* generate 8 chars long random identifier */
	RANDOM	
} Identifier_t;

typedef struct {
	Identifier_t identifier;
	char *basename;
	bool remove_ext;
	size_t file_count;
	bool top_dir_only;
	bool execute_script;
	char *script_file_path;
} data_t;

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
    -c <path>          After renaming, execute script for each file\n\
    -e                 Remove extension from the files\n\
    -r                 Generate random, 8 characters long identifier\n\
    -t                 Do not traverse into subdirectories of the path\n\
    -d                 Use last modified date of the file as an identifier\n\
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

data_t _data_t;

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

	if (ext != NULL && !_data_t.remove_ext) {
		strncat(newpath, dot, strlen(dot)+1);
		strncat(newpath, ext, strlen(ext)+1);
	}

	free(filepath_copy);
	//TODO: check if file exist, if yes, return nullptr
	return newpath;
}

/* Uses last modified date of the file as an identifier */
static bool identifier_file_date(const char *filepath) {

	bool retval = true;
	char *newnamepart= NULL;
	char *newpath = NULL;
	char date[20];
	struct stat attributes;
	struct tm *ltime= NULL;
	
	/* +11 for the date (format: 2023-09-14T13:24:59) and for the trailing zero */
	newnamepart = malloc((strlen(_data_t.basename) + 20 ) * sizeof(char));

	if (newnamepart == NULL) {
		fprintf(stderr, "Malloc failed. Abort.\n");
		exit(EXIT_FAILURE);
	}

	strncpy(newnamepart, _data_t.basename, strlen(_data_t.basename) + 1);

	if (stat(filepath, &attributes) == -1) {
		perror("identifier_file_date");
		free(newnamepart);
		return false;	
	}

	ltime = localtime(&(attributes.st_mtime));

	if (ltime == NULL) {
		free(newnamepart);
		perror("identifier_file_date");
		return false;
	}

	strftime(date, 20, "%Y-%m-%dT%H:%M:%S", ltime);
	strncat(newnamepart, date, strlen(date));

	newpath = construct_new_filename(filepath, newnamepart);

	if (rename(filepath, newpath) == -1) {
		perror("identifier_file_date");
		free(newpath);
		free(newnamepart);
		return false;
	}

	free(newpath);
	free(newnamepart);
	
	return retval;
}

/* Implements the default behaviour of Brn.
 *
 * If no specified option is given this will be called.
 * Adds a counter to the filename.
 * For example test.txt becomes test(1).txt and so on.
 */
static bool identifier_count(const char *filepath) {

	bool retval = true;
	char *newnamepart = NULL;
	char *newpath = NULL;
	size_t length;
	char pa = '(';
	char pe = ')';
	char *ctmp = NULL;
	
	/* count the digits in the file_count */
	length = (_data_t.file_count == 0) ? 1 : log10(_data_t.file_count) + 1;
	/* +3 for ( and ) and the trailing zero */
	newnamepart = malloc((strlen(_data_t.basename) + length + 3) *sizeof(char));

	if (newnamepart == NULL) {
		fprintf(stderr, "Malloc failed. Abort.\n");
		exit(EXIT_FAILURE);
	}

	strncpy(newnamepart, _data_t.basename, strlen(_data_t.basename) + 1);
	strncat(newnamepart, &pa, 1);

	ctmp = malloc(length + 1);

	if (ctmp == NULL) {
		free(newnamepart);
		fprintf(stderr, "Malloc failed. Abort.\n");
		exit(EXIT_FAILURE);
	}
	
	snprintf(ctmp, length + 1, "%zu", _data_t.file_count);
	
	strncat(newnamepart, ctmp, strlen(ctmp));
	strncat(newnamepart, &pe, 1);

	newpath = construct_new_filename(filepath, newnamepart);

	if (rename(filepath, newpath) == -1) {
		perror("identifier_count");
		free(newpath);
		free(newnamepart);
		free(ctmp);

		return false;
	}

	free(newpath);
	free(newnamepart);
	free(ctmp);
	
	return retval;
}

/* Implements the random identifier.
 * 
 * Filename pointed by filepath is appended with 8 random characters.
 * For example test.txt becomes testRYUGHTQW.txt
 */
static bool identifier_random(const char *filepath) {

	bool retval = true;
	char *newnamepart = NULL;
	const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const size_t chars_size = sizeof(chars) - 1;
	char *newpath = NULL;

	newnamepart = malloc((strlen(_data_t.basename) + 9) * sizeof(char));

	if (newnamepart == NULL) {
		fprintf(stderr, "Malloc failed. Abort.\n");
		exit(EXIT_FAILURE);
	}

	strncpy(newnamepart, _data_t.basename, strlen(_data_t.basename) + 1);

	for (int n = 0; n < 8; n++) {
		char c;
		int index = rand() % chars_size;

		c = chars[index];
		strncat(newnamepart, &c, 1);
	}

	newpath = construct_new_filename(filepath, newnamepart);
	
	if (rename(filepath, newpath) == -1) {
		perror("identifier_random");
		retval = false;
	}

	free(newpath);
	free(newnamepart);
	
	return retval;
}

static bool select_identifier(const char *filepath) {

	bool retval = false;

	switch(_data_t.identifier) {
		case DEFAULT:
			retval = identifier_count(filepath);
			break;
		case FILE_DATE:
			retval = identifier_file_date(filepath);
			break;
		case RANDOM:
			retval = identifier_random(filepath);
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
			_data_t.file_count++;
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

	_data_t.remove_ext = false;
	_data_t.identifier = DEFAULT;
	_data_t.file_count = 0;
	_data_t.top_dir_only = false;
	_data_t.execute_script = false;
	_data_t.script_file_path = NULL;
	
	while (optind < argc) {
		if ((c = getopt(argc, argv, "b:c:ehortdV")) != -1) {
			switch (c) {
				case 'b': //basename
					_data_t.basename = optarg;			
					break;
				case 'c': /* Execute script point by optarg for each file */
					_data_t.execute_script = true;
					_data_t.script_file_path = optarg;
					break;
				case 'e':
					_data_t.remove_ext = true;
					break;	
				case 'h':
					usage();
					return 0;
					break;
				case 'r': /* Append filenames with 8 random characters */
					if (iflag_set == 0) {
						srand(time(NULL));
						_data_t.identifier = RANDOM;
						iflag_set = 1;
					}
					else {
						fprintf(stderr, "Another flag already set, ignoring -r\n");
					}
					break;
				case 't': /* Do not traverse into the subdirectories of the path */
					_data_t.top_dir_only = true;
					break;
				case 'd': /* Use last modified date of the file as an identifier */
					if (iflag_set == 0) {
						_data_t.identifier = FILE_DATE;
						iflag_set = 1;
					}
					else {
						fprintf(stderr, "Another flag already set, ignoring -C\n");
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

	if (_data_t.basename == NULL)
		fprintf(stderr, "You must set the basename (-b) for the files.\n");
	else
		walk_path(path, 15);

	return 0;
}
