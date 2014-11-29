/* $Id$ */

#include <nms_common.h>
#include <nms_agent.h>

#include "main.h"
#include "net.h"

/**
 * Check SSH service - parameter handler
 */
LONG H_CheckSSH(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	LONG nRet = SYSINFO_RC_SUCCESS;

	char szHost[256];
	TCHAR szPort[256];
	TCHAR szTimeout[64];
	unsigned short nPort;

   AgentGetParameterArgA(param, 1, szHost, sizeof(szHost));
   AgentGetParameterArg(param, 2, szPort, sizeof(szPort));
   AgentGetParameterArg(param, 3, szTimeout, sizeof(szTimeout));

	if (szHost[0] == 0)
	{
		return SYSINFO_RC_ERROR;
	}

	nPort = (unsigned short)_tcstoul(szPort, NULL, 10);
	if (nPort == 0)
	{
		nPort = 22;
	}

	UINT32 dwTimeout = _tcstoul(szTimeout, NULL, 0);
   INT64 start = GetCurrentTimeMs();
	int result = CheckSSH(szHost, 0, nPort, NULL, NULL, dwTimeout);
   if (*arg == 'R')
   {
	   ret_int64(value, GetCurrentTimeMs() - start);
   }
   else
   {
	   ret_int(value, result);
   }
	return nRet;
}

/**
 * Check SSH service
 */
int CheckSSH(char *szAddr, UINT32 dwAddr, short nPort, char *szUser, char *szPass, UINT32 dwTimeout)
{
	int nRet = 0;
	SOCKET nSd;

	nSd = NetConnectTCP(szAddr, dwAddr, nPort, dwTimeout);
	if (nSd != INVALID_SOCKET)
	{
		char szBuff[512];
		char szTmp[128];

		nRet = PC_ERR_HANDSHAKE;

		if (NetCanRead(nSd, 1000))
		{
			if (NetRead(nSd, szBuff, sizeof(szBuff)) >= 8)
			{
				int nMajor, nMinor;

				if (sscanf(szBuff, "SSH-%d.%d-", &nMajor, &nMinor) == 2)
				{
					snprintf(szTmp, sizeof(szTmp), "SSH-%d.%d-NetXMS\n",
							nMajor, nMinor);
					if (NetWrite(nSd, szTmp, (int)strlen(szTmp)) > 0)
					{
						nRet = PC_ERR_NONE;
					}
				}
			}
		}

		NetClose(nSd);
	}
	else
	{
		nRet = PC_ERR_CONNECT;
	}

	return nRet;
}
