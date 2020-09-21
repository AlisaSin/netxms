/*
** NetXMS multiplatform core agent
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
** File: tunnel.cpp
**
**/

#include "nxagentd.h"
#include <nxstat.h>

#define DEBUG_TAG _T("tunnel")

#define REQUEST_TIMEOUT	10000

#ifdef _WITH_ENCRYPTION

#ifndef _WIN32
#define DEFAULT_STORE "/etc/ssl/certs"
#endif

/**
 * Check if server address is valid
 */
bool IsValidServerAddress(const InetAddress &addr, bool *pbMasterServer, bool *pbControlServer, bool forceResolve);

/**
 * Register session
 */
bool RegisterSession(CommSession *session);

class Tunnel;

/**
 * Unique channel ID
 */
static VolatileCounter s_nextChannelId = 0;

/**
 * Tunnel communication channel
 */
class TunnelCommChannel : public AbstractCommChannel
{
private:
   Tunnel *m_tunnel;
   UINT32 m_id;
   bool m_active;
   VolatileCounter m_closed;
   RingBuffer m_buffer;
#ifdef _WIN32
   CRITICAL_SECTION m_bufferLock;
   CONDITION_VARIABLE m_dataCondition;
#else
   pthread_mutex_t m_bufferLock;
   pthread_cond_t m_dataCondition;
#endif

protected:
   virtual ~TunnelCommChannel();

public:
   TunnelCommChannel(Tunnel *tunnel);

   virtual ssize_t send(const void *data, size_t size, MUTEX mutex = INVALID_MUTEX_HANDLE) override;
   virtual ssize_t recv(void *buffer, size_t size, UINT32 timeout = INFINITE) override;
   virtual int poll(UINT32 timeout, bool write = false) override;
   virtual int shutdown() override;
   virtual void close() override;

   UINT32 getId() const { return m_id; }

   void putData(const BYTE *data, size_t size);
};

/**
 * Tunnel class
 */
class Tunnel
{
private:
   TCHAR *m_hostname;
   uint16_t m_port;
   InetAddress m_address;
   SOCKET m_socket;
   SSL_CTX *m_context;
   SSL *m_ssl;
   MUTEX m_sslLock;
   MUTEX m_writeLock;
   MUTEX m_stateLock;
   bool m_connected;
   bool m_reset;
   bool m_forceResolve;
   VolatileCounter m_requestId;
   THREAD m_recvThread;
   MsgWaitQueue *m_queue;
   RefCountHashMap<UINT32, TunnelCommChannel> m_channels;
   MUTEX m_channelLock;
   int m_tlsHandshakeFailures;
   bool m_ignoreClientCertificate;

   Tunnel(const TCHAR *hostname, uint16_t port);

   bool connectToServer();
   int sslWrite(const void *data, size_t size);
   bool sendMessage(const NXCPMessage& msg);
   NXCPMessage *waitForMessage(uint16_t code, uint32_t id) { return (m_queue != nullptr) ? m_queue->waitForMessage(code, id, REQUEST_TIMEOUT) : nullptr; }

   void processBindRequest(NXCPMessage *request);
   void processChannelCloseRequest(const NXCPMessage& request);
   void createSession(const NXCPMessage& request);

   X509_REQ *createCertificateRequest(const char *country, const char *org, const char *cn, EVP_PKEY **pkey);
   bool saveCertificate(X509 *cert, EVP_PKEY *key);
   bool loadCertificate();

   void recvThread();
   static THREAD_RESULT THREAD_CALL recvThreadStarter(void *arg);

public:
   ~Tunnel();

   void checkConnection();
   void disconnect();

   TunnelCommChannel *createChannel();
   void closeChannel(TunnelCommChannel *channel);
   ssize_t sendChannelData(uint32_t id, const void *data, size_t len);

   const TCHAR *getHostname() const { return m_hostname; }

   void debugPrintf(int level, const TCHAR *format, ...);

   static Tunnel *createFromConfig(const TCHAR *config);
};

/**
 * Tunnel constructor
 */
Tunnel::Tunnel(const TCHAR *hostname, uint16_t port) : m_channels(Ownership::True)
{
   m_hostname = MemCopyString(hostname);
   m_port = port;
   m_socket = INVALID_SOCKET;
   m_context = NULL;
   m_ssl = NULL;
   m_sslLock = MutexCreate();
   m_writeLock = MutexCreate();
   m_stateLock = MutexCreate();
   m_connected = false;
   m_reset = false;
   m_forceResolve = false;
   m_requestId = 0;
   m_recvThread = INVALID_THREAD_HANDLE;
   m_queue = NULL;
   m_channelLock = MutexCreate();
   m_tlsHandshakeFailures = 0;
   m_ignoreClientCertificate = false;
}

/**
 * Tunnel destructor
 */
Tunnel::~Tunnel()
{
   disconnect();
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
   if (m_ssl != NULL)
      SSL_free(m_ssl);
   if (m_context != NULL)
      SSL_CTX_free(m_context);
   MutexDestroy(m_sslLock);
   MutexDestroy(m_writeLock);
   MutexDestroy(m_stateLock);
   MutexDestroy(m_channelLock);
   MemFree(m_hostname);
}

/**
 * Debug output
 */
void Tunnel::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   TCHAR buffer[8192];
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);
   nxlog_debug_tag(DEBUG_TAG, level, _T("%s: %s"), m_hostname, buffer);
}

/**
 * Force disconnect
 */
