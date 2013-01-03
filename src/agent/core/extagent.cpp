/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: extagent.cpp
**
**/

#include "nxagentd.h"


/**
 *Static data
 */
static ObjectArray<ExternalSubagent> s_subagents;

/**
 * Constructor
 */
ExternalSubagent::ExternalSubagent(const TCHAR *name, const TCHAR *user)
{
	nx_strncpy(m_name, name, MAX_SUBAGENT_NAME);
	nx_strncpy(m_user, user, MAX_ESA_USER_NAME);
	m_connected = false;
	m_pipe = INVALID_PIPE_HANDLE;
	m_msgQueue = new MsgWaitQueue();
	m_requestId = 1;
	m_mutexPipeWrite = MutexCreate();
#ifdef _WIN32
	m_readEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#endif
}

/**
 * Destructor
 */
ExternalSubagent::~ExternalSubagent()
{
	delete m_msgQueue;
	MutexDestroy(m_mutexPipeWrite);
#ifdef _WIN32
	CloseHandle(m_readEvent);
#endif
}

/*
 * Send message to external subagent
 */
bool ExternalSubagent::sendMessage(CSCPMessage *msg)
{
	TCHAR buffer[256];
	bool success = false;

	AgentWriteDebugLog(6, _T("ExternalSubagent::sendMessage(%s): sending message %s"), m_name, NXCPMessageCodeName(msg->GetCode(), buffer));

	CSCP_MESSAGE *rawMsg = msg->CreateMessage();
	MutexLock(m_mutexPipeWrite);
#ifdef _WIN32
	DWORD bytes = 0;
	if (WriteFile(m_pipe, rawMsg, ntohl(rawMsg->dwSize), &bytes, NULL))
	{
		success = (bytes == ntohl(rawMsg->dwSize));
	}
#else
#endif
	MutexUnlock(m_mutexPipeWrite);
	free(rawMsg);
	return success;
}

/**
 * Wait for specific message to arrive
 */
CSCPMessage *ExternalSubagent::waitForMessage(WORD code, DWORD id)
{
	return m_msgQueue->waitForMessage(code, id, 5000);	// 5 sec timeout
}

/**
 * Read NXCP message from pipe
 */
CSCPMessage *ReadMessageFromPipe(HPIPE hPipe, HANDLE hEvent)
{
	BYTE buffer[8192];
	DWORD bytes;

#ifdef _WIN32
	if (hEvent != NULL)
	{
		OVERLAPPED ov;

		memset(&ov, 0, sizeof(OVERLAPPED));
		ov.hEvent = hEvent;
		if (!ReadFile(hPipe, buffer, 8192, NULL, &ov))
		{
			if (GetLastError() != ERROR_IO_PENDING)
				return NULL;
		}
		if (!GetOverlappedResult(hPipe, &ov, &bytes, TRUE))
		{
			return NULL;
		}
	}
	else
	{
		if (!ReadFile(hPipe, buffer, 8192, &bytes, NULL))
			return NULL;
	}
#else
	NXCPEncryptionContext *dummyCtx = NULL;
	CSCP_BUFFER nxcpBuffer;
	RecvNXCPMessage(0, NULL, &nxcpBuffer, 0, NULL, NULL, 0);
	bytes = RecvNXCPMessage(hPipe, (CSCP_MESSAGE *)buffer, &nxcpBuffer, 8192, &dummyCtx, NULL, 5000);
#endif

	if (bytes < CSCP_HEADER_SIZE)
		return NULL;

	if (ntohl(((CSCP_MESSAGE *)buffer)->dwSize) != bytes)
		return NULL;	// message size given in header does not match number of received bytes

	return new CSCPMessage((CSCP_MESSAGE *)buffer);
}

/**
 * Main connection thread
 */
