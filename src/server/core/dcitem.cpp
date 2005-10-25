/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: dcitem.cpp
**
**/

#include "nxcore.h"


//
// Default constructor for DCItem
//

DCItem::DCItem()
{
   m_dwId = 0;
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_iBusy = 0;
   m_iDataType = DCI_DT_INT;
   m_iPollingInterval = 3600;
   m_iRetentionTime = 0;
   m_iDeltaCalculation = DCM_ORIGINAL_VALUE;
   m_iSource = DS_INTERNAL;
   m_iStatus = ITEM_STATUS_NOT_SUPPORTED;
   m_szName[0] = 0;
   m_szDescription[0] = 0;
   m_szInstance[0] = 0;
   m_tLastPoll = 0;
   m_pszFormula = _tcsdup(_T(""));
   m_pNode = NULL;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_iAdvSchedule = 0;
   m_dwNumSchedules = 0;
   m_ppScheduleList = NULL;
   m_tLastCheck = 0;
}


//
// Create DCItem from another DCItem
//

DCItem::DCItem(const DCItem *pSrc)
{
   DWORD i;

   m_dwId = pSrc->m_dwId;
   m_dwTemplateId = pSrc->m_dwTemplateId;
   m_dwTemplateItemId = pSrc->m_dwTemplateItemId;
   m_iBusy = 0;
   m_iDataType = pSrc->m_iDataType;
   m_iPollingInterval = pSrc->m_iPollingInterval;
   m_iRetentionTime = pSrc->m_iRetentionTime;
   m_iDeltaCalculation = pSrc->m_iDeltaCalculation;
   m_iSource = pSrc->m_iSource;
   m_iStatus = pSrc->m_iStatus;
   _tcscpy(m_szName, pSrc->m_szName);
   _tcscpy(m_szDescription, pSrc->m_szDescription);
   _tcscpy(m_szInstance, pSrc->m_szInstance);
   m_tLastPoll = 0;
   m_pszFormula = _tcsdup(pSrc->m_pszFormula);
   m_pNode = NULL;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_tLastCheck = 0;
   m_iAdvSchedule = pSrc->m_iAdvSchedule;

   // Copy schedules
   m_dwNumSchedules = pSrc->m_dwNumSchedules;
   m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
   for(i = 0; i < m_dwNumSchedules; i++)
      m_ppScheduleList[i] = _tcsdup(pSrc->m_ppScheduleList[i]);

   // Copy thresholds
   m_dwNumThresholds = pSrc->m_dwNumThresholds;
   m_ppThresholdList = (Threshold **)malloc(sizeof(Threshold *) * m_dwNumThresholds);
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      m_ppThresholdList[i] = new Threshold(pSrc->m_ppThresholdList[i]);
      m_ppThresholdList[i]->CreateId();
   }
}


//
// Constructor for creating DCItem from database
// Assumes that fields in SELECT query are in following order:
// item_id,name,source,datatype,polling_interval,retention_time,status,
// delta_calculation,transformation,template_id,description,instance,
// template_item_id,adv_schedule
//