void Tunnel::disconnect()
{
   MutexLock(m_stateLock);
   if (m_socket != INVALID_SOCKET)
      shutdown(m_socket, SHUT_RDWR);
   m_connected = false;
   ThreadJoin(m_recvThread);
   m_recvThread = INVALID_THREAD_HANDLE;
   delete_and_null(m_queue);
   MutexUnlock(m_stateLock);

   Array channels(g_maxCommSessions, 16, Ownership::False);
   MutexLock(m_channelLock);
   Iterator<TunnelCommChannel> *it = m_channels.iterator();
   while(it->hasNext())
   {
      TunnelCommChannel *c = it->next();
      channels.add(c);
      c->incRefCount();
   }
   delete it;
   MutexUnlock(m_channelLock);

   for(int i = 0; i < channels.size(); i++)
   {
      AbstractCommChannel *c = (AbstractCommChannel *)channels.get(i);
      c->close();
      c->decRefCount();
   }
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL Tunnel::recvThreadStarter(void *arg)
{
   char name[256];
#ifdef UNICODE
   snprintf(name, 256, "TunnelRecvThread/%S", static_cast<Tunnel*>(arg)->m_hostname);
#else
   snprintf(name, 256, "TunnelRecvThread/%s", static_cast<Tunnel*>(arg)->m_hostname);
#endif
   static_cast<Tunnel*>(arg)->recvThread();
   return THREAD_OK;
}

/**
 * Receiver thread
 */
void Tunnel::recvThread()
{
   TlsMessageReceiver receiver(m_socket, m_ssl, m_sslLock, 8192, MAX_AGENT_MSG_SIZE);
   while(m_connected)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(1000, &result);
      if (msg != NULL)
      {
         if (nxlog_get_debug_level() >= 6)
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
         }

         if (msg->getCode() == CMD_RESET_TUNNEL)
         {
            m_reset = true;
            debugPrintf(4, _T("Receiver thread stopped (tunnel reset)"));
            delete msg;
            break;
         }

         switch(msg->getCode())
         {
            case CMD_BIND_AGENT_TUNNEL:
               ThreadPoolExecute(g_commThreadPool, this, &Tunnel::processBindRequest, msg);
               msg = nullptr; // prevent message deletion
               break;
            case CMD_CREATE_CHANNEL:
               createSession(*msg);
               break;
            case CMD_CHANNEL_DATA:
               if (msg->isBinary())
               {
                  MutexLock(m_channelLock);
                  TunnelCommChannel *channel = m_channels.get(msg->getId());
                  MutexUnlock(m_channelLock);
                  if (channel != nullptr)
                  {
                     channel->putData(msg->getBinaryData(), msg->getBinaryDataSize());
                     channel->decRefCount();
                  }
               }
               break;
            case CMD_CLOSE_CHANNEL:
               processChannelCloseRequest(*msg);
               break;
            default:
               m_queue->put(msg);
               msg = nullptr; // prevent message deletion
               break;
         }
         delete msg;
      }
      else if (result != MSGRECV_TIMEOUT)
      {
         debugPrintf(4, _T("Receiver thread stopped (%s)"), AbstractMessageReceiver::resultToText(result));
         m_reset = true;
         break;
      }
   }
   nxlog_report_event(61, NXLOG_WARNING, 1, _T("Tunnel with %s closed"), m_hostname);
}

/**
 * Write to SSL
 */
int Tunnel::sslWrite(const void *data, size_t size)
{
   if (!m_connected || m_reset)
      return -1;

   bool canRetry;
   int bytes;
   MutexLock(m_writeLock);
   do
   {
      canRetry = false;
      MutexLock(m_sslLock);
      bytes = SSL_write(m_ssl, data, static_cast<int>(size));
      if (bytes <= 0)
      {
         int err = SSL_get_error(m_ssl, bytes);
         if ((err == SSL_ERROR_WANT_READ) || (err == SSL_ERROR_WANT_WRITE))
         {
            MutexUnlock(m_sslLock);
            SocketPoller sp(err == SSL_ERROR_WANT_WRITE);
            sp.add(m_socket);
            if (sp.poll(REQUEST_TIMEOUT) > 0)
               canRetry = true;
            MutexLock(m_sslLock);
         }
         else
         {
            debugPrintf(7, _T("SSL_write error (bytes=%d ssl_err=%d socket_err=%d)"), bytes, err, WSAGetLastError());
            if (err == SSL_ERROR_SSL)
               LogOpenSSLErrorStack(7);
         }
      }
      MutexUnlock(m_sslLock);
   }
   while(canRetry);
   MutexUnlock(m_writeLock);
   return bytes;
}

/**
 * Send message
 */
bool Tunnel::sendMessage(const NXCPMessage& msg)
{
   if (!m_connected || m_reset)
      return false;

   if (nxlog_get_debug_level() >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg.getCode(), buffer));
   }
   NXCP_MESSAGE *data = msg.serialize(false);
   bool success = (sslWrite(data, ntohl(data->size)) == ntohl(data->size));
   MemFree(data);
   return success;
}

/**
 * Load certificate for this tunnel
 */
