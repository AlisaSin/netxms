/*
** NetXMS - Network Management System
** Copyright (C) 2021 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
**/

#include "libnetxms.h"

#if !HAVE_STRCASESTR

/**
 * Find substring in a string ignoring the case of both arguments
 */
char LIBNETXMS_EXPORTABLE *strcasestr(const char *s, const char *ss)
{
   char c;
   if ((c = *ss++) == 0)
      return const_cast<char*>(s);

   c = tolower(c);
   size_t sslen = strlen(ss);
   do
   {
      char sc;
      do
      {
         if ((sc = *s++) == 0)
            return nullptr;
      } while(static_cast<char>(tolower(sc)) != c);
   } while (strnicmp(s, ss, sslen) != 0);
   s--;
   return const_cast<char*>(s);
}

#endif