DCItem::DCItem(DB_RESULT hResult, int iRow, Template *pNode)
{
   char szQuery[256];
   DB_RESULT hTempResult;
   DWORD i;

   m_dwId = DBGetFieldULong(hResult, iRow, 0);
   nx_strncpy(m_szName, DBGetField(hResult, iRow, 1), MAX_ITEM_NAME);
   DecodeSQLString(m_szName);
   m_iSource = (BYTE)DBGetFieldLong(hResult, iRow, 2);
   m_iDataType = (BYTE)DBGetFieldLong(hResult, iRow, 3);
   m_iPollingInterval = DBGetFieldLong(hResult, iRow, 4);
   m_iRetentionTime = DBGetFieldLong(hResult, iRow, 5);
   m_iStatus = (BYTE)DBGetFieldLong(hResult, iRow, 6);
   m_iDeltaCalculation = (BYTE)DBGetFieldLong(hResult, iRow, 7);
   m_pszFormula = strdup(DBGetField(hResult, iRow, 8));
   DecodeSQLString(m_pszFormula);
   m_dwTemplateId = DBGetFieldULong(hResult, iRow, 9);
   nx_strncpy(m_szDescription, DBGetField(hResult, iRow, 10), MAX_DB_STRING);
   DecodeSQLString(m_szDescription);
   nx_strncpy(m_szInstance, DBGetField(hResult, iRow, 11), MAX_DB_STRING);
   DecodeSQLString(m_szInstance);
   m_dwTemplateItemId = DBGetFieldULong(hResult, iRow, 12);
   m_iBusy = 0;
   m_tLastPoll = 0;
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_tLastCheck = 0;
   m_iAdvSchedule = (BYTE)DBGetFieldLong(hResult, iRow, 13);

   if (m_iAdvSchedule)
   {
      sprintf(szQuery, "SELECT schedule FROM dci_schedules WHERE item_id=%ld", m_dwId);
      hTempResult = DBSelect(g_hCoreDB, szQuery);
      if (hTempResult != NULL)
      {
         m_dwNumSchedules = DBGetNumRows(hTempResult);
         m_ppScheduleList = (TCHAR **)malloc(sizeof(TCHAR *) * m_dwNumSchedules);
         for(i = 0; i < m_dwNumSchedules; i++)
         {
            m_ppScheduleList[i] = _tcsdup(DBGetField(hTempResult, i, 0));
            DecodeSQLString(m_ppScheduleList[i]);
         }
         DBFreeResult(hTempResult);
      }
      else
      {
         m_dwNumSchedules = 0;
         m_ppScheduleList = NULL;
      }
   }
   else
   {
      m_dwNumSchedules = 0;
      m_ppScheduleList = NULL;
   }

   // Load last raw value from database
   sprintf(szQuery, "SELECT raw_value,last_poll_time FROM raw_dci_values WHERE item_id=%ld", m_dwId);
   hTempResult = DBSelect(g_hCoreDB, szQuery);
   if (hTempResult != NULL)
   {
      if (DBGetNumRows(hTempResult) > 0)
      {
         m_prevRawValue = DBGetField(hTempResult, 0, 0);
         m_tPrevValueTimeStamp = DBGetFieldULong(hTempResult, 0, 1);
         m_tLastPoll = m_tPrevValueTimeStamp;
      }
      DBFreeResult(hTempResult);
   }
}


//
// Constructor for creating new DCItem from scratch
//

DCItem::DCItem(DWORD dwId, char *szName, int iSource, int iDataType, 
               int iPollingInterval, int iRetentionTime, Template *pNode,
               char *pszDescription)
{
   m_dwId = dwId;
   m_dwTemplateId = 0;
   m_dwTemplateItemId = 0;
   nx_strncpy(m_szName, szName, MAX_ITEM_NAME);
   if (pszDescription != NULL)
      nx_strncpy(m_szDescription, pszDescription, MAX_DB_STRING);
   else
      strcpy(m_szDescription, m_szName);
   m_szInstance[0] = 0;
   m_iSource = iSource;
   m_iDataType = iDataType;
   m_iPollingInterval = iPollingInterval;
   m_iRetentionTime = iRetentionTime;
   m_iDeltaCalculation = DCM_ORIGINAL_VALUE;
   m_iStatus = ITEM_STATUS_ACTIVE;
   m_iBusy = 0;
   m_tLastPoll = 0;
   m_pszFormula = strdup("");
   m_dwNumThresholds = 0;
   m_ppThresholdList = NULL;
   m_pNode = pNode;
   m_hMutex = MutexCreate();
   m_dwCacheSize = 0;
   m_ppValueCache = NULL;
   m_tPrevValueTimeStamp = 0;
   m_bCacheLoaded = FALSE;
   m_iAdvSchedule = 0;
   m_dwNumSchedules = 0;
   m_ppScheduleList = NULL;
   m_tLastCheck = 0;

   UpdateCacheSize();
}


//
// Destructor for DCItem
//

DCItem::~DCItem()
{
   DWORD i;

   for(i = 0; i < m_dwNumThresholds; i++)
      delete m_ppThresholdList[i];
   safe_free(m_ppThresholdList);
   for(i = 0; i < m_dwNumSchedules; i++)
      free(m_ppScheduleList[i]);
   safe_free(m_ppScheduleList);
   safe_free(m_pszFormula);
   ClearCache();
   MutexDestroy(m_hMutex);
}


//
// Clear data cache
//

void DCItem::ClearCache(void)
{
   DWORD i;

   for(i = 0; i < m_dwCacheSize; i++)
      delete m_ppValueCache[i];
   safe_free(m_ppValueCache);
   m_ppValueCache = NULL;
   m_dwCacheSize = 0;
}


//
// Load data collection items thresholds from database
//

BOOL DCItem::LoadThresholdsFromDB(void)
{
   DWORD i;
   char szQuery[256];
   DB_RESULT hResult;
   BOOL bResult = FALSE;

   sprintf(szQuery, "SELECT threshold_id,fire_value,rearm_value,check_function,"
                    "check_operation,parameter_1,parameter_2,event_code FROM thresholds "
                    "WHERE item_id=%ld ORDER BY sequence_number", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      m_dwNumThresholds = DBGetNumRows(hResult);
      if (m_dwNumThresholds > 0)
      {
         m_ppThresholdList = (Threshold **)malloc(sizeof(Threshold *) * m_dwNumThresholds);
         for(i = 0; i < m_dwNumThresholds; i++)
            m_ppThresholdList[i] = new Threshold(hResult, i, this);
      }
      DBFreeResult(hResult);
      bResult = TRUE;
   }

   //UpdateCacheSize();

   return bResult;
}


