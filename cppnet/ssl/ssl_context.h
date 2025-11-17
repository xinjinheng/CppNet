// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#ifndef CPPNET_SSL_SSL_CONTEXT
#define CPPNET_SSL_SSL_CONTEXT

#include <memory>
#include <string>
#include <mutex>

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#endif

namespace cppnet {

class SSLContext {
public:
    SSLContext();
    ~SSLContext();

    bool Init(bool server = true, const std::string& cert_file = "", const std::string& key_file = "");
    bool InitClient(const std::string& ca_file = "");
    bool InitServer(const std::string& cert_file, const std::string& key_file, const std::string& ca_file = "");

    bool LoadCertificates(const std::string& cert_file, const std::string& key_file);
    bool LoadCAFile(const std::string& ca_file);
    bool SetCipherList(const std::string& cipher_list);
    bool EnableSessionCache(bool enable = true);
    bool EnableVerifyPeer(bool enable = true);

    void* GetNativeContext() const { return _ssl_ctx; }

    bool IsServer() const { return _server; }
    bool IsInitialized() const { return _initialized; }
private:
    bool _server;
    bool _initialized;
    void* _ssl_ctx;
    std::mutex _mutex;

    std::string _cert_file;
    std::string _key_file;
    std::string _ca_file;
};

class SSLSession {
public:
    SSLSession();
    ~SSLSession();

    bool Init(std::shared_ptr<SSLContext> context, uint64_t sockfd);
    bool Handshake();
    int32_t Read(void* buffer, int32_t len);
    int32_t Write(const void* buffer, int32_t len);
    bool Shutdown();
    bool Close();

    void* GetNativeSession() const { return _ssl; }
    bool IsHandshaked() const { return _handshaked; }
    bool IsClosing() const { return _closing; }
private:
    void* _ssl;
    bool _handshaked;
    bool _closing;
    uint64_t _sockfd;
    std::weak_ptr<SSLContext> _context;
};

class SSLCertificateManager {
public:
    SSLCertificateManager();
    ~SSLCertificateManager();

    bool AddCertificate(const std::string& domain, std::shared_ptr<SSLContext> context);
    bool RemoveCertificate(const std::string& domain);
    std::shared_ptr<SSLContext> GetCertificate(const std::string& domain);
    bool UpdateCertificate(const std::string& domain, const std::string& cert_file, const std::string& key_file);

    std::shared_ptr<SSLContext> GetDefaultCertificate();
    bool SetDefaultCertificate(std::shared_ptr<SSLContext> context);

private:
    std::unordered_map<std::string, std::shared_ptr<SSLContext>> _certificates;
    std::shared_ptr<SSLContext> _default_cert;
    std::mutex _mutex;
};

} // namespace cppnet

#endif