/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2021 Victor Kirhenshtein
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
** File: nxdbmgr.h
**
**/

#ifndef _nxdbmgr_h_
#define _nxdbmgr_h_

#include <nms_common.h>
#include <nms_util.h>
#include <uuid.h>
#include <nxsrvapi.h>
#include <nxdbapi.h>
#include <netxmsdb.h>
#include <nxdbmgr_tools.h>

/**
 * Column identifier (table.column)
 */
struct COLUMN_IDENTIFIER
{
   const TCHAR *table;
   const char *column;
};

//
// Functions
//

DB_HANDLE ConnectToDatabase();
void CheckDatabase();
bool CreateDatabase(const char *driver, const TCHAR *dbName, const TCHAR *dbLogin, const TCHAR *dbPassword);
void InitDatabase(const char *initFile);
bool ClearDatabase(bool preMigration);
void ExportDatabase(char *file, const StringList& excludedTables, const StringList& includedTables);
void ImportDatabase(const char *file, const StringList& excludedTables, const StringList& includedTables);
void MigrateDatabase(const TCHAR *sourceConfig, TCHAR *destConfFields, const StringList& excludedTables, const StringList& includedTables);
void UpgradeDatabase();
void UnlockDatabase();
void ReindexIData();

bool ExecSQLBatch(const char *pszFile, bool showOutput);
bool ValidateDatabase();

bool SetMajorSchemaVersion(INT32 nextMajor, INT32 nextMinor);
bool SetMinorSchemaVersion(INT32 nextMinor);
INT32 GetSchemaLevelForMajorVersion(INT32 major);
bool SetSchemaLevelForMajorVersion(INT32 major, INT32 level);

void RegisterOnlineUpgrade(int major, int minor);
void UnregisterOnlineUpgrade(int major, int minor);
bool IsOnlineUpgradePending();
void RunPendingOnlineUpgrades();

IntegerArray<uint32_t> *GetDataCollectionTargets();
bool IsDataTableExist(const TCHAR *format, UINT32 id);

BOOL CreateIDataTable(DWORD nodeId);
BOOL CreateTDataTable(DWORD nodeId);
BOOL CreateTDataTable_preV281(DWORD nodeId);

void ResetSystemAccount();

bool LoadServerModules(TCHAR *moduleLoadList);
bool EnumerateModuleTables(bool (*handler)(const TCHAR *, void *), void *userData);
bool EnumerateModuleSchemas(bool (*handler)(const TCHAR *, void *), void *userData);
bool UpgradeModuleSchemas();
bool CheckModuleSchemas();

bool IsColumnIntegerFixNeeded(const TCHAR *table, const char *name);
bool IsTimestampColumn(const TCHAR *table, const char *name);

bool LoadDataCollectionObjects();
const TCHAR *GetDCObjectStorageClass(uint32_t id);

/**
 * Global variables
 */
extern bool g_isGuiMode;
extern bool g_checkData;
extern bool g_checkDataTablesOnly;
extern bool g_dataOnlyMigration;
extern bool g_skipDataMigration;
extern bool g_skipDataSchemaMigration;
extern int g_migrationTxnSize;

#endif