//
// Save to database
//

BOOL DCItem::SaveToDB(DB_HANDLE hdb)
{
   TCHAR *pszEscName, *pszEscFormula, *pszEscDescr, *pszEscInstance, szQuery[2048];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE, bResult;

   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT item_id FROM items WHERE item_id=%ld", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Prepare and execute query
   pszEscName = EncodeSQLString(m_szName);
   pszEscFormula = EncodeSQLString(m_pszFormula);
   pszEscDescr = EncodeSQLString(m_szDescription);
   pszEscInstance = EncodeSQLString(m_szInstance);
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO items (item_id,node_id,template_id,name,description,source,"
                       "datatype,polling_interval,retention_time,status,delta_calculation,"
                       "transformation,instance,template_item_id,adv_schedule)"
                       " VALUES (%ld,%ld,%ld,'%s','%s',%d,%d,%ld,%ld,%d,"
                       "%d,'%s','%s',%ld,%d)",
                       m_dwId, (m_pNode == NULL) ? 0 : m_pNode->Id(), m_dwTemplateId,
                       pszEscName, pszEscDescr, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus, m_iDeltaCalculation,
                       pszEscFormula, pszEscInstance, m_dwTemplateItemId, m_iAdvSchedule);
   else
      sprintf(szQuery, "UPDATE items SET node_id=%ld,template_id=%ld,name='%s',source=%d,"
                       "datatype=%d,polling_interval=%ld,retention_time=%ld,status=%d,"
                       "delta_calculation=%d,transformation='%s',description='%s',"
                       "instance='%s',template_item_id=%ld,adv_schedule=%d WHERE item_id=%ld",
                       (m_pNode == NULL) ? 0 : m_pNode->Id(), m_dwTemplateId,
                       pszEscName, m_iSource, m_iDataType, m_iPollingInterval,
                       m_iRetentionTime, m_iStatus, m_iDeltaCalculation, pszEscFormula,
                       pszEscDescr, pszEscInstance, m_dwTemplateItemId,
                       m_iAdvSchedule, m_dwId);
   bResult = DBQuery(hdb, szQuery);
   free(pszEscName);
   free(pszEscFormula);
   free(pszEscDescr);
   free(pszEscInstance);

   // Save thresholds
   if (bResult)
   {
      DWORD i;

      for(i = 0; i < m_dwNumThresholds; i++)
         m_ppThresholdList[i]->SaveToDB(hdb, i);
   }

   // Delete non-existing thresholds
   sprintf(szQuery, "SELECT threshold_id FROM thresholds WHERE item_id=%ld", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      int i, iNumRows;
      DWORD j, dwId;

      iNumRows = DBGetNumRows(hResult);
      for(i = 0; i < iNumRows; i++)
      {
         dwId = DBGetFieldULong(hResult, i, 0);
         for(j = 0; j < m_dwNumThresholds; j++)
            if (m_ppThresholdList[j]->Id() == dwId)
               break;
         if (j == m_dwNumThresholds)
         {
            sprintf(szQuery, "DELETE FROM thresholds WHERE threshold_id=%ld", dwId);
            DBQuery(hdb, szQuery);
         }
      }
      DBFreeResult(hResult);
   }

   // Create record in raw_dci_values if needed
   sprintf(szQuery, "SELECT item_id FROM raw_dci_values WHERE item_id=%ld", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) == 0)
      {
         char *pszEscValue;

         pszEscValue = EncodeSQLString(m_prevRawValue.String());
         sprintf(szQuery, "INSERT INTO raw_dci_values (item_id,raw_value,last_poll_time)"
                          " VALUES (%ld,'%s',%ld)",
                 m_dwId, pszEscValue, m_tPrevValueTimeStamp);
         free(pszEscValue);
         DBQuery(hdb, szQuery);
      }
      DBFreeResult(hResult);
   }

   // Save schedules
   sprintf(szQuery, "DELETE FROM dci_schedules WHERE item_id=%ld", m_dwId);
   DBQuery(hdb, szQuery);
   if (m_iAdvSchedule)
   {
      TCHAR *pszEscSchedule;
      DWORD i;

      for(i = 0; i < m_dwNumSchedules; i++)
      {
         pszEscSchedule = EncodeSQLString(m_ppScheduleList[i]);
         sprintf(szQuery, "INSERT INTO dci_schedules (item_id,schedule) VALUES (%ld,'%s')",
                 m_dwId, pszEscSchedule);
         free(pszEscSchedule);
         DBQuery(hdb, szQuery);
      }
   }

   Unlock();
   return bResult;
}


