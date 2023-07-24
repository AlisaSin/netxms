/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: ef.cpp
**
**/

#include "nxcore.h"

/**
 * Setup event forwarding session
 */
BOOL EF_SetupSession(ISCSession *, NXCPMessage *request)
{
	return TRUE;
}

/**
 * Close event forwarding session
 */
void EF_CloseSession(ISCSession *)
{
}

/**
 * Process event forwarding session message
 */
BOOL EF_ProcessMessage(ISCSession *session, NXCPMessage *request, NXCPMessage *response)
{
	UINT32 code, id;
	TCHAR *name;

	if (request->getCode() == CMD_FORWARD_EVENT)
	{
	   TCHAR buffer[64];
		DbgPrintf(4, _T("Event forwarding request from %s"), IpToStr(session->GetPeerAddress(), buffer));

		shared_ptr<NetObj> object;
		id = request->getFieldAsUInt32(VID_OBJECT_ID);
		if (id != 0)
			object = FindObjectById(id);  // Object is specified explicitely
		else
			object = FindNodeByIP(0, request->getFieldAsInetAddress(VID_IP_ADDRESS));	// Object is specified by IP address
		
		if (object != nullptr)
		{
			name = request->getFieldAsString(VID_EVENT_NAME);
			if (name != nullptr)
			{
				DbgPrintf(5, _T("Event specified by name (%s)"), name);
				shared_ptr<EventTemplate> pt = FindEventTemplateByName(name);
				if (pt != nullptr)
				{
					code = pt->getCode();
					DbgPrintf(5, _T("Event name %s resolved to event code %d"), name, code);
				}
				else
				{
					code = 0;
					DbgPrintf(5, _T("Event name %s cannot be resolved"), name);
				}
				MemFree(name);
			}
			else
			{
				code = request->getFieldAsUInt32(VID_EVENT_CODE);
				DbgPrintf(5, _T("Event specified by code (%d)"), code);
			}

			bool success;
			if (request->isFieldExist(VID_EVENT_ARG_NAMES_BASE))
			{
            success = EventBuilder(code, object->getId())
               .origin(EventOrigin::REMOTE_SERVER)
               .tags(request->getFieldAsSharedString(VID_TAGS))
               .params(*request, VID_EVENT_ARG_BASE, VID_EVENT_ARG_NAMES_BASE, VID_NUM_ARGS)
               .post();
         }
         else
         {
            success = EventBuilder(code, object->getId())
               .origin(EventOrigin::REMOTE_SERVER)
               .tags(request->getFieldAsSharedString(VID_TAGS))
               .params(*request, VID_EVENT_ARG_BASE, VID_NUM_ARGS)
               .post();
         }
         response->setField(VID_RCC, success ? ISC_ERR_SUCCESS : ISC_ERR_POST_EVENT_FAILED);
		}
		else
		{
			response->setField(VID_RCC, ISC_ERR_OBJECT_NOT_FOUND);
		}
	}
	else
	{
		response->setField(VID_RCC, ISC_ERR_NOT_IMPLEMENTED);
	}
	return FALSE;	// Don't close session
}