void ExternalSubagent::connect(HPIPE hPipe)
{
	TCHAR buffer[256];

	m_pipe = hPipe;
	m_connected = true;
	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): connection established"), m_name);
	while(true)
	{
#ifdef _WIN32
		CSCPMessage *msg = ReadMessageFromPipe(hPipe, m_readEvent);
#else
		CSCPMessage *msg = ReadMessageFromPipe(hPipe, NULL);
#endif
		if (msg == NULL)
			break;
		AgentWriteDebugLog(6, _T("ExternalSubagent(%s): received message %s"), m_name, NXCPMessageCodeName(msg->GetCode(), buffer));
		m_msgQueue->put(msg);
	}

	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): connection closed"), m_name);
	m_connected = false;
	m_msgQueue->clear();
}

/**
 * Get list of supported parameters
 */
NETXMS_SUBAGENT_PARAM *ExternalSubagent::getSupportedParameters(DWORD *count)
{
	CSCPMessage msg;
	NETXMS_SUBAGENT_PARAM *result = NULL;

	msg.SetCode(CMD_GET_PARAMETER_LIST);
	msg.SetId(m_requestId++);
	if (sendMessage(&msg))
	{
		CSCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.GetId());
		if (response != NULL)
		{
			if (response->GetVariableLong(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->GetVariableLong(VID_NUM_PARAMETERS);
				result = (NETXMS_SUBAGENT_PARAM *)malloc(*count * sizeof(NETXMS_SUBAGENT_PARAM));
				DWORD varId = VID_PARAM_LIST_BASE;
				for(DWORD i = 0; i < *count; i++)
				{
					response->GetVariableStr(varId++, result[i].name, MAX_PARAM_NAME);
					response->GetVariableStr(varId++, result[i].description, MAX_DB_STRING);
					result[i].dataType = (int)response->GetVariableShort(varId++);
				}
			}
			delete response;
		}
	}
	return result;
}

/**
 * Get list of supported lists
 */
NETXMS_SUBAGENT_LIST *ExternalSubagent::getSupportedLists(DWORD *count)
{
	CSCPMessage msg;
	NETXMS_SUBAGENT_LIST *result = NULL;

	msg.SetCode(CMD_GET_ENUM_LIST);
	msg.SetId(m_requestId++);
	if (sendMessage(&msg))
	{
		CSCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.GetId());
		if (response != NULL)
		{
			if (response->GetVariableLong(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->GetVariableLong(VID_NUM_ENUMS);
				result = (NETXMS_SUBAGENT_LIST *)malloc(*count * sizeof(NETXMS_SUBAGENT_LIST));
				DWORD varId = VID_ENUM_LIST_BASE;
				for(DWORD i = 0; i < *count; i++)
				{
					response->GetVariableStr(varId++, result[i].name, MAX_PARAM_NAME);
				}
			}
			delete response;
		}
	}
	return result;
}

/**
 * Get list of supported tables
 */
NETXMS_SUBAGENT_TABLE *ExternalSubagent::getSupportedTables(DWORD *count)
{
	CSCPMessage msg;
	NETXMS_SUBAGENT_TABLE *result = NULL;

	msg.SetCode(CMD_GET_TABLE_LIST);
	msg.SetId(m_requestId++);
	if (sendMessage(&msg))
	{
		CSCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.GetId());
		if (response != NULL)
		{
			if (response->GetVariableLong(VID_RCC) == ERR_SUCCESS)
			{
				*count = response->GetVariableLong(VID_NUM_TABLES);
				result = (NETXMS_SUBAGENT_TABLE *)malloc(*count * sizeof(NETXMS_SUBAGENT_TABLE));
				DWORD varId = VID_TABLE_LIST_BASE;
				for(DWORD i = 0; i < *count; i++)
				{
					response->GetVariableStr(varId++, result[i].name, MAX_PARAM_NAME);
					response->GetVariableStr(varId++, result[i].instanceColumn, MAX_COLUMN_NAME);
					response->GetVariableStr(varId++, result[i].description, MAX_DB_STRING);
				}
			}
			delete response;
		}
	}
	return result;
}

