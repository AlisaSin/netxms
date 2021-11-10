/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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
** File: bizsvcbase.cpp
**
**/

#include "nxcore.h"

/**
 * Base business service default constructor
 */
BaseBusinessService::BaseBusinessService() : super(), AutoBindTarget(this)
{
   m_id = 0;
   m_pollingDisabled = false;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
}

/**
 * Base business service default constructor
 */
BaseBusinessService::BaseBusinessService(const TCHAR *name) : super(name, 0), AutoBindTarget(this)
{
   m_pollingDisabled = false;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
}

/**
 * Create business service from prototype
 */
BaseBusinessService::BaseBusinessService(const BaseBusinessService& prototype, const TCHAR *name) : super(name, 0), AutoBindTarget(this)
{
   m_pollingDisabled = false;
   m_objectStatusThreshhold = prototype.m_objectStatusThreshhold;
   m_dciStatusThreshhold = prototype.m_dciStatusThreshhold;

   for(int i = 0; i < MAX_AUTOBIND_TARGET_FILTERS; i++)
      setAutoBindFilter(i, prototype.m_autoBindFilterSources[i]);
   m_autoBindFlags = prototype.m_autoBindFlags;

   unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = prototype.getChecks();
   for (const shared_ptr<BusinessServiceCheck>& check : *checks)
   {
      BusinessServiceCheck *ch = new BusinessServiceCheck(m_id, *check.get());
      m_checks.add(ch);
      ch->saveToDatabase();
   }
}

/**
 * Base business service destructor
 */
BaseBusinessService::~BaseBusinessService()
{
}

/**
 * Get all checks
 */
unique_ptr<SharedObjectArray<BusinessServiceCheck>> BaseBusinessService::getChecks() const
{
   checksLock();
   auto checks = make_unique<SharedObjectArray<BusinessServiceCheck>>(m_checks);
   checksUnlock();
   return checks;
}

/**
 * Load business service checks from database
 */
bool BaseBusinessService::loadChecksFromDatabase(DB_HANDLE hdb)
{
   nxlog_debug_tag(DEBUG_TAG_BIZSVC, 4, _T("Loading service checks for business service %u"), m_id);

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id,service_id,type,description,related_object,related_dci,status_threshold,content,current_ticket FROM business_service_checks WHERE service_id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      DBFreeStatement(hStmt);
      return false;
   }

   int rows = DBGetNumRows(hResult);
   for (int i = 0; i < rows; i++)
   {
      m_checks.add(new BusinessServiceCheck(hResult, i));
   }

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
   return true;
}

/**
 * Delete business service check from service and database. Returns client RCC.
 */
uint32_t BaseBusinessService::deleteCheck(uint32_t checkId)
{
   uint32_t rcc = RCC_INVALID_BUSINESS_CHECK_ID;
   checksLock();
   SharedPtrIterator<BusinessServiceCheck> it = m_checks.begin();
   while (it.hasNext())
   {
      if (it.next()->getId() == checkId)
      {
         if (deleteCheckFromDatabase(checkId))
         {
            it.remove();
            rcc = RCC_SUCCESS;
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
         break;
      }
   }
   checksUnlock();
   return rcc;
}

/**
 * Delete business service check from database
 */
bool BaseBusinessService::deleteCheckFromDatabase(uint32_t checkId)
{
   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM business_service_checks WHERE id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, checkId);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
      NotifyClientsOnBusinessServiceCheckDelete(*this, checkId);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * Create or modify business service check from request. Returns client RCC.
 */
uint32_t BaseBusinessService::modifyCheckFromMessage(const NXCPMessage& request)
{
   uint32_t rcc = RCC_INVALID_BUSINESS_CHECK_ID;
   uint32_t checkId = request.getFieldAsUInt32(VID_CHECK_ID);
   shared_ptr<BusinessServiceCheck> check;

   checksLock();
   if (checkId != 0)
   {
      for (const shared_ptr<BusinessServiceCheck>& c : m_checks)
      {
         if (c->getId() == checkId)
         {
            check = c;
            break;
         }
      }
   }
   else
   {
      check = make_shared<BusinessServiceCheck>(m_id);
   }

   if (check != nullptr)
   {
      check->modifyFromMessage(request);
      if (check->saveToDatabase())
      {
         rcc = RCC_SUCCESS;
         if (checkId == 0) // new check was created
            m_checks.add(check);
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
   }

   checksUnlock();

   if (rcc == RCC_SUCCESS)
      NotifyClientsOnBusinessServiceCheckUpdate(*this, check);

   return rcc;
}

/**
 * Modify business service from request
 */
uint32_t BaseBusinessService::modifyFromMessageInternal(NXCPMessage *request)
{
   AutoBindTarget::modifyFromMessage(request);
   if (request->isFieldExist(VID_OBJECT_STATUS_THRESHOLD))
   {
      m_objectStatusThreshhold = request->getFieldAsUInt32(VID_OBJECT_STATUS_THRESHOLD);
   }
   if (request->isFieldExist(VID_DCI_STATUS_THRESHOLD))
   {
      m_dciStatusThreshhold = request->getFieldAsUInt32(VID_DCI_STATUS_THRESHOLD);
   }
   return super::modifyFromMessageInternal(request);
}

/**
 * Fill message with business service data
 */
void BaseBusinessService::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   AutoBindTarget::fillMessage(msg);
   msg->setField(VID_OBJECT_STATUS_THRESHOLD, m_objectStatusThreshhold);
   msg->setField(VID_DCI_STATUS_THRESHOLD, m_dciStatusThreshhold);
   return super::fillMessageInternal(msg, userId);
}

/**
 * Load Business service from database
 */
bool BaseBusinessService::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;

   if (!loadChecksFromDatabase(hdb))
      return false;

   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   return true;
}

/**
 * Save business service to database
 */
bool BaseBusinessService::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success)
      success = AutoBindTarget::saveToDatabase(hdb);

   if (success)
   {
      for (int i = 0; (i < m_checks.size()) && success; i++)
         success = m_checks.get(i)->saveToDatabase();
   }

   return success;
}