bool Tunnel::loadCertificate()
{
   BYTE addressHash[SHA1_DIGEST_SIZE];
#ifdef UNICODE
   char *un = MBStringFromWideString(m_hostname);
   CalculateSHA1Hash((BYTE *)un, strlen(un), addressHash);
   MemFree(un);
#else
   CalculateSHA1Hash((BYTE *)m_hostname, strlen(m_hostname), addressHash);
#endif

   TCHAR prefix[48];
   BinToStr(addressHash, SHA1_DIGEST_SIZE, prefix);

   TCHAR name[MAX_PATH];
   _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);
   FILE *f = _tfopen(name, _T("r"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      if (errno == ENOENT)
      {
         // Try fallback file
         m_address.buildHashKey(addressHash);
         BinToStr(addressHash, 18, prefix);
         _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);

         f = _tfopen(name, _T("r"));
         if (f == NULL)
         {
            debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
            return false;
         }
      }
      else
      {
         return false;
      }
   }

   X509 *cert = PEM_read_X509(f, NULL, NULL, NULL);
   fclose(f);

   if (cert == NULL)
   {
      debugPrintf(4, _T("Cannot load certificate from file \"%s\""), name);
      return false;
   }

   _sntprintf(name, MAX_PATH, _T("%s%s.key"), g_certificateDirectory, prefix);
   f = _tfopen(name, _T("r"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      X509_free(cert);
      return false;
   }

   EVP_PKEY *key = PEM_read_PrivateKey(f, NULL, NULL, (void *)"nxagentd");
   fclose(f);

   if (key == NULL)
   {
      debugPrintf(4, _T("Cannot load private key from file \"%s\""), name);
      X509_free(cert);
      return false;
   }

   bool success = false;
   if (SSL_CTX_use_certificate(m_context, cert) == 1)
   {
      if (SSL_CTX_use_PrivateKey(m_context, key) == 1)
      {
         debugPrintf(4, _T("Certificate and private key loaded"));
         success = true;
      }
      else
      {
         debugPrintf(4, _T("Cannot set private key"));
      }
   }
   else
   {
      debugPrintf(4, _T("Cannot set certificate"));
   }

   X509_free(cert);
   EVP_PKEY_free(key);
   return success;
}

/**
 * SSL message callback
 */
static void SSLInfoCallback(const SSL *ssl, int where, int ret)
{
   if (where & SSL_CB_ALERT)
   {
      nxlog_debug_tag(_T("ssl"), 4, _T("SSL %s alert: %hs (%hs)"), (where & SSL_CB_READ) ? _T("read") : _T("write"),
               SSL_alert_type_string_long(ret), SSL_alert_desc_string_long(ret));
   }
   else if (where & SSL_CB_HANDSHAKE_START)
   {
      nxlog_debug_tag(_T("ssl"), 6, _T("SSL handshake start (%hs)"), SSL_state_string_long(ssl));
   }
   else if (where & SSL_CB_HANDSHAKE_DONE)
   {
      nxlog_debug_tag(_T("ssl"), 6, _T("SSL handshake done (%hs)"), SSL_state_string_long(ssl));
   }
   else
   {
      int method = where & ~SSL_ST_MASK;
      const TCHAR *prefix;
      if (method & SSL_ST_CONNECT)
         prefix = _T("SSL_connect");
      else if (method & SSL_ST_ACCEPT)
         prefix = _T("SSL_accept");
      else
         prefix = _T("undefined");

      if (where & SSL_CB_LOOP)
      {
         nxlog_debug_tag(_T("ssl"), 6, _T("%s: %hs"), prefix, SSL_state_string_long(ssl));
      }
      else if (where & SSL_CB_EXIT)
      {
         if (ret == 0)
            nxlog_debug_tag(_T("ssl"), 3, _T("%s: failed in %hs"), prefix, SSL_state_string_long(ssl));
         else if (ret < 0)
            nxlog_debug_tag(_T("ssl"), 3, _T("%s: error in %hs"), prefix, SSL_state_string_long(ssl));
      }
   }
}

/**
 * Validate server certificate
 */
static bool ValidateServerCertificate(X509 *cert)
{
   bool valid = false;
   X509_STORE *trustedCertificateStore = X509_STORE_new();
   if (trustedCertificateStore == nullptr)
   {
      nxlog_write(NXLOG_INFO, _T("ValidateServerCertificate: cannot create certificate store"));
      return valid;
   }

   X509_LOOKUP *dirLookup = X509_STORE_add_lookup(trustedCertificateStore, X509_LOOKUP_hash_dir());
   X509_LOOKUP *fileLookup = X509_STORE_add_lookup(trustedCertificateStore, X509_LOOKUP_file());

   if (!g_trustedRootCertificates.isEmpty())
   {
      auto it = g_trustedRootCertificates.iterator();
      while(it->hasNext())
      {
         const TCHAR *trustedRoot = it->next();
         NX_STAT_STRUCT st;
         if (CALL_STAT(trustedRoot, &st) != 0)
            continue;

#ifdef UNICODE
         char mbTrustedRoot[MAX_PATH];
         WideCharToMultiByteSysLocale(trustedRoot, mbTrustedRoot, MAX_PATH);
#else
         const char *mbTrustedRoot = trustedRoot;
#endif
         int added = 0;
         if (S_ISDIR(st.st_mode))
         {
            added = X509_LOOKUP_add_dir(dirLookup, mbTrustedRoot, X509_FILETYPE_PEM);
         }
         else
         {
            added = X509_LOOKUP_load_file(fileLookup, mbTrustedRoot, X509_FILETYPE_PEM);
         }
         nxlog_debug_tag(DEBUG_TAG, 7, _T("ValidateServerCertificate: trusted root %s \"%s\" loaded"), S_ISDIR(st.st_mode) ? _T("directory") : _T("certificate"), trustedRoot);
      }
      delete it;
   }

#ifdef _WIN32
   //TODO: load default store from win api
#else
   struct stat fileInfo;
   if (stat(DEFAULT_STORE, &fileInfo) == 0)
   {
      X509_LOOKUP_add_dir(dirLookup, DEFAULT_STORE, X509_FILETYPE_PEM);
   }
#endif

   X509_STORE_CTX *ctx = X509_STORE_CTX_new();
   if (ctx != nullptr)
   {
      X509_STORE_CTX_init(ctx, trustedCertificateStore, cert, nullptr);
      int result = X509_verify_cert(ctx);
      valid = (result == 1);
      if (!valid)
         nxlog_debug_tag(DEBUG_TAG, 4, _T("ValidateServerCertificate: validation failed with reason: %hs"), X509_verify_cert_error_string(X509_STORE_CTX_get_error(ctx)));

      X509_STORE_CTX_free(ctx);
   }
   else
   {
      TCHAR buffer[256];
      nxlog_debug_tag(DEBUG_TAG, 3, _T("ValidateServerCertificate: X509_STORE_CTX_new() failed: %s"), _ERR_error_tstring(ERR_get_error(), buffer));
   }
   X509_STORE_free(trustedCertificateStore);

   return valid;
}

