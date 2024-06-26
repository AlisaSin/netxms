/* 
** nxgenguid - command line tool for GUID generation
** Copyright (C) 2004-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxgenguid.cpp
**
**/

#include <nms_util.h>
#include <netxms_getopt.h>
#include <netxms-version.h>
#include <uuid.h>

NETXMS_EXECUTABLE_HEADER(nxgenguid)

/**
 * main
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   // Parse command line
   bool nl = true;
   opterr = 1;
   int ch;
   while((ch = getopt(argc, argv, "hnv")) != -1)
   {
      switch(ch)
      {
         case 'h':   // Display help and exit
            printf("Usage: nxgenguid [<options>]\n"
                   "Valid options are:\n"
                   "   -h           : Display help and exit.\n"
                   "   -n           : Do not add newline.\n"
                   "   -v           : Display version and exit.\n"
                   "\n");
            return 0;
         case 'n':   // No newline
            nl = false;
            break;
         case 'v':   // Print version and exit
            printf("NetXMS GUID Generation Tool Version " NETXMS_VERSION_STRING_A "\n");
            return 0;
         case '?':
            return 1;
         default:
            break;
      }
   }

   _tprintf(_T("%s%s"), uuid::generate().toString().cstr(), nl ? _T("\n") : _T(""));
   return 0;
}
