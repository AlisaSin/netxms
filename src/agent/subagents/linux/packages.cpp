/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2013-2023 Victor Kirhenshtein
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
**/

#include "linux_subagent.h"
#include <sys/utsname.h>

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
#if _OPENWRT
   const TCHAR *command = _T("opkg list-installed | awk -e '{ print \"@@@ #\" $1 \"|\" $3 \"||||\" }'");
   bool shellExec = true;
#else
   bool shellExec;
   const TCHAR *command;
   if (access("/bin/rpm", X_OK) == 0)
   {
		command = _T("/bin/rpm -qa --queryformat '@@@ #%{NAME}:%{ARCH}|%{VERSION}%|RELEASE?{-%{RELEASE}}:{}||%{VENDOR}|%{INSTALLTIME}|%{URL}|%{SUMMARY}\\n'");
		shellExec = false;
   }
   else if (access("/usr/bin/dpkg-query", X_OK) == 0)
   {
		command = _T("/usr/bin/dpkg-query -W -f '@@@${Status}#${package}:${Architecture}|${version}|||${homepage}|${description}\\n' | grep '@@@install.*installed.*#'");
      shellExec = true;
	}
	else
	{
		return SYSINFO_RC_UNSUPPORTED;
	}
#endif

   struct utsname un;
   const TCHAR *arch;
#ifdef UNICODE
   TCHAR machine[64];
#endif
   if (uname(&un) != -1)
   {
      if (!strcmp(un.machine, "i686") || !strcmp(un.machine, "i586") || !strcmp(un.machine, "i486") || !strcmp(un.machine, "i386"))
      {
         arch = _T(":i686:i586:i486:i386");
      }
      else if (!strcmp(un.machine, "amd64") || !strcmp(un.machine, "x86_64"))
      {
         arch = _T(":amd64:x86_64");
      }
      else
      {
#ifdef UNICODE
         machine[0] = 0;
         mb_to_wchar(un.machine, -1, &machine[1], 63);
         arch = machine;
#else
         memmove(&un.machine[1], un.machine, strlen(un.machine) + 1);
         un.machine[0] = ':';
         arch = un.machine;
#endif
      }
   }
   else
   {
      arch = _T(":i686:i586:i486:i386");
   }

   LineOutputProcessExecutor executor(command, shellExec);
   if (!executor.execute())
      return SYSINFO_RC_ERROR;

   if (!executor.waitForCompletion(5000))
      return SYSINFO_RC_ERROR;

	value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
	value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"), true);
	value->addColumn(_T("VENDOR"), DCI_DT_STRING, _T("Vendor"));
	value->addColumn(_T("DATE"), DCI_DT_STRING, _T("Install Date"));
	value->addColumn(_T("URL"), DCI_DT_STRING, _T("URL"));
	value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));

	for(int i = 0; i < executor.getData().size(); i++)
	{
		TCHAR line[1024];
		_tcslcpy(line, executor.getData().get(i), 1024);

		if (_tcsncmp(line, _T("@@@"), 3))
			continue;

		value->addRow();
		TCHAR *curr = _tcschr(&line[3], _T('#'));
		if (curr != nullptr)
			curr++;
		else
			curr = &line[3];
		for(int i = 0; i < 6; i++)
		{
			TCHAR *ptr = _tcschr(curr, _T('|'));
			if (ptr != nullptr)
				*ptr = 0;

			// Remove architecture from package name if it is the same as
			// OS architecture or package is architecture-independent
			if (i == 0)
			{
			   TCHAR *pa = _tcsrchr(curr, _T(':'));
			   if (pa != nullptr)
			   {
			      if (!_tcscmp(pa, _T(":all")) || !_tcscmp(pa, _T(":noarch")) || !_tcscmp(pa, _T(":(none)")) || (_tcsstr(arch, pa) != nullptr))
			         *pa = 0;
			   }
			}

			value->set(i, curr);

			if (ptr == nullptr)
				break;
			curr = ptr + 1;
		}
	}

	return SYSINFO_RC_SUCCESS;
}