/**
 * Connect to server
 */
bool Tunnel::connectToServer()
{
   MutexLock(m_stateLock);

   // Cleanup from previous connection attempt
   if (m_socket != INVALID_SOCKET)
      closesocket(m_socket);
   if (m_ssl != NULL)
      SSL_free(m_ssl);
   if (m_context != NULL)
      SSL_CTX_free(m_context);

   m_socket = INVALID_SOCKET;
   m_context = NULL;
   m_ssl = NULL;

   m_address = InetAddress::resolveHostName(m_hostname);
   if (!m_address.isValidUnicast())
   {
      debugPrintf(4, _T("Server address cannot be resolved or is not valid"));
      MutexUnlock(m_stateLock);
      return false;
   }

   // Create socket and connect
   m_socket = ConnectToHost(m_address, m_port, REQUEST_TIMEOUT);
   if (m_socket == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      debugPrintf(4, _T("Cannot establish connection (%s)"), GetLastSocketErrorText(buffer, 1024));
      MutexUnlock(m_stateLock);
      return false;
   }

   // Setup secure connection
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   const SSL_METHOD *method = TLS_method();
#else
   const SSL_METHOD *method = SSLv23_method();
#endif
   if (method == NULL)
   {
      debugPrintf(4, _T("Cannot obtain TLS method"));
      MutexUnlock(m_stateLock);
      return false;
   }

   m_context = SSL_CTX_new((SSL_METHOD *)method);
   if (m_context == NULL)
   {
      debugPrintf(4, _T("Cannot create TLS context"));
      MutexUnlock(m_stateLock);
      return false;
   }
   if (g_dwFlags & AF_ENABLE_SSL_TRACE)
   {
      SSL_CTX_set_info_callback(m_context, SSLInfoCallback);
   }
#ifdef SSL_OP_NO_COMPRESSION
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);
#else
   SSL_CTX_set_options(m_context, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
#endif
   bool certificateLoaded = m_ignoreClientCertificate ? false : loadCertificate();
   m_ignoreClientCertificate = false;  // reset ignore flag for next try

   m_ssl = SSL_new(m_context);
   if (m_ssl == NULL)
   {
      debugPrintf(4, _T("Cannot create SSL object"));
      MutexUnlock(m_stateLock);
      return false;
   }

   SSL_set_connect_state(m_ssl);
   SSL_set_fd(m_ssl, (int)m_socket);

   while(true)
   {
      int rc = SSL_do_handshake(m_ssl);
      if (rc != 1)
      {
         int sslErr = SSL_get_error(m_ssl, rc);
         if (sslErr == SSL_ERROR_WANT_READ)
         {
            SocketPoller poller;
            poller.add(m_socket);
            if (poller.poll(REQUEST_TIMEOUT) > 0)
               continue;
            debugPrintf(4, _T("TLS handshake failed (timeout)"));
            MutexUnlock(m_stateLock);
            return false;
         }
         else
         {
            char buffer[128];
            debugPrintf(4, _T("TLS handshake failed (%hs)"), ERR_error_string(sslErr, buffer));

            unsigned long error;
            while((error = ERR_get_error()) != 0)
            {
               ERR_error_string_n(error, buffer, sizeof(buffer));
               debugPrintf(5, _T("Caused by: %hs"), buffer);
            }

            if (certificateLoaded)
            {
               m_tlsHandshakeFailures++;
               if (m_tlsHandshakeFailures >= 10)
               {
                  m_ignoreClientCertificate = true;
                  m_tlsHandshakeFailures = 0;
                  debugPrintf(4, _T("Next connection attempt will ignore agent certificate"));
               }
            }
            MutexUnlock(m_stateLock);
            return false;
         }
      }
      break;
   }

   m_tlsHandshakeFailures = 0;

   // Check server certificate
   X509 *cert = SSL_get_peer_certificate(m_ssl);
   if (cert == NULL)
   {
      debugPrintf(4, _T("Server certificate not provided"));
      MutexUnlock(m_stateLock);
      return false;
   }

   char *subj = X509_NAME_oneline(X509_get_subject_name(cert), NULL ,0);
   char *issuer = X509_NAME_oneline(X509_get_issuer_name(cert), NULL ,0);
   debugPrintf(4, _T("Server certificate subject is %hs"), subj);
   debugPrintf(4, _T("Server certificate issuer is %hs"), issuer);

   bool isValid = true;
   if (g_dwFlags & AF_CHECK_SERVER_CERTIFICATE)
   {
      isValid = ValidateServerCertificate(cert);
      debugPrintf(3, _T("Certificate \"%hs\" for issuer %hs - validation %s"),
            subj, issuer, isValid ? _T("successful") : _T("failed"));
   }
   OPENSSL_free(subj);
   OPENSSL_free(issuer);

   X509_free(cert);
   if (!isValid)
   {
      MutexUnlock(m_stateLock);
      return false;
   }

   // Setup receiver
   delete m_queue;
   m_queue = new MsgWaitQueue();
   m_connected = true;
   m_recvThread = ThreadCreateEx(Tunnel::recvThreadStarter, 0, this);

   MutexUnlock(m_stateLock);

   m_requestId = 0;

   // Do handshake
   NXCPMessage msg(CMD_SETUP_AGENT_TUNNEL, InterlockedIncrement(&m_requestId), 4);  // Use version 4 during setup
   msg.setField(VID_AGENT_VERSION, NETXMS_VERSION_STRING);
   msg.setField(VID_AGENT_BUILD_TAG, NETXMS_BUILD_TAG);
   msg.setField(VID_AGENT_ID, g_agentId);
   msg.setField(VID_SYS_NAME, g_systemName);
   msg.setField(VID_ZONE_UIN, g_zoneUIN);
   msg.setField(VID_USERAGENT_INSTALLED, IsUserAgentInstalled());
   msg.setField(VID_AGENT_PROXY, (g_dwFlags & AF_ENABLE_PROXY) ? true : false);
   msg.setField(VID_SNMP_PROXY, (g_dwFlags & AF_ENABLE_SNMP_PROXY) ? true : false);
   msg.setField(VID_SNMP_TRAP_PROXY, (g_dwFlags & AF_ENABLE_SNMP_TRAP_PROXY) ? true : false);
   msg.setField(VID_SYSLOG_PROXY, (g_dwFlags & AF_ENABLE_SYSLOG_PROXY) ? true : false);

   TCHAR fqdn[256];
   if (GetLocalHostName(fqdn, 256, true))
      msg.setField(VID_HOSTNAME, fqdn);

   VirtualSession session(0);
   TCHAR buffer[MAX_RESULT_LENGTH];
   if (GetParameterValue(_T("System.PlatformName"), buffer, &session) == ERR_SUCCESS)
      msg.setField(VID_PLATFORM_NAME, buffer);
   if (GetParameterValue(_T("System.UName"), buffer, &session) == ERR_SUCCESS)
      msg.setField(VID_SYS_DESCRIPTION, buffer);

   BYTE hwid[HARDWARE_ID_LENGTH];
   if (GetSystemHardwareId(hwid))
      msg.setField(VID_HARDWARE_ID, hwid, sizeof(hwid));

   sendMessage(msg);

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response == NULL)
   {
      debugPrintf(4, _T("Cannot configure tunnel (request timeout)"));
      disconnect();
      return false;
   }

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   if (rcc != ERR_SUCCESS)
   {
      debugPrintf(4, _T("Cannot configure tunnel (error %d)"), rcc);
      disconnect();
      return false;
   }

   // Force master/control server DNS names resolve after new tunnel is established
   // This will fix the following situation:
   //    - server DNS name was not available at agent startup
   //    - DNS name was resolved later and used for establishing tunnel
   //    - first session from server got wrong access because DNS name in server info structure still unresolved
   m_forceResolve = true;

   nxlog_report_event(60, NXLOG_INFO, 1, _T("Tunnel with %s established"), m_hostname);
   return true;
}

