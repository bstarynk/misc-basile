
/**
    file helloworld1.c 
    Copyright © 2018 Basile Starynkevitch.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

----
    Contact me (Basile Starynkevitch) by email
    basile@starynkevitch.net and/or basile.starynkevitch@cea.fr
**/
#include <stdio.h>

void
sayhello (const char *msg)
{
  if (!msg)
    printf ("Hello, World!\n");
  else
    printf ("Hello from %s\n", msg);
}

int
main (int argc, char **argv)
{
  if (argc > 1)
    sayhello (argv[1]);
  else
    sayhello (NULL);
}

// end of file helloworld1.c
