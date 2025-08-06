// file mycppstream.cc in github.com/bstarynk/misc-basile/
// SPDX-License-Identifier: GPL-3.0-or-later

/***
    © Copyright (C) 2025 Basile Starynkevitch
    <basile@starynkevitch.net>

    This mycppstream.cc program is free software for Linux; you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation; either
    version 2, or (at your option) any later version.

***/

#define private public
#define protected public

#include <iostream>
#include <fstream>
#include <unistd.h>


int main(void)
{
  printf("sizeof cin=%zd\n", sizeof(std::cin));
  fflush(nullptr);
  std::cout << __FILE__ << ":" << __LINE__ << std::endl;
  std::ofstream myout("/dev/stdout");
  myout << " myout of size " << sizeof(myout)
	<< " align " << alignof(myout) << " " <<  __FILE__ << ":" << __LINE__ << std::endl;
  return 0;
}