//
// Check last value for threshold breaches
//

void DCItem::CheckThresholds(ItemValue &value)
{
   DWORD i, iResult;
   ItemValue checkValue;

   for(i = 0; i < m_dwNumThresholds; i++)
   {
      iResult = m_ppThresholdList[i]->Check(value, m_ppValueCache, checkValue);
      switch(iResult)
      {
         case THRESHOLD_REACHED:
            PostEvent(m_ppThresholdList[i]->EventCode(), m_pNode->Id(), "ssssis", m_szName,
                      m_szDescription, m_ppThresholdList[i]->StringValue(), 
                      (const char *)checkValue, m_dwId, m_szInstance);
            i = m_dwNumThresholds;  // Stop processing
            break;
         case THRESHOLD_REARMED:
            PostEvent(EVENT_THRESHOLD_REARMED, m_pNode->Id(), "ssi", m_szName, 
                      m_szDescription, m_dwId);
            break;
         case NO_ACTION:
            if (m_ppThresholdList[i]->IsReached())
               i = m_dwNumThresholds;  // Threshold condition still true, stop processing
            break;
      }
   }
}


//
// Create CSCP message with item data
//

void DCItem::CreateMessage(CSCPMessage *pMsg)
{
   DCI_THRESHOLD dct;
   DWORD i, dwId;

   Lock();
   pMsg->SetVariable(VID_DCI_ID, m_dwId);
   pMsg->SetVariable(VID_TEMPLATE_ID, m_dwTemplateId);
   pMsg->SetVariable(VID_NAME, m_szName);
   pMsg->SetVariable(VID_DESCRIPTION, m_szDescription);
   pMsg->SetVariable(VID_INSTANCE, m_szInstance);
   pMsg->SetVariable(VID_POLLING_INTERVAL, (DWORD)m_iPollingInterval);
   pMsg->SetVariable(VID_RETENTION_TIME, (DWORD)m_iRetentionTime);
   pMsg->SetVariable(VID_DCI_SOURCE_TYPE, (WORD)m_iSource);
   pMsg->SetVariable(VID_DCI_DATA_TYPE, (WORD)m_iDataType);
   pMsg->SetVariable(VID_DCI_STATUS, (WORD)m_iStatus);
   pMsg->SetVariable(VID_DCI_DELTA_CALCULATION, (WORD)m_iDeltaCalculation);
   pMsg->SetVariable(VID_DCI_FORMULA, m_pszFormula);
   pMsg->SetVariable(VID_NUM_THRESHOLDS, m_dwNumThresholds);
   for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < m_dwNumThresholds; i++, dwId++)
   {
      m_ppThresholdList[i]->CreateMessage(&dct);
      pMsg->SetVariable(dwId, (BYTE *)&dct, sizeof(DCI_THRESHOLD));
   }
   pMsg->SetVariable(VID_ADV_SCHEDULE, (WORD)m_iAdvSchedule);
   if (m_iAdvSchedule)
   {
      pMsg->SetVariable(VID_NUM_SCHEDULES, m_dwNumSchedules);
      for(i = 0, dwId = VID_DCI_SCHEDULE_BASE; i < m_dwNumSchedules; i++, dwId++)
         pMsg->SetVariable(dwId, m_ppScheduleList[i]);
   }
   Unlock();
}


//
// Delete item and collected data from database
//

void DCItem::DeleteFromDB(void)
{
   char szQuery[256];

   sprintf(szQuery, "DELETE FROM items WHERE item_id=%d", m_dwId);
   QueueSQLRequest(szQuery);
   sprintf(szQuery, "DELETE FROM idata_%d WHERE item_id=%d", m_pNode->Id(), m_dwId);
   QueueSQLRequest(szQuery);
   sprintf(szQuery, "DELETE FROM thresholds WHERE item_id=%d", m_dwId);
   QueueSQLRequest(szQuery);
}


//
// Update item from CSCP message
//

