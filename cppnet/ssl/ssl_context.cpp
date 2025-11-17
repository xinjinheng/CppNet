// Use of this source code is governed by a BSD 3-Clause License
// that can be found in the LICENSE file.

// Author: caozhiyi (caozhiyi5@gmail.com)

#include "ssl_context.h"

namespace cppnet {

SSLContext::SSLContext() : 
    _server(true),
    _initialized(false),
    _ssl_ctx(nullptr) {
#ifdef USE_OPENSSL
    SSL_library_init();
    SSL_load_error_strings();
#endif
}

SSLContext::~SSLContext() {
#ifdef USE_OPENSSL
    if (_ssl_ctx) {
        SSL_CTX_free((SSL_CTX*)_ssl_ctx);
    }
#endif
}

bool SSLContext::Init(bool server, const std::string& cert_file, const std::string& key_file) {
    if (_initialized) {
        return true;
    }

    _server = server;

#ifdef USE_OPENSSL
    const SSL_METHOD* method;
    if (_server) {
        method = TLS_server_method();
    } else {
        method = TLS_client_method();
    }

    _ssl_ctx = SSL_CTX_new(method);
    if (!_ssl_ctx) {
        return false;
    }

    if (!cert_file.empty() && !key_file.empty()) {
        if (!LoadCertificates(cert_file, key_file)) {
            SSL_CTX_free((SSL_CTX*)_ssl_ctx);
            _ssl_ctx = nullptr;
            return false;
        }
    }

    _initialized = true;
#endif
    return true;
}

bool SSLContext::InitClient(const std::string& ca_file) {
    if (!Init(false)) {
        return false;
    }

#ifdef USE_OPENSSL
    if (!ca_file.empty()) {
        if (!LoadCAFile(ca_file)) {
            return false;
        }
    }
#endif
    return true;
}

bool SSLContext::InitServer(const std::string& cert_file, const std::string& key_file, const std::string& ca_file) {
    if (!Init(true, cert_file, key_file)) {
        return false;
    }

#ifdef USE_OPENSSL
    if (!ca_file.empty()) {
        if (!LoadCAFile(ca_file)) {
            return false;
        }
    }
#endif
    return true;
}

bool SSLContext::LoadCertificates(const std::string& cert_file, const std::string& key_file) {
#ifdef USE_OPENSSL
    if (!_ssl_ctx) {
        return false;
    }

    if (SSL_CTX_use_certificate_file((SSL_CTX*)_ssl_ctx, cert_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file((SSL_CTX*)_ssl_ctx, key_file.c_str(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }

    if (!SSL_CTX_check_private_key((SSL_CTX*)_ssl_ctx)) {
        return false;
    }

    _cert_file = cert_file;
    _key_file = key_file;
    return true;
#else
    return false;
#endif
}

bool SSLContext::LoadCAFile(const std::string& ca_file) {
#ifdef USE_OPENSSL
    if (!_ssl_ctx) {
        return false;
    }

    if (SSL_CTX_load_verify_locations((SSL_CTX*)_ssl_ctx, ca_file.c_str(), nullptr) <= 0) {
        return false;
    }

    _ca_file = ca_file;
    return true;
#else
    return false;
#endif
}

bool SSLContext::SetCipherList(const std::string& cipher_list) {
#ifdef USE_OPENSSL
    if (!_ssl_ctx) {
        return false;
    }

    if (SSL_CTX_set_cipher_list((SSL_CTX*)_ssl_ctx, cipher_list.c_str()) <= 0) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool SSLContext::EnableSessionCache(bool enable) {
#ifdef USE_OPENSSL
    if (!_ssl_ctx) {
        return false;
    }

    if (enable) {
        SSL_CTX_set_session_cache_mode((SSL_CTX*)_ssl_ctx, SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_CLIENT);
        SSL_CTX_sess_set_cache_size((SSL_CTX*)_ssl_ctx, 1024);
    } else {
        SSL_CTX_set_session_cache_mode((SSL_CTX*)_ssl_ctx, SSL_SESS_CACHE_OFF);
    }
    return true;
#else
    return false;
#endif
}

bool SSLContext::EnableVerifyPeer(bool enable) {
#ifdef USE_OPENSSL
    if (!_ssl_ctx) {
        return false;
    }

    if (_server) {
        if (enable) {
            SSL_CTX_set_verify((SSL_CTX*)_ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
        } else {
            SSL_CTX_set_verify((SSL_CTX*)_ssl_ctx, SSL_VERIFY_NONE, nullptr);
        }
    } else {
        if (enable) {
            SSL_CTX_set_verify((SSL_CTX*)_ssl_ctx, SSL_VERIFY_PEER, nullptr);
        } else {
            SSL_CTX_set_verify((SSL_CTX*)_ssl_ctx, SSL_VERIFY_NONE, nullptr);
        }
    }
    return true;
#else
    return false;
#endif
}

// SSLSession implementation

SSLSession::SSLSession() : 
    _ssl(nullptr),
    _handshaked(false),
    _closing(false),
    _sockfd(0) {
}

SSLSession::~SSLSession() {
#ifdef USE_OPENSSL
    if (_ssl) {
        SSL_free((SSL*)_ssl);
    }
#endif
}

bool SSLSession::Init(std::shared_ptr<SSLContext> context, uint64_t sockfd) {
    if (!context || !context->IsInitialized() || !sockfd) {
        return false;
    }

#ifdef USE_OPENSSL
    _ssl = SSL_new((SSL_CTX*)context->GetNativeContext());
    if (!_ssl) {
        return false;
    }

    _sockfd = sockfd;
    SSL_set_fd((SSL*)_ssl, sockfd);
    _context = context;
    
    if (context->IsServer()) {
        SSL_set_accept_state((SSL*)_ssl);
    } else {
        SSL_set_connect_state((SSL*)_ssl);
    }

    return true;
#else
    return false;
#endif
}

bool SSLSession::Handshake() {
    if (!_ssl || _handshaked) {
        return true;
    }

#ifdef USE_OPENSSL
    int32_t ret = SSL_do_handshake((SSL*)_ssl);
    if (ret == 1) {
        _handshaked = true;
        return true;
    } else {
        int32_t err = SSL_get_error((SSL*)_ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            // 需要继续握手
            return false;
        } else {
            // 握手失败
            return false;
        }
    }
#else
    return false;
#endif
}

int32_t SSLSession::Read(void* buffer, int32_t len) {
    if (!_ssl || !_handshaked || !buffer || len <= 0) {
        return -1;
    }

#ifdef USE_OPENSSL
    int32_t ret = SSL_read((SSL*)_ssl, buffer, len);
    if (ret <= 0) {
        int32_t err = SSL_get_error((SSL*)_ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;
        } else {
            return -1;
        }
    }
    return ret;
#else
    return -1;
#endif
}

int32_t SSLSession::Write(const void* buffer, int32_t len) {
    if (!_ssl || !_handshaked || !buffer || len <= 0) {
        return -1;
    }

#ifdef USE_OPENSSL
    int32_t ret = SSL_write((SSL*)_ssl, buffer, len);
    if (ret <= 0) {
        int32_t err = SSL_get_error((SSL*)_ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;
        } else {
            return -1;
        }
    }
    return ret;
#else
    return -1;
#endif
}

bool SSLSession::Shutdown() {
    if (!_ssl || _closing) {
        return true;
    }

#ifdef USE_OPENSSL
    int32_t ret = SSL_shutdown((SSL*)_ssl);
    if (ret == 1) {
        _closing = true;
        return true;
    } else if (ret == 0) {
        // 需要第二次调用
        ret = SSL_shutdown((SSL*)_ssl);
        if (ret == 1) {
            _closing = true;
            return true;
        }
    }
    return false;
#else
    return true;
#endif
}

bool SSLSession::Close() {
    if (!_ssl) {
        return true;
    }

#ifdef USE_OPENSSL
    SSL_free((SSL*)_ssl);
    _ssl = nullptr;
    _handshaked = false;
    _closing = true;
    return true;
#else
    return true;
#endif
}

// SSLCertificateManager implementation

SSLCertificateManager::SSLCertificateManager() {
}

SSLCertificateManager::~SSLCertificateManager() {
}

bool SSLCertificateManager::AddCertificate(const std::string& domain, std::shared_ptr<SSLContext> context) {
    if (domain.empty() || !context) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _certificates.find(domain);
    if (it != _certificates.end()) {
        return false;
    }

    _certificates[domain] = context;
    return true;
}

bool SSLCertificateManager::RemoveCertificate(const std::string& domain) {
    if (domain.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _certificates.find(domain);
    if (it == _certificates.end()) {
        return false;
    }

    _certificates.erase(it);
    return true;
}

std::shared_ptr<SSLContext> SSLCertificateManager::GetCertificate(const std::string& domain) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _certificates.find(domain);
    if (it != _certificates.end()) {
        return it->second;
    }
    return GetDefaultCertificate();
}

bool SSLCertificateManager::UpdateCertificate(const std::string& domain, const std::string& cert_file, const std::string& key_file) {
    std::shared_ptr<SSLContext> new_ctx = std::make_shared<SSLContext>();
    if (!new_ctx->InitServer(cert_file, key_file)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _certificates[domain] = new_ctx;
    return true;
}

std::shared_ptr<SSLContext> SSLCertificateManager::GetDefaultCertificate() {
    std::lock_guard<std::mutex> lock(_mutex);
    return _default_cert;
}

bool SSLCertificateManager::SetDefaultCertificate(std::shared_ptr<SSLContext> context) {
    if (!context || !context->IsInitialized()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _default_cert = context;
    return true;
}

} // namespace cppnet