/* 
** NetXMS - Network Management System
** Copyright (C) 2007-2010 Victor Kirhenshtein
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
** File: cert.cpp
**
**/

#include "nxcore.h"
#include <nxcrypto.h>

#ifdef _WITH_ENCRYPTION

// WARNING! this hack works only for d2i_X509(); be careful when adding new code
#ifdef OPENSSL_CONST
# undef OPENSSL_CONST
#endif
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
# define OPENSSL_CONST const
#else
# define OPENSSL_CONST
#endif

/**
 * Server certificate file information
 */
TCHAR g_serverCertificatePath[MAX_PATH] = _T("");
char g_serverCertificatePassword[MAX_PASSWORD] = "";

/**
 * Server certificate
 */
static X509 *s_serverCertificate = NULL;
static EVP_PKEY *s_serverCertificateKey = NULL;

/**
 * Trusted CA certificate store
 */
static X509_STORE *s_trustedCertificateStore = NULL;
static Mutex s_certificateStoreLock;

/**
 * Get CN from certificate
 */
bool GetCertificateCN(X509 *cert, TCHAR *buffer, size_t size)
{
   X509_NAME *subject = X509_get_subject_name(cert);
   if (subject == NULL)
      return false;

   int idx = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
   if (idx == -1)
      return false;

   X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, idx);
   if (entry == NULL)
      return false;

   ASN1_STRING *data = X509_NAME_ENTRY_get_data(entry);
   if (data == NULL)
      return false;

   unsigned char *utf8CertCN;
   ASN1_STRING_to_UTF8(&utf8CertCN, data);
#ifdef UNICODE
   MultiByteToWideChar(CP_UTF8, 0, (char *)utf8CertCN, -1, buffer, (int)size);
#else
   utf8_to_mb((char *)utf8CertCN, -1, buffer, (int)size);
#endif
   buffer[size - 1] = 0;
   OPENSSL_free(utf8CertCN);
   return true;
}

/**
 * Create X509 certificate structure from login message
 */
X509 *CertificateFromLoginMessage(NXCPMessage *pMsg)
{
	UINT32 dwLen;
	BYTE *pData;
	OPENSSL_CONST BYTE *p;
	X509 *pCert = NULL;

	dwLen = pMsg->getFieldAsBinary(VID_CERTIFICATE, NULL, 0);
	if (dwLen > 0)
	{
		pData = (BYTE *)malloc(dwLen);
		pMsg->getFieldAsBinary(VID_CERTIFICATE, pData, dwLen);
		p = pData;
		pCert = d2i_X509(NULL, &p, dwLen);
		free(pData);
	}
	return pCert;
}

/**
 * Check public key
 */
static BOOL CheckPublicKey(EVP_PKEY *key, const TCHAR *mappingData)
{
	int pkeyLen;
	unsigned char *ucBuf, *uctempBuf;
	TCHAR *pkeyText;
	BOOL valid;
	
	pkeyLen = i2d_PublicKey(key, NULL);
	ucBuf = (unsigned char *)malloc(pkeyLen +1);
	uctempBuf = ucBuf;
	i2d_PublicKey(key, &uctempBuf);
	
	pkeyText = (TCHAR *)malloc((pkeyLen * 2 + 1) * sizeof(TCHAR));
	BinToStr(ucBuf, pkeyLen, pkeyText);

	valid = !_tcscmp(pkeyText, mappingData);

	free(ucBuf);
	free(pkeyText);

	return valid;
}

/**
 * Check ciertificate's CN
 */
static BOOL CheckCommonName(X509 *cert, const TCHAR *cn)
{
   TCHAR certCn[256];
   if (!GetCertificateCN(cert, certCn, 256))
      return FALSE;

   nxlog_debug(3, _T("Certificate CN=\"%s\", user CN=\"%s\""), certCn, cn);
   return !_tcsicmp(certCn, cn);
}

/**
 * Validate user's certificate
 */