void DCItem::UpdateFromMessage(CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                               DWORD **ppdwMapIndex, DWORD **ppdwMapId)
{
   DWORD i, j, dwNum, dwId;
   DCI_THRESHOLD *pNewThresholds;
   Threshold **ppNewList;
   TCHAR *pszStr;

   Lock();

   pMsg->GetVariableStr(VID_NAME, m_szName, MAX_ITEM_NAME);
   pMsg->GetVariableStr(VID_DESCRIPTION, m_szDescription, MAX_DB_STRING);
   pMsg->GetVariableStr(VID_INSTANCE, m_szInstance, MAX_DB_STRING);
   m_iSource = (BYTE)pMsg->GetVariableShort(VID_DCI_SOURCE_TYPE);
   m_iDataType = (BYTE)pMsg->GetVariableShort(VID_DCI_DATA_TYPE);
   m_iPollingInterval = pMsg->GetVariableLong(VID_POLLING_INTERVAL);
   m_iRetentionTime = pMsg->GetVariableLong(VID_RETENTION_TIME);
   m_iStatus = (BYTE)pMsg->GetVariableShort(VID_DCI_STATUS);
   m_iDeltaCalculation = (BYTE)pMsg->GetVariableShort(VID_DCI_DELTA_CALCULATION);
   safe_free(m_pszFormula);
   m_pszFormula = pMsg->GetVariableStr(VID_DCI_FORMULA);

   // Update schedules
   for(i = 0; i < m_dwNumSchedules; i++)
      free(m_ppScheduleList[i]);
   m_iAdvSchedule = (BYTE)pMsg->GetVariableShort(VID_ADV_SCHEDULE);
   if (m_iAdvSchedule)
   {
      m_dwNumSchedules = pMsg->GetVariableLong(VID_NUM_SCHEDULES);
      m_ppScheduleList = (TCHAR **)realloc(m_ppScheduleList, sizeof(TCHAR *) * m_dwNumSchedules);
      for(i = 0, dwId = VID_DCI_SCHEDULE_BASE; i < m_dwNumSchedules; i++, dwId++)
      {
         pszStr = pMsg->GetVariableStr(dwId);
         if (pszStr != NULL)
         {
            m_ppScheduleList[i] = pszStr;
         }
         else
         {
            m_ppScheduleList[i] = _tcsdup(_T("(null)"));
         }
      }
   }
   else
   {
      if (m_ppScheduleList != NULL)
      {
         free(m_ppScheduleList);
         m_ppScheduleList = NULL;
      }
      m_dwNumSchedules = 0;
   }

   // Update thresholds
   dwNum = pMsg->GetVariableLong(VID_NUM_THRESHOLDS);
   pNewThresholds = (DCI_THRESHOLD *)malloc(sizeof(DCI_THRESHOLD) * dwNum);
   *ppdwMapIndex = (DWORD *)malloc(dwNum * sizeof(DWORD));
   *ppdwMapId = (DWORD *)malloc(dwNum * sizeof(DWORD));
   *pdwNumMaps = 0;

   // Read all thresholds from message
   for(i = 0, dwId = VID_DCI_THRESHOLD_BASE; i < dwNum; i++, dwId++)
   {
      pMsg->GetVariableBinary(dwId, (BYTE *)&pNewThresholds[i], sizeof(DCI_THRESHOLD));
      pNewThresholds[i].dwId = ntohl(pNewThresholds[i].dwId);
   }
   
   // Check if some thresholds was deleted, and reposition others if needed
   ppNewList = (Threshold **)malloc(sizeof(Threshold *) * dwNum);
   for(i = 0; i < m_dwNumThresholds; i++)
   {
      for(j = 0; j < dwNum; j++)
         if (m_ppThresholdList[i]->Id() == pNewThresholds[j].dwId)
            break;
      if (j == dwNum)
      {
         // No threshold with that id in new list, delete it
         delete m_ppThresholdList[i];
         m_dwNumThresholds--;
         memmove(&m_ppThresholdList[i], &m_ppThresholdList[i + 1], sizeof(Threshold *) * (m_dwNumThresholds - i));
         i--;
      }
      else
      {
         // Move existing thresholds to appropriate positions in new list
         ppNewList[j] = m_ppThresholdList[i];
      }
   }
   safe_free(m_ppThresholdList);
   m_ppThresholdList = ppNewList;
   m_dwNumThresholds = dwNum;

   // Add or update thresholds
   for(i = 0; i < dwNum; i++)
   {
      if (pNewThresholds[i].dwId == 0)    // New threshold?
      {
         m_ppThresholdList[i] = new Threshold(this);
         m_ppThresholdList[i]->CreateId();

         // Add index -> id mapping
         (*ppdwMapIndex)[*pdwNumMaps] = i;
         (*ppdwMapId)[*pdwNumMaps] = m_ppThresholdList[i]->Id();
         (*pdwNumMaps)++;
      }
      m_ppThresholdList[i]->UpdateFromMessage(&pNewThresholds[i]);
   }
      
   safe_free(pNewThresholds);
   UpdateCacheSize();
   Unlock();
}


//
// Process new value
//