/**
 * Check tunnel connection and connect as needed
 */
void Tunnel::checkConnection()
{
   if (m_reset)
   {
      m_reset = false;
      disconnect();
      debugPrintf(3, _T("Resetting tunnel"));
      if (connectToServer())
         debugPrintf(3, _T("Tunnel is active"));
   }
   else if (!m_connected)
   {
      if (connectToServer())
         debugPrintf(3, _T("Tunnel is active"));
   }
   else
   {
      NXCPMessage msg(CMD_KEEPALIVE, InterlockedIncrement(&m_requestId), 4);
      if (sendMessage(&msg))
      {
         NXCPMessage *response = waitForMessage(CMD_KEEPALIVE, msg.getId());
         if (response == nullptr)
         {
            debugPrintf(3, _T("Connection test failed"));
            disconnect();
         }
         else
         {
            delete response;
         }
      }
      else
      {
         debugPrintf(3, _T("Connection test failed"));
         disconnect();
      }
   }
}

/**
 * Create certificate request
 */
X509_REQ *Tunnel::createCertificateRequest(const char *country, const char *org, const char *cn, EVP_PKEY **pkey)
{
   RSA *key = RSA_new();
   if (key == NULL)
   {
      debugPrintf(4, _T("call to RSA_new() failed"));
      return NULL;
   }

   BIGNUM *bn = BN_new();
   if (bn == NULL)
   {
      debugPrintf(4, _T("call to BN_new() failed"));
      RSA_free(key);
      return NULL;
   }

   BN_set_word(bn, RSA_F4);
   if (RSA_generate_key_ex(key, NETXMS_RSA_KEYLEN, bn, NULL) == -1)
   {
      debugPrintf(4, _T("call to RSA_generate_key_ex() failed"));
      RSA_free(key);
      BN_free(bn);
      return NULL;
   }
   BN_free(bn);

   X509_REQ *req = X509_REQ_new();
   if (req != NULL)
   {
      X509_REQ_set_version(req, 1);
      X509_NAME *subject = X509_REQ_get_subject_name(req);
      if (subject != NULL)
      {
         if (country != NULL)
            X509_NAME_add_entry_by_txt(subject, "C", MBSTRING_UTF8, (const BYTE *)country, -1, -1, 0);
         X509_NAME_add_entry_by_txt(subject, "O", MBSTRING_UTF8, (const BYTE *)((org != NULL) ? org : "netxms.org"), -1, -1, 0);
         X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_UTF8, (const BYTE *)cn, -1, -1, 0);

         EVP_PKEY *ekey = EVP_PKEY_new();
         if (ekey != NULL)
         {
            EVP_PKEY_assign_RSA(ekey, key);
            key = NULL; // will be freed by EVP_PKEY_free
            X509_REQ_set_pubkey(req, ekey);
            if (X509_REQ_sign(req, ekey, EVP_sha256()) > 0)
            {
               *pkey = ekey;
            }
            else
            {
               debugPrintf(4, _T("call to X509_REQ_sign() failed"));
               X509_REQ_free(req);
               req = NULL;
               EVP_PKEY_free(ekey);
            }
         }
         else
         {
            debugPrintf(4, _T("call to EVP_PKEY_new() failed"));
            X509_REQ_free(req);
            req = NULL;
         }
      }
      else
      {
         debugPrintf(4, _T("call to X509_REQ_get_subject_name() failed"));
         X509_REQ_free(req);
         req = NULL;
      }
   }
   else
   {
      debugPrintf(4, _T("call to X509_REQ_new() failed"));
   }

   if (key != NULL)
      RSA_free(key);
   return req;
}