BOOL ValidateUserCertificate(X509 *pCert, const TCHAR *pszLogin, BYTE *pChallenge, BYTE *pSignature,
									  UINT32 dwSigLen, int nMappingMethod, const TCHAR *pszMappingData)
{
	EVP_PKEY *pKey;
	BYTE hash[SHA1_DIGEST_SIZE];
	BOOL bValid = FALSE;

#ifdef UNICODE
	WCHAR *certSubject = WideStringFromMBString(CHECK_NULL_A(pCert->name));
#else
#define certSubject (CHECK_NULL(pCert->name))
#endif

	DbgPrintf(3, _T("Validating certificate \"%s\" for user %s"), certSubject, pszLogin);
	s_certificateStoreLock.lock();

	if (s_trustedCertificateStore == NULL)
	{
		DbgPrintf(3, _T("Cannot validate user certificate because certificate store is not initialized"));
		s_certificateStoreLock.unlock();
#ifdef UNICODE
		free(certSubject);
#endif
		return FALSE;
	}

	// Validate signature
	pKey = X509_get_pubkey(pCert);
	if (pKey != NULL)
	{
		CalculateSHA1Hash(pChallenge, CLIENT_CHALLENGE_SIZE, hash);
		switch(pKey->type)
		{
			case EVP_PKEY_RSA:
				bValid = RSA_verify(NID_sha1, hash, SHA1_DIGEST_SIZE, pSignature, dwSigLen, pKey->pkey.rsa);
				break;
			default:
				DbgPrintf(3, _T("Unknown key type %d in certificate \"%s\" for user %s"), pKey->type, certSubject, pszLogin);
				break;
		}
	}

	// Validate certificate
	if (bValid)
	{
		X509_STORE_CTX *pStore = X509_STORE_CTX_new();
		if (pStore != NULL)
		{
			X509_STORE_CTX_init(pStore, s_trustedCertificateStore, pCert, NULL);
			bValid = X509_verify_cert(pStore);
			X509_STORE_CTX_free(pStore);
			DbgPrintf(3, _T("Certificate \"%s\" for user %s - validation %s"),
			          certSubject, pszLogin, bValid ? _T("successful") : _T("failed"));
		}
		else
		{
			TCHAR szBuffer[256];

			DbgPrintf(3, _T("X509_STORE_CTX_new() failed: %s"), _ERR_error_tstring(ERR_get_error(), szBuffer));
			bValid = FALSE;
		}
	}

	// Check user mapping
	if (bValid)
	{
		switch(nMappingMethod)
		{
			case USER_MAP_CERT_BY_SUBJECT:
				bValid = !_tcsicmp(certSubject, CHECK_NULL_EX(pszMappingData));
				break;
			case USER_MAP_CERT_BY_PUBKEY:
				bValid = CheckPublicKey(pKey, CHECK_NULL_EX(pszMappingData));
				break;
			case USER_MAP_CERT_BY_CN:
            bValid = CheckCommonName(pCert, ((pszMappingData != NULL) && (*pszMappingData != 0)) ? pszMappingData : pszLogin);
				break;
			default:
				DbgPrintf(3, _T("Invalid certificate mapping method %d for user %s"), nMappingMethod, pszLogin);
				bValid = FALSE;
				break;
		}
	}

	s_certificateStoreLock.unlock();

#ifdef UNICODE
	free(certSubject);
#endif

	return bValid;
#undef certSubject
}

/**
 * Validate agent certificate
 */
bool ValidateAgentCertificate(X509 *cert)
{
   X509_STORE *store = X509_STORE_new();
   if (store == NULL)
   {
      nxlog_debug(3, _T("ValidateAgentCertificate: cannot create certificate store"));
   }

   X509_STORE_add_cert(store, s_serverCertificate);
   bool valid = false;

   X509_STORE_CTX *ctx = X509_STORE_CTX_new();
   if (ctx != NULL)
   {
      X509_STORE_CTX_init(ctx, store, cert, NULL);
      valid = (X509_verify_cert(ctx) == 1);
      X509_STORE_CTX_free(ctx);
   }
   else
   {
      TCHAR buffer[256];
      nxlog_debug(3, _T("ValidateAgentCertificate: X509_STORE_CTX_new() failed: %s"), _ERR_error_tstring(ERR_get_error(), buffer));
   }

   X509_STORE_free(store);
   return valid;
}

/**
 * Reload certificates from database
 */