void DCItem::NewValue(DWORD dwTimeStamp, const char *pszOriginalValue)
{
   char *pszEscValue, szQuery[MAX_LINE_SIZE + 128];
   ItemValue rawValue, *pValue;

   Lock();

   // Normally m_pNode shouldn't be NULL for polled items,
   // but who knows...
   if (m_pNode == NULL)
   {
      Unlock();
      return;
   }

   // Save raw value into database
   pszEscValue = EncodeSQLString(pszOriginalValue);
   sprintf(szQuery, "UPDATE raw_dci_values SET raw_value='%s',last_poll_time=%ld WHERE item_id=%ld",
           pszEscValue, dwTimeStamp, m_dwId);
   free(pszEscValue);
   QueueSQLRequest(szQuery);

   // Create new ItemValue object and transform it as needed
   pValue = new ItemValue(pszOriginalValue, dwTimeStamp);
   if (m_tPrevValueTimeStamp == 0)
      m_prevRawValue = *pValue;  // Delta should be zero for first poll
   rawValue = *pValue;
   Transform(*pValue, (long)(dwTimeStamp - m_tPrevValueTimeStamp));
   m_prevRawValue = rawValue;
   m_tPrevValueTimeStamp = dwTimeStamp;

   // Save transformed value to database
   pszEscValue = EncodeSQLString(pValue->String());
   sprintf(szQuery, "INSERT INTO idata_%ld (item_id,idata_timestamp,idata_value)"
                    " VALUES (%ld,%ld,'%s')", m_pNode->Id(), m_dwId, dwTimeStamp, pszEscValue);
   free(pszEscValue);
   QueueSQLRequest(szQuery);

   // Check thresholds and add value to cache
   CheckThresholds(*pValue);

   if (m_dwCacheSize > 0)
   {
      delete m_ppValueCache[m_dwCacheSize - 1];
      memmove(&m_ppValueCache[1], m_ppValueCache, sizeof(ItemValue *) * (m_dwCacheSize - 1));
      m_ppValueCache[0] = pValue;
   }
   else
   {
      delete pValue;
   }

   Unlock();
}


//
// Transform received value
//