/**
 * List supported parameters
 */
void ExternalSubagent::listParameters(CSCPMessage *msg, DWORD *baseId, DWORD *count)
{
	DWORD paramCount = 0;
	NETXMS_SUBAGENT_PARAM *list = getSupportedParameters(&paramCount);
	if (list != NULL)
	{
		DWORD id = *baseId;

		for(DWORD i = 0; i < paramCount; i++)
		{
			msg->SetVariable(id++, list[i].name);
			msg->SetVariable(id++, list[i].description);
			msg->SetVariable(id++, (WORD)list[i].dataType);
		}
		*baseId = id;
		*count += paramCount;
		free(list);
	}
}

/**
 * List supported parameters
 */
void ExternalSubagent::listParameters(StringList *list)
{
	DWORD paramCount = 0;
	NETXMS_SUBAGENT_PARAM *plist = getSupportedParameters(&paramCount);
	if (plist != NULL)
	{
		for(DWORD i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		free(plist);
	}
}

/**
 * List supported lists
 */
void ExternalSubagent::listLists(CSCPMessage *msg, DWORD *baseId, DWORD *count)
{
	DWORD paramCount = 0;
	NETXMS_SUBAGENT_LIST *list = getSupportedLists(&paramCount);
	if (list != NULL)
	{
		DWORD id = *baseId;

		for(DWORD i = 0; i < paramCount; i++)
		{
			msg->SetVariable(id++, list[i].name);
		}
		*baseId = id;
		*count += paramCount;
		free(list);
	}
}

/**
 * List supported lists
 */
void ExternalSubagent::listLists(StringList *list)
{
	DWORD paramCount = 0;
	NETXMS_SUBAGENT_LIST *plist = getSupportedLists(&paramCount);
	if (plist != NULL)
	{
		for(DWORD i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		free(plist);
	}
}

/**
 * List supported tables
 */
void ExternalSubagent::listTables(CSCPMessage *msg, DWORD *baseId, DWORD *count)
{
	DWORD paramCount = 0;
	NETXMS_SUBAGENT_TABLE *list = getSupportedTables(&paramCount);
	if (list != NULL)
	{
		DWORD id = *baseId;

		for(DWORD i = 0; i < paramCount; i++)
		{
			msg->SetVariable(id++, list[i].name);
			msg->SetVariable(id++, list[i].instanceColumn);
			msg->SetVariable(id++, list[i].description);
		}
		*baseId = id;
		*count += paramCount;
		free(list);
	}
}

/**
 * List supported tables
 */
void ExternalSubagent::listTables(StringList *list)
{
	DWORD paramCount = 0;
	NETXMS_SUBAGENT_TABLE *plist = getSupportedTables(&paramCount);
	if (plist != NULL)
	{
		for(DWORD i = 0; i < paramCount; i++)
			list->add(plist[i].name);
		free(plist);
	}
}

/**
 * Get parameter value from external subagent
 */
DWORD ExternalSubagent::getParameter(const TCHAR *name, TCHAR *buffer)
{
	CSCPMessage msg;
	DWORD rcc;

	msg.SetCode(CMD_GET_PARAMETER);
	msg.SetId(m_requestId++);
	msg.SetVariable(VID_PARAMETER, name);
	if (sendMessage(&msg))
	{
		CSCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.GetId());
		if (response != NULL)
		{
			rcc = response->GetVariableLong(VID_RCC);
			if (rcc == ERR_SUCCESS)
				response->GetVariableStr(VID_VALUE, buffer, MAX_RESULT_LENGTH);
			delete response;
		}
		else
		{
			rcc = ERR_INTERNAL_ERROR;
		}
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}

/**
 * Get table value from external subagent
 */
DWORD ExternalSubagent::getTable(const TCHAR *name, Table *value)
{
	CSCPMessage msg;
	DWORD rcc;

	msg.SetCode(CMD_GET_TABLE);
	msg.SetId(m_requestId++);
	msg.SetVariable(VID_PARAMETER, name);
	if (sendMessage(&msg))
	{
		CSCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.GetId());
		if (response != NULL)
		{
			rcc = response->GetVariableLong(VID_RCC);
			if (rcc == ERR_SUCCESS)
				value->updateFromMessage(response);
			delete response;
		}
		else
		{
			rcc = ERR_INTERNAL_ERROR;
		}
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}

/**
 * Get list value from external subagent
 */
DWORD ExternalSubagent::getList(const TCHAR *name, StringList *value)
{
	CSCPMessage msg;
	DWORD rcc;

	msg.SetCode(CMD_GET_LIST);
	msg.SetId(m_requestId++);
	msg.SetVariable(VID_PARAMETER, name);
	if (sendMessage(&msg))
	{
		CSCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.GetId());
		if (response != NULL)
		{
			rcc = response->GetVariableLong(VID_RCC);
			if (rcc == ERR_SUCCESS)
			{
            DWORD count = response->GetVariableLong(VID_NUM_STRINGS);
            for(DWORD i = 0; i < count; i++)
					value->addPreallocated(response->GetVariableStr(VID_ENUM_VALUE_BASE + i));
			}
			delete response;
		}
		else
		{
			rcc = ERR_INTERNAL_ERROR;
		}
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}

/**
 * Listener for external subagents
 */
#ifdef _WIN32

static THREAD_RESULT THREAD_CALL ExternalSubagentConnector(void *arg)
{
	ExternalSubagent *subagent = (ExternalSubagent *)arg;
	SECURITY_ATTRIBUTES sa;
	PSECURITY_DESCRIPTOR sd = NULL;
	SID_IDENTIFIER_AUTHORITY sidAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	EXPLICIT_ACCESS ea;
	PSID sidEveryone = NULL;
	ACL *acl = NULL;
	TCHAR pipeName[MAX_PATH], errorText[1024];

	// Create a well-known SID for the Everyone group.
	if(!AllocateAndInitializeSid(&sidAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sidEveryone))
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: AllocateAndInitializeSid failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Initialize an EXPLICIT_ACCESS structure for an ACE.
	// The ACE will allow either Everyone or given user to access pipe
	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
	ea.grfAccessPermissions = (FILE_GENERIC_READ | FILE_GENERIC_WRITE) & ~FILE_CREATE_PIPE_INSTANCE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = NO_INHERITANCE;
	const TCHAR *user = subagent->getUserName();
	if ((user[0] == 0) || !_tcscmp(user, _T("*")))
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName  = (LPTSTR)sidEveryone;
	}
	else
	{
		ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
		ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea.Trustee.ptstrName  = (LPTSTR)user;
	}

	// Create a new ACL that contains the new ACEs.
	if (SetEntriesInAcl(1, &ea, NULL, &acl) != ERROR_SUCCESS)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: SetEntriesInAcl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (sd == NULL)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: LocalAlloc failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	if (!InitializeSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION))
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: InitializeSecurityDescriptor failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	// Add the ACL to the security descriptor. 
   if (!SetSecurityDescriptorDacl(sd, TRUE, acl, FALSE))
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: SetSecurityDescriptorDacl failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = sd;
	_sntprintf(pipeName, MAX_PATH, _T("\\\\.\\pipe\\nxagentd.subagent.%s"), subagent->getName());
	HANDLE hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 8192, 8192, 0, &sa);
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		AgentWriteDebugLog(2, _T("ExternalSubagentConnector: CreateNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
		goto cleanup;
	}

	AgentWriteDebugLog(2, _T("ExternalSubagent(%s): named pipe created, waiting for connection"), subagent->getName());
	int connectErrors = 0;
	while(!(g_dwFlags & AF_SHUTDOWN))
	{
		BOOL connected = ConnectNamedPipe(hPipe, NULL);
		if (connected || (GetLastError() == ERROR_PIPE_CONNECTED))
		{
			subagent->connect(hPipe);
			DisconnectNamedPipe(hPipe);
			connectErrors = 0;
		}
		else
		{
			AgentWriteDebugLog(2, _T("ExternalSubagentConnector: ConnectNamedPipe failed (%s)"), GetSystemErrorText(GetLastError(), errorText, 1024));
			connectErrors++;
			if (connectErrors > 10)
				break;	// Stop this connector if ConnectNamedPipe fails instantly
		}
	}

cleanup:
	if (hPipe != NULL)
		CloseHandle(hPipe);

	if (sd != NULL)
		LocalFree(sd);

	if (acl != NULL)
		LocalFree(acl);

	if (sidEveryone != NULL)
		FreeSid(sidEveryone);

	return THREAD_OK;
}