/**
 * Creates certificate and key copy if files exist. Files are copied as NAME.DATE
 */
static void BackupFileIfExist(const TCHAR *name)
{
   if (_taccess(name, 0) != 0)
   {
     return;
   }

   TCHAR formatedTime[256];
   time_t t = time(NULL);
#if HAVE_LOCALTIME_R
   struct tm tmbuffer;
   struct tm *ltm = localtime_r(&t, &tmbuffer);
#else
   struct tm *ltm = localtime(&t);
#endif
   _tcsftime(formatedTime, 256, _T("%Y.%m.%d.%H.%M.%S"), ltm);

   TCHAR newName[MAX_PATH];
   _sntprintf(newName, MAX_PATH, _T("%s.%s"), name, formatedTime);

   _trename(name, newName);
}

/**
 * Save certificate
 */
bool Tunnel::saveCertificate(X509 *cert, EVP_PKEY *key)
{
   BYTE addressHash[SHA1_DIGEST_SIZE];
#ifdef UNICODE
   char *un = MBStringFromWideString(m_hostname);
   CalculateSHA1Hash((BYTE *)un, strlen(un), addressHash);
   free(un);
#else
   CalculateSHA1Hash((BYTE *)m_hostname, strlen(m_hostname), addressHash);
#endif

   TCHAR prefix[48];
   BinToStr(addressHash, SHA1_DIGEST_SIZE, prefix);

   TCHAR name[MAX_PATH];
   _sntprintf(name, MAX_PATH, _T("%s%s.crt"), g_certificateDirectory, prefix);
   BackupFileIfExist(name);
   FILE *f = _tfopen(name, _T("w"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      return false;
   }
   int rc = PEM_write_X509(f, cert);
   fclose(f);
   if (rc != 1)
   {
      debugPrintf(4, _T("PEM_write_X509(\"%s\") failed"), name);
      return false;
   }

   _sntprintf(name, MAX_PATH, _T("%s%s.key"), g_certificateDirectory, prefix);
   BackupFileIfExist(name);
   f = _tfopen(name, _T("w"));
   if (f == NULL)
   {
      debugPrintf(4, _T("Cannot open file \"%s\" (%s)"), name, _tcserror(errno));
      return false;
   }
   rc = PEM_write_PrivateKey(f, key, EVP_des_ede3_cbc(), NULL, 0, 0, (void *)"nxagentd");
   fclose(f);
   if (rc != 1)
   {
      debugPrintf(4, _T("PEM_write_PrivateKey(\"%s\") failed"), name);
      return false;
   }

   debugPrintf(4, _T("Certificate and private key saved"));
   return true;
}

/**
 * Process tunnel bind request
 */
void Tunnel::processBindRequest(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId(), 4);

   uuid guid = request->getFieldAsGUID(VID_GUID);
   char *cn = guid.toString().getUTF8String();

   char *country = request->getFieldAsUtf8String(VID_COUNTRY);
   char *org = request->getFieldAsUtf8String(VID_ORGANIZATION);

   EVP_PKEY *key = NULL;
   X509_REQ *req = createCertificateRequest(country, org, cn, &key);

   MemFree(country);
   MemFree(org);
   MemFree(cn);

   if (req != NULL)
   {
      BYTE *buffer = NULL;
      int len = i2d_X509_REQ(req, &buffer);
      if (len > 0)
      {
         NXCPMessage certRequest(CMD_REQUEST_CERTIFICATE, request->getId(), 4);
         certRequest.setField(VID_CERTIFICATE, buffer, len);
         sendMessage(&certRequest);
         OPENSSL_free(buffer);

         NXCPMessage *certResponse = waitForMessage(CMD_NEW_CERTIFICATE, request->getId());
         if (certResponse != nullptr)
         {
            UINT32 rcc = certResponse->getFieldAsUInt32(VID_RCC);
            if (rcc == ERR_SUCCESS)
            {
               size_t certLen;
               const BYTE *certData = certResponse->getBinaryFieldPtr(VID_CERTIFICATE, &certLen);
               if (certData != nullptr)
               {
                  X509 *cert = d2i_X509(NULL, &certData, (long)certLen);
                  if (cert != nullptr)
                  {
                     if (saveCertificate(cert, key))
                     {
                        response.setField(VID_RCC, ERR_SUCCESS);
                     }
                     else
                     {
                        response.setField(VID_RCC, ERR_IO_FAILURE);
                     }
                     X509_free(cert);
                  }
                  else
                  {
                     debugPrintf(4, _T("certificate data is invalid"));
                     response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
                  }
               }
               else
               {
                  debugPrintf(4, _T("certificate data missing in server response"));
                  response.setField(VID_RCC, ERR_INTERNAL_ERROR);
               }
            }
            else
            {
               debugPrintf(4, _T("certificate request failed (%d)"), rcc);
               response.setField(VID_RCC, rcc);
            }
            delete certResponse;
         }
         else
         {
            debugPrintf(4, _T("timeout waiting for certificate request completion"));
            response.setField(VID_RCC, ERR_REQUEST_TIMEOUT);
         }
      }
      else
      {
         debugPrintf(4, _T("call to i2d_X509_REQ() failed"));
         response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
      }
      X509_REQ_free(req);
      EVP_PKEY_free(key);
   }
   else
   {
      response.setField(VID_RCC, ERR_ENCRYPTION_ERROR);
   }

   sendMessage(&response);
   delete request;
}

