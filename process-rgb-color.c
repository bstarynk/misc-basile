/*** file process-rgb-color.c from https://github.com/bstarynk/misc-basile/ ***/
/*** SPDX-License-Identifier: GPLv3+ ***/

/*******************************************************************************
 * © Copyright 2024 Basile STARYNKEVITCH (France)
 * email <basile@starynkevitch.net>
 * License: GPLv3+ (file COPYING-GPLv3)
 *    This software is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

char*progname;
int main(int argc, char**argv)
{
  char linebuf[128];
  char colorbuf[80];
  progname = argv[0];
  if (argc>1 && !strcmp(argv[1], "--help")) {
    printf("%s: process the /etc/X11/rgb.txt file and emit on stdout C macro expressions\n"
	   "RPS_RGB_COLOR(Red,Green,Blue,\"colorname\");\n", progname);
    return 0;
  }
  char* rgbfilename = "/etc/X11/rgb.txt";
  if (argc>1 && !access(argv[1], R_OK))
    rgbfilename = argv[1];
  FILE* rgbf = fopen(rgbfilename, "r");
  if (!rgbf) {
    fprintf(stderr, "%s: cannot fopen %s - %s\n", progname, rgbfilename, strerror(errno));
    exit(EXIT_FAILURE);
  };
  do {
    memset(linebuf, 0, sizeof(linebuf));
    memset (colorbuf, 0, sizeof(colorbuf));
    if (!fgets(linebuf, sizeof(linebuf)-4, rgbf))
      break;
    int r=0, g=0, b=0, i=0;
    if (sscanf(linebuf, "%d %d %d %64s %n", &r, &g, &b, colorbuf, &i) <=3)
      continue;
    printf("RPS_RGB_COLOR(%d,%d,%d,\"%s\");\n", r, g, b, colorbuf);
  } while (!feof(rgbf));
  printf("/// end of colors\n");
  return 0;
}