#else

static THREAD_RESULT THREAD_CALL ExternalSubagentConnector(void *arg)
{
	return THREAD_OK;
}

#endif

/**
 * Add external subagent from config.
 * Each line in config should be in form 
 * name:user
 * If user part is omited or set to *, connection from any account will be accepted
 */
bool AddExternalSubagent(const TCHAR *config)
{
	TCHAR buffer[1024], user[256] = _T("*");

	nx_strncpy(buffer, config, 1024);
	TCHAR *ptr = _tcschr(buffer, _T(':'));
	if (ptr != NULL)
	{
		*ptr = 0;
		ptr++;
		_tcsncpy(user, ptr, 256);
		Trim(user);
	}

	ExternalSubagent *subagent = new ExternalSubagent(buffer, user);
	s_subagents.add(subagent);
	ThreadCreate(ExternalSubagentConnector, 0, subagent);
	return true;
}

/**
 * Add parameters from external providers to NXCP message
 */
void ListParametersFromExtSubagents(CSCPMessage *msg, DWORD *baseId, DWORD *count)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listParameters(msg, baseId, count);
		}
	}
}

/**
 * Add parameters from external subagents to string list
 */
void ListParametersFromExtSubagents(StringList *list)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listParameters(list);
		}
	}
}

/**
 * Add lists from external providers to NXCP message
 */
