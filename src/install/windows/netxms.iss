; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#include "setup.iss"
OutputBaseFilename=netxms-1.1.8

[Components]
Name: "base"; Description: "Base Files"; Types: full compact custom; Flags: fixed
Name: "console"; Description: "Administrator's Console"; Types: full
Name: "tools"; Description: "Command Line Tools"; Types: full
Name: "server"; Description: "NetXMS Server"; Types: full compact
Name: "server\mssql"; Description: "Microsoft SQL Server 2008 Native Client"; Types: full
Name: "server\mysql"; Description: "MySQL Client Library"; Types: full
Name: "server\pgsql"; Description: "PostgreSQL Client Library"; Types: full
Name: "server\oracle"; Description: "Oracle Instant Client"; Types: full
Name: "websrv"; Description: "Web Server"; Types: full
Name: "jre"; Description: "Java Runtime Environment"; Types: full
Name: "pdb"; Description: "Install PDB files for selected components"; Types: custom

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Common files
Source: "..\..\..\ChangeLog"; DestDir: "{app}\doc"; Flags: ignoreversion; Components: base
Source: "..\..\..\Release\libnetxms.dll"; DestDir: "{app}\bin"; BeforeInstall: StopAllServices; Flags: ignoreversion; Components: base
Source: "..\..\..\Release\libnetxms.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libnetxmsw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\Release\libnetxmsw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libexpat.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\Release\libexpat.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\libtre.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\Release\libtre.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
Source: "..\..\..\Release\nxzlib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "..\..\..\Release\nxzlib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base and pdb
; Executables and DLLs shared between different components (server, console, etc.)
Source: "..\..\..\Release\libnxcl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools websrv
Source: "..\..\..\Release\libnxcl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (tools or websrv) and pdb
Source: "..\..\..\Release\libnxclw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console tools
Source: "..\..\..\Release\libnxclw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (console or tools) and pdb
Source: "..\..\..\Release\libnxmap.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server tools websrv
Source: "..\..\..\Release\libnxmap.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or tools or websrv) and pdb
Source: "..\..\..\Release\libnxmapw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console server
Source: "..\..\..\Release\libnxmapw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (console or server) and pdb
Source: "..\..\..\Release\libnxsnmpw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\..\Release\libnxsnmpw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\..\Release\libnxsl.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\..\Release\libnxsl.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: (server or console) and pdb
Source: "..\..\..\Release\nxscript.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server console
Source: "..\..\..\Release\nxconfig.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server websrv
; Server files
Source: "..\..\..\Release\nxsqlite.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxsqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxsnmp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\libnxsnmp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxlp.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\libnxlp.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxdb.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\libnxdb.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxdbw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\libnxdbw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\libnxsrv.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\libnxsrv.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxcore.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxcore.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\netxmsd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\netxmsd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\mysql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\mysql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\mssql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\mssql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\odbc.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\odbc.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\pgsql.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\pgsql.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\sqlite.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\sqlite.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\oracle.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\oracle.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\informix.ddr"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\informix.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\generic.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\generic.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxagent.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxagent.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\portech.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\portech.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\dbemu.sms"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\dbemu.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxaction.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxadm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxdbmgr.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxencpasswd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxget.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\nxsnmpget.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxsnmpwalk.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxsnmpset.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxupload.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxmibc.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\nxagentd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\winnt.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\winnt.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\winperf.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\winperf.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\wmi.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\wmi.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ping.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\ping.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\portcheck.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\portcheck.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ecs.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\ecs.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\logwatch.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\logwatch.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ups.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\ups.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\odbcquery.nsm"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\odbcquery.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\avaya-ers.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\avaya-ers.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\baystack.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\baystack.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\ers8000.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\ers8000.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\cisco.dll"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\cisco.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\cat2900xl.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\cat2900xl.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\catalyst.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\catalyst.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\Release\netscreen.ndd"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server
Source: "..\..\..\Release\netscreen.pdb"; DestDir: "{app}\lib\ndd"; Flags: ignoreversion; Components: server and pdb
Source: "..\..\..\sql\dbinit_mssql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_mysql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_oracle.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_pgsql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbinit_sqlite.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mssql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_mysql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_oracle.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_pgsql.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\sql\dbschema_sqlite.sql"; DestDir: "{app}\lib\sql"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\mibs\*.txt"; DestDir: "{app}\var\mibs"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\netxmsd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\contrib\nxagentd.conf-dist"; DestDir: "{app}\etc"; Flags: ignoreversion; Components: server
Source: "..\..\..\images\*"; DestDir: "{app}\var\images"; Flags: ignoreversion; Components: server
Source: "..\..\java\report-generator\target\report-generator.jar"; DestDir: "{app}\lib\java"; Flags: ignoreversion; Components: server
; Console files
Source: "..\..\..\Release\scilexer.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\Release\scilexer.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\Release\libnxsnmpw.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\Release\libnxsnmpw.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\Release\nxuilib.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\Release\nxuilib.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\Release\nxlexer.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\Release\nxlexer.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\Release\nxcon.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\Release\nxcon.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\Release\nxav.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\Release\nxav.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\..\Release\nxnotify.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console
Source: "..\..\..\Release\nxnotify.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: console and pdb
Source: "..\..\java\build\win32.win32.x86\nxmc\*"; DestDir: "{app}\bin"; Flags: ignoreversion recursesubdirs; Components: console
; Command-line tools files
Source: "..\..\..\Release\nxalarm.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\Release\nxsms.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\Release\nxevent.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
Source: "..\..\..\Release\nxpush.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: tools
; Web server files
Source: "..\..\..\Release\libgd.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv
Source: "..\..\..\Release\libgd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv and pdb
Source: "..\..\..\Release\nxhttpd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv
Source: "..\..\..\Release\nxhttpd.pdb"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: websrv and pdb
Source: "..\..\webui\nxhttpd\static\*.js"; DestDir: "{app}\var\www"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\netxms.css"; DestDir: "{app}\var\www"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\*.png"; DestDir: "{app}\var\www\images"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\buttons\normal\*.png"; DestDir: "{app}\var\www\images\buttons\normal"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\buttons\pressed\*.png"; DestDir: "{app}\var\www\images\buttons\pressed"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\ctrlpanel\*.png"; DestDir: "{app}\var\www\images\ctrlpanel"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\objects\*.png"; DestDir: "{app}\var\www\images\objects"; Flags: ignoreversion; Components: websrv
Source: "..\..\webui\nxhttpd\static\images\status\*.png"; DestDir: "{app}\var\www\images\status"; Flags: ignoreversion; Components: websrv
; Third party files
Source: "Files\libmysql.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\mysql
Source: "Files\libpq.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\libintl-8.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\libiconv-2.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\oci.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "Files\oraociei11.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\oracle
Source: "Files\ssleay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: server\pgsql
Source: "Files\libeay32.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: base
Source: "Files\jre\*"; DestDir: "{app}\bin\jre"; Flags: ignoreversion recursesubdirs; Components: jre
; Install-time files
Source: "Files\vcredist_x86.exe"; DestDir: "{app}\var"; DestName: "vcredist.exe"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "Files\rm.exe"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: base
Source: "Files\sqlncli.msi"; DestDir: "{app}\var"; Flags: ignoreversion deleteafterinstall; Components: server\mssql

#include "common.iss"