void ReloadCertificates()
{
	BYTE *pBinCert;
	OPENSSL_CONST BYTE *p;
	DB_RESULT hResult;
	int i, nRows, nLoaded;
	UINT32 dwLen;
	X509 *pCert;
	TCHAR szBuffer[256], szSubject[256], *pszCertData;

	s_certificateStoreLock.lock();

	if (s_trustedCertificateStore != NULL)
		X509_STORE_free(s_trustedCertificateStore);

	s_trustedCertificateStore = X509_STORE_new();
	if (s_trustedCertificateStore != NULL)
	{
	   // Add server's certificate as trusted
	   if (s_serverCertificate != NULL)
	      X509_STORE_add_cert(s_trustedCertificateStore, s_serverCertificate);

		_sntprintf(szBuffer, 256, _T("SELECT cert_data,subject FROM certificates WHERE cert_type=%d"), CERT_TYPE_TRUSTED_CA);
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		hResult = DBSelect(hdb, szBuffer);
		if (hResult != NULL)
		{
			nRows = DBGetNumRows(hResult);
			for(i = 0, nLoaded = 0; i < nRows; i++)
			{
				pszCertData = DBGetField(hResult, i, 0, NULL, 0);
				if (pszCertData != NULL)
				{
					dwLen = (UINT32)_tcslen(pszCertData);
					pBinCert = (BYTE *)malloc(dwLen);
					StrToBin(pszCertData, pBinCert, dwLen);
					free(pszCertData);
					p = pBinCert;
					pCert = d2i_X509(NULL, &p, dwLen);
					free(pBinCert);
					if (pCert != NULL)
					{
						if (X509_STORE_add_cert(s_trustedCertificateStore, pCert))
						{
							nLoaded++;
						}
						else
						{
							nxlog_write(MSG_CANNOT_ADD_CERT, EVENTLOG_ERROR_TYPE,
										"ss", DBGetField(hResult, i, 1, szSubject, 256),
										_ERR_error_tstring(ERR_get_error(), szBuffer));
						}
						X509_free(pCert); // X509_STORE_add_cert increments reference count
					}
					else
					{
						nxlog_write(MSG_CANNOT_LOAD_CERT, EVENTLOG_ERROR_TYPE,
									"ss", DBGetField(hResult, i, 1, szSubject, 256),
									_ERR_error_tstring(ERR_get_error(), szBuffer));
					}
				}
			}
			DBFreeResult(hResult);

			if (nLoaded > 0)
				nxlog_write(MSG_CA_CERTIFICATES_LOADED, EVENTLOG_INFORMATION_TYPE, "d", nLoaded);
		}
		DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		nxlog_write(MSG_CANNOT_INIT_CERT_STORE, EVENTLOG_ERROR_TYPE, "s", _ERR_error_tstring(ERR_get_error(), szBuffer));
	}

	s_certificateStoreLock.unlock();
}

/**
 * Certificate stuff initialization
 */
void InitCertificates()
{
   ReloadCertificates();
}

/**
 * Load server certificate
 */
bool LoadServerCertificate(RSA **serverKey)
{
   if (g_serverCertificatePath[0] == 0)
   {
      nxlog_write(MSG_SERVER_CERT_NOT_SET, NXLOG_INFO, NULL);
      return false;
   }

   FILE *f = _tfopen(g_serverCertificatePath, _T("r"));
   if (f == NULL)
   {
      nxlog_write(MSG_CANNOT_LOAD_SERVER_CERT, NXLOG_ERROR, "ss", g_serverCertificatePath, _tcserror(errno));
      return false;
   }

   DecryptPasswordA("system", g_serverCertificatePassword, g_serverCertificatePassword, MAX_PASSWORD);
   s_serverCertificate = PEM_read_X509(f, NULL, NULL, g_serverCertificatePassword);
   s_serverCertificateKey = PEM_read_PrivateKey(f, NULL, NULL, g_serverCertificatePassword);
   fclose(f);

   if ((s_serverCertificate == NULL) || (s_serverCertificateKey == NULL))
   {
      TCHAR buffer[1024];
      nxlog_write(MSG_CANNOT_LOAD_SERVER_CERT, NXLOG_ERROR, "ss", g_serverCertificatePath, _ERR_error_tstring(ERR_get_error(), buffer));
      return false;
   }

   RSA *privKey = EVP_PKEY_get1_RSA(s_serverCertificateKey);
   RSA *pubKey = EVP_PKEY_get1_RSA(X509_get_pubkey(s_serverCertificate));
   if ((privKey != NULL) && (pubKey != NULL))
   {
      // Combine into one key
      int len = i2d_RSAPublicKey(pubKey, NULL);
      len += i2d_RSAPrivateKey(privKey, NULL);
      BYTE *buffer = (BYTE *)malloc(len);

      BYTE *pos = buffer;
      i2d_RSAPublicKey(pubKey, &pos);
      i2d_RSAPrivateKey(privKey, &pos);

      *serverKey = RSAKeyFromData(buffer, len, true);
      free(buffer);
   }

   return true;
}

/**
 * Setup server-side TLS context
 */
bool SetupServerTlsContext(SSL_CTX *context)
{
   if ((s_serverCertificate == NULL) || (s_serverCertificateKey == NULL))
      return false;

   SSL_CTX_use_certificate(context, s_serverCertificate);
   SSL_CTX_use_PrivateKey(context, s_serverCertificateKey);
   return true;
}

#else		/* _WITH_ENCRYPTION */

/**
 * Stub for certificate initialization
 */
void InitCertificates()
{
}

#endif	/* _WITH_ENCRYPTION */