void ListListsFromExtSubagents(CSCPMessage *msg, DWORD *baseId, DWORD *count)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listLists(msg, baseId, count);
		}
	}
}

/**
 * Add lists from external subagents to string list
 */
void ListListsFromExtSubagents(StringList *list)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listLists(list);
		}
	}
}

/**
 * Add tables from external providers to NXCP message
 */
void ListTablesFromExtSubagents(CSCPMessage *msg, DWORD *baseId, DWORD *count)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listTables(msg, baseId, count);
		}
	}
}

/**
 * Add tables from external subagents to string list
 */
void ListTablesFromExtSubagents(StringList *list)
{
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			s_subagents.get(i)->listTables(list);
		}
	}
}

/**
 * Get value from external subagent
 *
 * @return agent error code
 */
DWORD GetParameterValueFromExtSubagent(const TCHAR *name, TCHAR *buffer)
{
	DWORD rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getParameter(name, buffer);
			if (rc == ERR_SUCCESS)
				break;
		}
	}
	return rc;
}

/**
 * Get table from external subagent
 *
 * @return agent error code
 */
DWORD GetTableValueFromExtSubagent(const TCHAR *name, Table *value)
{
	DWORD rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getTable(name, value);
			if (rc == ERR_SUCCESS)
				break;
		}
	}
	return rc;
}

/**
 * Get list from external subagent
 *
 * @return agent error code
 */
DWORD GetListValueFromExtSubagent(const TCHAR *name, StringList *value)
{
	DWORD rc = ERR_UNKNOWN_PARAMETER;
	for(int i = 0; i < s_subagents.size(); i++)
	{
		if (s_subagents.get(i)->isConnected())
		{
			rc = s_subagents.get(i)->getList(name, value);
			if (rc == ERR_SUCCESS)
				break;
		}
	}
	return rc;
}