void DCItem::Transform(ItemValue &value, long nElapsedTime)
{
   switch(m_iDeltaCalculation)
   {
      case DCM_SIMPLE:
         switch(m_iDataType)
         {
            case DCI_DT_INT:
               value = (long)value - (long)m_prevRawValue;
               break;
            case DCI_DT_UINT:
               value = (DWORD)value - (DWORD)m_prevRawValue;
               break;
            case DCI_DT_INT64:
               value = (INT64)value - (INT64)m_prevRawValue;
               break;
            case DCI_DT_UINT64:
               value = (QWORD)value - (QWORD)m_prevRawValue;
               break;
            case DCI_DT_FLOAT:
               value = (double)value - (double)m_prevRawValue;
               break;
            case DCI_DT_STRING:
               value = (long)((_tcscmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      case DCM_AVERAGE_PER_MINUTE:
         nElapsedTime /= 60;  // Convert to minutes
      case DCM_AVERAGE_PER_SECOND:
         // Check elapsed time to prevent divide-by-zero exception
         if (nElapsedTime == 0)
            nElapsedTime++;

         switch(m_iDataType)
         {
            case DCI_DT_INT:
               value = ((long)value - (long)m_prevRawValue) / nElapsedTime;
               break;
            case DCI_DT_UINT:
               value = ((DWORD)value - (DWORD)m_prevRawValue) / (DWORD)nElapsedTime;
               break;
            case DCI_DT_INT64:
               value = ((INT64)value - (INT64)m_prevRawValue) / (INT64)nElapsedTime;
               break;
            case DCI_DT_UINT64:
               value = ((QWORD)value - (QWORD)m_prevRawValue) / (QWORD)nElapsedTime;
               break;
            case DCI_DT_FLOAT:
               value = ((double)value - (double)m_prevRawValue) / (double)nElapsedTime;
               break;
            case DCI_DT_STRING:
               // I don't see any meaning in "average delta per second (minute)" for string
               // values, so result will be 0 if there are no difference between
               // current and previous values, and 1 otherwise
               value = (long)((strcmp((const TCHAR *)value, (const TCHAR *)m_prevRawValue) == 0) ? 0 : 1);
               break;
            default:
               // Delta calculation is not supported for other types
               break;
         }
         break;
      default:    // Default is no transformation
         break;
   }
}


//
// Set new ID
//

void DCItem::ChangeBinding(DWORD dwNewId, Template *pNewNode)
{
   DWORD i;

   Lock();
   m_pNode = pNewNode;
   m_dwId = dwNewId;
   for(i = 0; i < m_dwNumThresholds; i++)
      m_ppThresholdList[i]->BindToItem(m_dwId);
   ClearCache();
   UpdateCacheSize();
   Unlock();
}


//
// Update required cache size depending on thresholds
//

void DCItem::UpdateCacheSize(void)
{
   DWORD i, dwRequiredSize;

   // Minimum cache size is 1 for nodes (so GetLastValue can work)
   // and 0 for templates
   if (m_pNode != NULL)
   {
      dwRequiredSize = (m_pNode->Type() == OBJECT_NODE) ? 1 : 0;
   }
   else
   {
      dwRequiredSize = 0;
   }

   // Calculate required cache size
   for(i = 0; i < m_dwNumThresholds; i++)
      if (dwRequiredSize < m_ppThresholdList[i]->RequiredCacheSize())
         dwRequiredSize = m_ppThresholdList[i]->RequiredCacheSize();

   // Update cache if needed
   if (dwRequiredSize < m_dwCacheSize)
   {
      // Destroy unneeded values
      if (m_dwCacheSize > 0)
         for(i = m_dwCacheSize - 1; i >= dwRequiredSize; i--)
            delete m_ppValueCache[i];

      m_dwCacheSize = dwRequiredSize;
      if (m_dwCacheSize > 0)
      {
         m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * m_dwCacheSize);
      }
      else
      {
         safe_free(m_ppValueCache);
         m_ppValueCache = NULL;
      }
   }
   else if (dwRequiredSize > m_dwCacheSize)
   {
      // Expand cache
      m_ppValueCache = (ItemValue **)realloc(m_ppValueCache, sizeof(ItemValue *) * dwRequiredSize);
      for(i = m_dwCacheSize; i < dwRequiredSize; i++)
         m_ppValueCache[i] = NULL;

      // Load missing values from database
      if (m_pNode != NULL)
      {
         DB_ASYNC_RESULT hResult;
         char szBuffer[MAX_DB_STRING];
         BOOL bHasData;

         switch(g_dwDBSyntax)
         {
            case DB_SYNTAX_MSSQL:
               sprintf(szBuffer, "SELECT TOP %ld idata_value,idata_timestamp FROM idata_%ld "
                                 "WHERE item_id=%ld ORDER BY idata_timestamp DESC",
                       dwRequiredSize, m_pNode->Id(), m_dwId);
               break;
            case DB_SYNTAX_ORACLE:
               sprintf(szBuffer, "SELECT idata_value,idata_timestamp FROM idata_%ld "
                                 "WHERE item_id=%ld AND ROWNUM <= %ld ORDER BY idata_timestamp DESC",
                       m_pNode->Id(), m_dwId, dwRequiredSize);
               break;
            case DB_SYNTAX_MYSQL:
            case DB_SYNTAX_PGSQL:
               sprintf(szBuffer, "SELECT idata_value,idata_timestamp FROM idata_%ld "
                                 "WHERE item_id=%ld ORDER BY idata_timestamp DESC LIMIT %ld",
                       m_pNode->Id(), m_dwId, dwRequiredSize);
               break;
            default:
               sprintf(szBuffer, "SELECT idata_value,idata_timestamp FROM idata_%ld "
                                 "WHERE item_id=%ld ORDER BY idata_timestamp DESC",
                       m_pNode->Id(), m_dwId);
               break;
         }
         hResult = DBAsyncSelect(g_hCoreDB, szBuffer);
         if (hResult != NULL)
         {
            // Skip already cached values
            for(i = 0, bHasData = TRUE; i < m_dwCacheSize; i++)
               bHasData = DBFetch(hResult);

            // Create new cache entries
            for(; (i < dwRequiredSize) && bHasData; i++)
            {
               bHasData = DBFetch(hResult);
               if (bHasData)
               {
                  DBGetFieldAsync(hResult, 0, szBuffer, MAX_DB_STRING);
                  DecodeSQLString(szBuffer);
                  m_ppValueCache[i] = 
                     new ItemValue(szBuffer, DBGetFieldAsyncULong(hResult, 1));
               }
               else
               {
                  m_ppValueCache[i] = new ItemValue(_T(""), 1);   // Empty value
               }
            }

            // Fill up cache with empty values if we don't have enough values in database
            for(; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);

            DBFreeAsyncResult(hResult);
         }
         else
         {
            // Error reading data from database, fill cache with empty values
            for(i = m_dwCacheSize; i < dwRequiredSize; i++)
               m_ppValueCache[i] = new ItemValue(_T(""), 1);
         }
      }
      m_dwCacheSize = dwRequiredSize;
   }
   m_bCacheLoaded = TRUE;
}


//
// Put last value into CSCP message
//

void DCItem::GetLastValue(CSCPMessage *pMsg, DWORD dwId)
{
   pMsg->SetVariable(dwId++, m_dwId);
   pMsg->SetVariable(dwId++, m_szName);
   pMsg->SetVariable(dwId++, m_szDescription);
   pMsg->SetVariable(dwId++, (WORD)m_iSource);
   if (m_dwCacheSize > 0)
   {
      pMsg->SetVariable(dwId++, (WORD)m_iDataType);
      pMsg->SetVariable(dwId++, (TCHAR *)m_ppValueCache[0]->String());
      pMsg->SetVariable(dwId++, m_ppValueCache[0]->GetTimeStamp());
   }
   else
   {
      pMsg->SetVariable(dwId++, (WORD)DCI_DT_NULL);
      pMsg->SetVariable(dwId++, _T(""));
      pMsg->SetVariable(dwId++, (DWORD)0);
   }
}


//
// Clean expired data
//

void DCItem::CleanData(void)
{
   TCHAR szQuery[256];
   time_t now;

   now = time(NULL);
   Lock();
   _sntprintf(szQuery, 256, _T("DELETE FROM idata_%ld WHERE (item_id=%ld) AND (idata_timestamp<%ld)"),
              m_pNode->Id(), m_dwId, now - (DWORD)m_iRetentionTime * 86400);
   Unlock();
   QueueSQLRequest(szQuery);
}


//
// Prepare item for deletion
//

void DCItem::PrepareForDeletion(void)
{
   Lock();

   m_iStatus = ITEM_STATUS_DISABLED;   // Prevent future polls

   // Wait until current poll ends, if any
   // while() is not a very good solution, and probably need to be
   // rewrited using conditions
   while(m_iBusy)
   {
      Unlock();
      ThreadSleepMs(100);
      Lock();
   }

   Unlock();
}


//
// Prepare item for replacing an old one
//

void DCItem::PrepareForReplacement(DCItem *pItem, int iStatus)
{
   m_tLastPoll = pItem->m_tLastPoll;
   m_dwId = pItem->m_dwId;
   ClearCache();
   m_dwCacheSize = pItem->m_dwCacheSize;
   m_ppValueCache = pItem->m_ppValueCache;
   pItem->m_ppValueCache = NULL;
   pItem->m_dwCacheSize = 0;
   m_prevRawValue = pItem->m_prevRawValue;
   m_tPrevValueTimeStamp = pItem->m_tPrevValueTimeStamp;
   m_iStatus = iStatus;
}


//
// Match schedule to current time
//

static BOOL MatchSchedule(struct tm *pCurrTime, TCHAR *pszSchedule)
{
   TCHAR *pszCurr, szValue[256];

   // Minute
   pszCurr = ExtractWord(pszSchedule, szValue);
   if (szValue[0] != '*')
   {
      if (atoi(szValue) != pCurrTime->tm_min)
         return FALSE;
   }

   // Hour
   pszCurr = ExtractWord(pszCurr, szValue);
   if (szValue[0] != '*')
   {
      if (atoi(szValue) != pCurrTime->tm_hour)
         return FALSE;
   }

   // Day of month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (szValue[0] != '*')
   {
      if (atoi(szValue) != pCurrTime->tm_mday)
         return FALSE;
   }

   // Month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (szValue[0] != '*')
   {
      if (atoi(szValue) != pCurrTime->tm_mon + 1)
         return FALSE;
   }

   // Day of week
   pszCurr = ExtractWord(pszCurr, szValue);
   if (szValue[0] != '*')
   {
      int nValue;

      nValue = atoi(szValue);
      if (nValue == 7)
         nValue = 0;
      if (nValue != pCurrTime->tm_wday)
         return FALSE;
   }

   return TRUE;
}


//
// Check if DCI have to be polled
//

BOOL DCItem::ReadyForPolling(time_t currTime)
{
   BOOL bResult;

   Lock();
   if ((m_iStatus == ITEM_STATUS_ACTIVE) && (!m_iBusy) && m_bCacheLoaded)
   {
      if (m_iAdvSchedule)
      {
         DWORD i;
         struct tm tmCurrLocal, tmLastLocal;

         memcpy(&tmCurrLocal, localtime(&currTime), sizeof(struct tm));
         memcpy(&tmLastLocal, localtime(&m_tLastCheck), sizeof(struct tm));
         for(i = 0, bResult = FALSE; i < m_dwNumSchedules; i++)
         {
            if (MatchSchedule(&tmCurrLocal, m_ppScheduleList[i]))
            {
               if (!MatchSchedule(&tmLastLocal, m_ppScheduleList[i]))
               {
                  bResult = TRUE;
                  break;
               }
            }
         }
         m_tLastCheck = currTime;
      }
      else
      {
         bResult = (m_tLastPoll + m_iPollingInterval <= currTime);
      }
   }
   else
   {
      bResult = FALSE;
   }
   Unlock();
   return bResult;
}