/**
 * Create new session
 */
void Tunnel::createSession(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId(), 4);

   // Assume that peer always have minimal access, so don't check return value
   bool masterServer, controlServer;
   IsValidServerAddress(m_address, &masterServer, &controlServer, m_forceResolve);
   m_forceResolve = false;

   TunnelCommChannel *channel = createChannel();
   if (channel != NULL)
   {
      CommSession *session = new CommSession(channel, m_address, masterServer, controlServer);
      if (RegisterSession(session))
      {
         response.setField(VID_RCC, ERR_SUCCESS);
         response.setField(VID_CHANNEL_ID, channel->getId());
         debugPrintf(9, _T("New session registered"));
         session->run();
      }
      else
      {
         delete session;
         response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
      }
      channel->decRefCount();
   }
   else
   {
      response.setField(VID_RCC, ERR_OUT_OF_RESOURCES);
   }

   sendMessage(&response);
}

/**
 * Create channel
 */
TunnelCommChannel *Tunnel::createChannel()
{
   TunnelCommChannel *channel = NULL;
   MutexLock(m_channelLock);
   if (m_channels.size() < (int)g_maxCommSessions)
   {
      channel = new TunnelCommChannel(this);
      m_channels.set(channel->getId(), channel);
      debugPrintf(5, _T("New channel created (ID=%d)"), channel->getId());
   }
   MutexUnlock(m_channelLock);
   return channel;
}

/**
 * Process server's channel close request
 */
void Tunnel::processChannelCloseRequest(const NXCPMessage& request)
{
   uint32_t id = request.getFieldAsUInt32(VID_CHANNEL_ID);
   debugPrintf(5, _T("Close request for channel %d"), id);
   MutexLock(m_channelLock);
   TunnelCommChannel *channel = m_channels.get(id);
   MutexUnlock(m_channelLock);
   if (channel != nullptr)
   {
      channel->close();
      channel->decRefCount();
   }
}

/**
 * Close channel
 */
void Tunnel::closeChannel(TunnelCommChannel *channel)
{
   uint32_t id = 0;
   MutexLock(m_channelLock);
   if (m_channels.contains(channel->getId()))
   {
      id = channel->getId();
      debugPrintf(5, _T("Channel %d closed"), id);
      m_channels.remove(id);
   }
   MutexUnlock(m_channelLock);

   if (id != 0)
   {
      NXCPMessage msg(CMD_CLOSE_CHANNEL, 0, 4);
      msg.setField(VID_CHANNEL_ID, id);
      sendMessage(&msg);
   }
}

/**
 * Send channel data
 */
ssize_t Tunnel::sendChannelData(uint32_t id, const void *data, size_t len)
{
   NXCP_MESSAGE *msg = CreateRawNXCPMessage(CMD_CHANNEL_DATA, id, 0, data, len, NULL, false);
   int rc = sslWrite(msg, ntohl(msg->size));
   if (rc == ntohl(msg->size))
      rc = (int)len;  // adjust number of bytes to exclude tunnel overhead
   MemFree(msg);
   return rc;
}

/**
 * Create tunnel object from configuration record
 */
Tunnel *Tunnel::createFromConfig(const TCHAR *config)
{
   StringBuffer sb(config);
   int port = AGENT_TUNNEL_PORT;
   TCHAR *p = _tcschr(sb.getBuffer(), _T(':'));
   if (p != nullptr)
   {
      *p = 0;
      p++;

      TCHAR *eptr;
      port = _tcstol(p, &eptr, 10);
      if ((port < 1) || (port > 65535))
         return nullptr;
   }
   return new Tunnel(sb.cstr(), port);
}

/**
 * Channel constructor
 */
TunnelCommChannel::TunnelCommChannel(Tunnel *tunnel) : AbstractCommChannel(), m_buffer(32768, 32768)
{
   m_id = InterlockedIncrement(&s_nextChannelId);
   m_tunnel = tunnel;
   m_active = true;
   m_closed = 0;
#ifdef _WIN32
   InitializeCriticalSectionAndSpinCount(&m_bufferLock, 4000);
   InitializeConditionVariable(&m_dataCondition);
#else
   pthread_mutex_init(&m_bufferLock, NULL);
   pthread_cond_init(&m_dataCondition, NULL);
#endif
}

/**
 * Channel destructor
 */
TunnelCommChannel::~TunnelCommChannel()
{
#ifdef _WIN32
   DeleteCriticalSection(&m_bufferLock);
#else
   pthread_mutex_destroy(&m_bufferLock);
   pthread_cond_destroy(&m_dataCondition);
#endif
}

/**
 * Send data
 */
ssize_t TunnelCommChannel::send(const void *data, size_t size, MUTEX mutex)
{
   return m_active ? m_tunnel->sendChannelData(m_id, data, size) : -1;
}

/**
 * Receive data
 */
