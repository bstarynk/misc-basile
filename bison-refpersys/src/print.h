/* Print information on generated parser, for bison,

   Copyright (C) 2000, 2009-2015, 2018-2022 Free Software Foundation,
   Inc.

   This file is part of Bison, the GNU Compiler Compiler.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.

   Modified in 2024 by Basile STARYNKEVITCH in France
   <basile@starynkevitch.net>q
*/

#ifndef PRINT_H_
# define PRINT_H_

void print_results (void);

#define PRINT_FROM_COMMENT(Fil) do{fprintf(Fil,"//§ from %s@%s:%d\n", \
    __FUNC__, __FILE__, __LINE__);} while(0)

#endif /* !PRINT_H_ */
