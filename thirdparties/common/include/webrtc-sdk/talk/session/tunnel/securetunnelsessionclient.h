/*
 * libjingle
 * Copyright 2004--2008, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// SecureTunnelSessionClient and SecureTunnelSession.
// SecureTunnelSessionClient extends TunnelSessionClient to exchange
// certificates as part of the session description.
// SecureTunnelSession is a TunnelSession that wraps the underlying
// tunnel stream into an SSLStreamAdapter.

#ifndef TALK_SESSION_TUNNEL_SECURETUNNELSESSIONCLIENT_H_
#define TALK_SESSION_TUNNEL_SECURETUNNELSESSIONCLIENT_H_

#include <string>

#include "talk/base/sslidentity.h"
#include "talk/base/sslstreamadapter.h"
#include "talk/session/tunnel/tunnelsessionclient.h"

namespace cricket {

class SecureTunnelSession;  // below

// SecureTunnelSessionClient

// This TunnelSessionClient establishes secure tunnels protected by
// SSL/TLS. The PseudoTcpChannel stream is wrapped with an
// SSLStreamAdapter. An SSLIdentity must be set or generated.
//
// The TunnelContentDescription is extended to include the client and
// server certificates. The initiator acts as the client. The session
// initiate stanza carries a description that contains the client's
// certificate, and the session accept response's description has the
// server certificate added to it.

class SecureTunnelSessionClient : public TunnelSessionClient {
 public:
  // The jid is used as the name for sessions for outgoing tunnels.
  // manager is the SessionManager to which we register this client
  // and its sessions.
  SecureTunnelSessionClient(const buzz::Jid& jid, SessionManager* manager);

  // Configures this client to use a preexisting SSLIdentity.
  // The client takes ownership of the identity object.
  // Use either SetIdentity or GenerateIdentity, and only once.
  void SetIdentity(talk_base::SSLIdentity* identity);

  // Generates an identity from nothing.
  // Returns true if generation was successful.
  // Use either SetIdentity or GenerateIdentity, and only once.
  bool GenerateIdentity();

  // Returns our identity for SSL purposes, as either set by
  // SetIdentity() or generated by GenerateIdentity(). Call this
  // method only after our identity has been successfully established
  // by one of those methods.
  talk_base::SSLIdentity& GetIdentity() const;

  // Inherited methods
  virtual void OnIncomingTunnel(const buzz::Jid& jid, Session *session);
  virtual bool ParseContent(SignalingProtocol protocol,
                            const buzz::XmlElement* elem,
                            ContentDescription** content,
                            ParseError* error);
  virtual bool WriteContent(SignalingProtocol protocol,
                            const ContentDescription* content,
                            buzz::XmlElement** elem,
                            WriteError* error);
  virtual SessionDescription* CreateOffer(
      const buzz::Jid &jid, const std::string &description);
  virtual SessionDescription* CreateAnswer(
      const SessionDescription* offer);

 protected:
  virtual TunnelSession* MakeTunnelSession(
      Session* session, talk_base::Thread* stream_thread,
      TunnelSessionRole role);

 private:
  // Our identity (key and certificate) for SSL purposes. The
  // certificate part will be communicated within the session
  // description. The identity will be passed to the SSLStreamAdapter
  // and used for SSL authentication.
  talk_base::scoped_ptr<talk_base::SSLIdentity> identity_;

  DISALLOW_EVIL_CONSTRUCTORS(SecureTunnelSessionClient);
};

// SecureTunnelSession:
// A TunnelSession represents one session for one client. It
// provides the actual tunnel stream and handles state changes.
// A SecureTunnelSession is a TunnelSession that wraps the underlying
// tunnel stream into an SSLStreamAdapter.

class SecureTunnelSession : public TunnelSession {
 public:
  // This TunnelSession will tie together the given client and session.
  // stream_thread is passed to the PseudoTCPChannel: it's the thread
  // designated to interact with the tunnel stream.
  // role is either INITIATOR or RESPONDER, depending on who is
  // initiating the session.
  SecureTunnelSession(SecureTunnelSessionClient* client, Session* session,
                      talk_base::Thread* stream_thread,
                      TunnelSessionRole role);

  // Returns the stream that implements the actual P2P tunnel.
  // This may be called only once. Caller is responsible for freeing
  // the returned object.
  virtual talk_base::StreamInterface* GetStream();

 protected:
  // Inherited method: callback on accepting a session.
  virtual void OnAccept();

  // Helper method for GetStream() that Instantiates the
  // SSLStreamAdapter to wrap the PseudoTcpChannel's stream, and
  // configures it with our identity and role.
  talk_base::StreamInterface* MakeSecureStream(
      talk_base::StreamInterface* stream);

  // Our role in requesting the tunnel: INITIATOR or
  // RESPONDER. Translates to our role in SSL negotiation:
  // respectively client or server. Also indicates which slot of the
  // SecureTunnelContentDescription our cert goes into: client-cert or
  // server-cert respectively.
  TunnelSessionRole role_;

  // This is the stream representing the usable tunnel endpoint.  It's
  // a StreamReference wrapping the SSLStreamAdapter instance, which
  // further wraps a PseudoTcpChannel::InternalStream. The
  // StreamReference is because in the case of CreateTunnel(), the
  // stream endpoint is returned early, but we need to keep a handle
  // on it so we can setup the peer certificate when we receive it
  // later.
  talk_base::scoped_ptr<talk_base::StreamReference> ssl_stream_reference_;

  DISALLOW_EVIL_CONSTRUCTORS(SecureTunnelSession);
};

}  // namespace cricket

#endif  // TALK_SESSION_TUNNEL_SECURETUNNELSESSIONCLIENT_H_