ssize_t TunnelCommChannel::recv(void *buffer, size_t size, UINT32 timeout)
{
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   if (!m_active && m_buffer.isEmpty())
   {
      LeaveCriticalSection(&m_bufferLock);
      return 0;   // closed
   }
   while (m_buffer.isEmpty())
   {
      if (!SleepConditionVariableCS(&m_dataCondition, &m_bufferLock, timeout))
      {
         LeaveCriticalSection(&m_bufferLock);
         return -2;
      }

      if (!m_active)
      {
         LeaveCriticalSection(&m_bufferLock);
         return 0;   // closed while waiting
      }
   }
#else
   pthread_mutex_lock(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
      if (!m_active)
      {
         pthread_mutex_unlock(&m_bufferLock);
         return 0;   // closed
      }

#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      int rc = pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      int rc = pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts);
#endif
      if (rc != 0)
      {
         pthread_mutex_unlock(&m_bufferLock);
         return -2;  // timeout
      }

      if (!m_active) // closed while waiting
      {
         pthread_mutex_unlock(&m_bufferLock);
         return 0;
      }
   }
#endif

   ssize_t bytes = m_buffer.read((BYTE *)buffer, size);
#ifdef _WIN32
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_unlock(&m_bufferLock);
#endif
   return bytes;
}

/**
 * Poll for data
 */
int TunnelCommChannel::poll(UINT32 timeout, bool write)
{
   if (write)
      return 1;

   if (!m_active)
      return -1;

#ifdef _WIN32
   int rc = 1;
   EnterCriticalSection(&m_bufferLock);
   while (m_buffer.isEmpty())
   {
      if (!SleepConditionVariableCS(&m_dataCondition, &m_bufferLock, timeout))
      {
         rc = 0;  // Timeout
         break;
      }

      if (!m_active)
      {
         rc = -1;
         break;
      }
   }
   LeaveCriticalSection(&m_bufferLock);
   return rc;
#else
   int rc = 0;
   pthread_mutex_lock(&m_bufferLock);
   if (m_buffer.isEmpty())
   {
#if HAVE_PTHREAD_COND_RELTIMEDWAIT_NP
      struct timespec ts;
      ts.tv_sec = timeout / 1000;
      ts.tv_nsec = (timeout % 1000) * 1000000;
      rc = pthread_cond_reltimedwait_np(&m_dataCondition, &m_bufferLock, &ts);
#else
      struct timeval now;
      struct timespec ts;
      gettimeofday(&now, NULL);
      ts.tv_sec = now.tv_sec + (timeout / 1000);
      now.tv_usec += (timeout % 1000) * 1000;
      ts.tv_sec += now.tv_usec / 1000000;
      ts.tv_nsec = (now.tv_usec % 1000000) * 1000;
      rc = pthread_cond_timedwait(&m_dataCondition, &m_bufferLock, &ts);
#endif
   }
   pthread_mutex_unlock(&m_bufferLock);
   return (rc == 0) ? 1 : 0;
#endif
}

/**
 * Shutdown channel
 */
int TunnelCommChannel::shutdown()
{
   m_active = false;
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
   WakeAllConditionVariable(&m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
#endif
   return 0;
}

/**
 * Close channel
 */
void TunnelCommChannel::close()
{
   if (InterlockedIncrement(&m_closed) > 1)
      return;  // already closed or close in progress
   shutdown();
   m_tunnel->closeChannel(this);
}

/**
 * Put data into buffer
 */
void TunnelCommChannel::putData(const BYTE *data, size_t size)
{
#ifdef _WIN32
   EnterCriticalSection(&m_bufferLock);
#else
   pthread_mutex_lock(&m_bufferLock);
#endif
   m_buffer.write(data, size);
#ifdef _WIN32
   WakeAllConditionVariable(&m_dataCondition);
   LeaveCriticalSection(&m_bufferLock);
#else
   pthread_cond_broadcast(&m_dataCondition);
   pthread_mutex_unlock(&m_bufferLock);
#endif
}

/**
 * Configured tunnels
 */
static ObjectArray<Tunnel> s_tunnels(0, 8, Ownership::True);

#endif	/* _WITH_ENCRYPTION */

/**
 * Parser server connection (tunnel) list
 */
void ParseTunnelList(const StringSet& tunnels)
{
#ifdef _WITH_ENCRYPTION
   auto it = tunnels.constIterator();
   while(it->hasNext())
   {
      const TCHAR *config = it->next();
      Tunnel *t = Tunnel::createFromConfig(config);
      if (t != nullptr)
      {
         s_tunnels.add(t);
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Added server tunnel %s"), t->getHostname());
      }
      else
      {
         nxlog_write(NXLOG_ERROR, _T("Invalid server connection configuration record \"%s\""), config);
      }
   }
   delete it;
#endif
}

/**
 * Tunnel manager
 */
void TunnelManager()
{
#ifdef _WITH_ENCRYPTION
   if (s_tunnels.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("No tunnels configured, tunnel manager will not start"));
      return;
   }

   g_tunnelKeepaliveInterval *= 1000;  // convert to milliseconds
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Tunnel manager started"));
   do
   {
      for(int i = 0; i < s_tunnels.size(); i++)
      {
         Tunnel *t = s_tunnels.get(i);
         t->checkConnection();
      }
   }
   while(!AgentSleepAndCheckForShutdown(g_tunnelKeepaliveInterval));

   // Shutdown all running tunnels
   for(int i = 0; i < s_tunnels.size(); i++)
      s_tunnels.get(i)->disconnect();

   nxlog_debug_tag(DEBUG_TAG, 3, _T("Tunnel manager stopped"));
#else
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Agent built without encryption support, tunnel manager will not start"));
#endif
}
