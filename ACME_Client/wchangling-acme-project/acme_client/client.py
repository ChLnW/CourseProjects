import requests
from typing import Optional
import requests.adapters
from binascii import Error as DecodeError
from jws import *
from object import *
import time
# import OpenSSL
from cryptography.hazmat.primitives import serialization, hashes
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography import x509
from http01_handler import HTTP01Server
from dns01_handler import DNS01Server

class ACMEClientError(Exception):
   def __init__(self, msg: str = '') -> None:
      print(f'ACMEClientError: {msg}', flush=True)
    
class ServerDirectory:
   def __init__(self, dir: dict) -> None:
      self.newNonce = dir['newNonce']
      self.newAccount = dir['newAccount']
      self.newOrder = dir['newOrder']
      self.revokeCert = dir['revokeCert']
      self.keyChange = dir['keyChange']
      self.newAuthz = dir['newAuthz'] if 'newAuthz' in dir else None
      self.meta = dir['meta'] if 'meta' in dir else None

class Client:
   DEFAULT_PEBBLE_DIR_URL = 'https://localhost:14000/dir'
   
   REPLAY_NONCE = 'Replay-Nonce'
   APP_JOSE_JSON = 'application/jose+json'

   RSA_PUBLIC_EXPONENT = 65537
   RSA_BIT_LEN = 2048
   
   def __init__(self, verify: Optional[str] = None, dirUrl: str = DEFAULT_PEBBLE_DIR_URL, 
                httpServer: Optional[HTTP01Server] = None,
                dnsServer: DNS01Server = None) -> None:
      self.jose = JOSE()
      self.session = requests.Session()
      adapter = requests.adapters.HTTPAdapter()
      self.session.mount("http://", adapter)
      self.session.mount("https://", adapter)
      self.session.verify = 'acme_client/pebble.minica.pem' if verify is None else verify
      self.dirUrl = dirUrl
      self.serverDir: ServerDirectory = None
      self.nonce = None
      self.httpServer = httpServer
      self.dnsServer = dnsServer
      self.getDirectory()
      
   # 7.1.1
   def getDirectory(self) -> None:
      r = self.session.get(self.dirUrl)
      if r.status_code != 200:
         raise ACMEClientError(f'getDirectory received status code {self.parseResponse(r)}')
      self.serverDir = ServerDirectory(r.json())
      
   # 7.2
   def addNonce(self, response: requests.Response) -> None:
      if not (self.REPLAY_NONCE in response.headers):
         raise ACMEClientError(f'addNonce nonce not found in response\n{self.parseResponse(response)}')
      # store nonce in its base64url encoded form as a str
      # only try decoding to detect invalid nonce as required in 6.5.1
      self.nonce = response.headers[self.REPLAY_NONCE]
      try:
         JWS.base64UrlDec(self.nonce.encode())
      except DecodeError as e:
         badNonce = self.nonce
         self.nonce = None
         raise ACMEClientError(f'getNewNonce received bad nonce: {badNonce}')

   def getNewNonce(self) -> None:
      if self.serverDir is None:
         self.getDirectory()
      r = self.session.head(self.serverDir.newNonce)
      if r.status_code != 200:
         raise ACMEClientError(f'getNewNonce received status code {self.parseResponse(r)}')
      # store nonce in its base64url encoded form as a str
      # only try decoding to detect invalid nonce as required in 6.5.1
      self.addNonce(r)

   def post(self, url: str, data: str, headers: dict = {}) -> requests.Response:
      headers.setdefault('Content-Type', self.APP_JOSE_JSON)
      r = self.session.post(url, data, headers=headers)
      return r
   
   def composeAndSend(self, payload: str, url: str) -> requests.Response:
      if self.nonce is None:
         self.getNewNonce()
      jws = JWS(payload, url, self.nonce)
      self.nonce = None
      msg = self.jose.toJSON(jws)
      r = self.post(url, msg)
      # print(f'Received Nonce: {r.headers[self.REPLAY_NONCE]}')
      self.addNonce(r)
      return r
   
   def sendWithRetry(self, payload: str, url: str, statusCodes: list[int])  -> requests.Response:
      while True:
         r = self.composeAndSend(payload, url)
         if r.status_code in statusCodes:
            return r
         if (r.status_code == 400) and ('urn:ietf:params:acme:error:badNonce' in r.text):
            continue
         raise ACMEClientError(f'sendWithRetry received status code {self.parseResponse(r)}')
   
   # Create new account (7.3)
   def newAccount(self) -> None:
      # clear current account if it already exists.
      self.jose.kid = None
      payload = {
         'termsOfServiceAgreed': True,
         # 'contact': ['nobody@nowhere.ch']
      }
      r = self.sendWithRetry(json.dumps(payload), self.serverDir.newAccount, [201, 200])
      if not ('Location' in r.headers):
         raise ACMEClientError(f'newAccount account link not found\n{self.parseResponse(r)}')
      self.jose.kid = r.headers['Location']
      # TODO: store the orders link somewhere? account object (7.1.2)?
      # r.json()["orders"]

   # TODO: handle error due to change of term of service (7.3.3)
   
   # change account key (7.3.5)
   def changeKey(self) -> None:
      newJose = JOSE()
      innerPayload = {
         'account': self.jose.kid,
         'oldKey': self.jose.jwk
      }
      innerJws = JWS(json.dumps(innerPayload), self.serverDir.keyChange)
      outerPayload = newJose.toJSON(innerJws)
      r = self.sendWithRetry(outerPayload, self.serverDir.keyChange, [200])
      newJose.kid = self.jose.kid
      self.jose = newJose

   # 7.3.6
   def deactivateAccount(self):
      payload = {
         'status': 'deactivated'
      }
      r = self.sendWithRetry(json.dumps(payload), self.jose.kid, [200])
      self.jose.kid = None

   # 7.4
   def applyCertificate(self, domainNames: list[str], challengeType: str = 'http-01') -> tuple[rsa.RSAPrivateKey, str]:
      payload = {
         'identifiers': [
            {"type": "dns", "value": d} for d in domainNames
         ]
      }
      r = self.sendWithRetry(json.dumps(payload), self.serverDir.newOrder, [201])
      if not ('Location' in r.headers):
         raise ACMEClientError(f'applyCertificate order location missing in response\n{self.parseResponse(r)}')
      location = r.headers['Location']
      orderObj = r.json()
      if (not 'identifiers' in orderObj):
         raise ACMEClientError(f'applyCertificate key "identifiers" missing in response\n{self.parseResponse(r)}')
      if (not 'authorizations' in orderObj):
         raise ACMEClientError(f'applyCertificate key "authorizations" missing in response\n{self.parseResponse(r)}')
      if (not 'finalize' in orderObj):
         raise ACMEClientError(f'applyCertificate key "finalize" missing in response\n{self.parseResponse(r)}')
      orders = ACMEOrder(location, orderObj)
      self.getAuthorization(orders, challengeType)
      # post to finalize
      key = rsa.generate_private_key(self.RSA_PUBLIC_EXPONENT, self.RSA_BIT_LEN)
      csr = x509.CertificateSigningRequestBuilder().subject_name(x509.Name([
         x509.NameAttribute(x509.NameOID.COMMON_NAME, domainNames[0]),
      ])).add_extension(
         x509.SubjectAlternativeName([x509.DNSName(n) for n in domainNames]),
         critical=False).sign(key, hashes.SHA256())
      # return (key, csr.public_bytes(serialization.Encoding.DER), orders)
      cert = self.downloadCertificate(orders, csr.public_bytes(serialization.Encoding.DER))
      return (key, cert)

   # def finalize(self, orders: ACMEOrder, csr: bytes):
   #    # finalize (7.4)
   #    payload = {
   #       'csr': JWS.base64UrlEnc(csr).decode()
   #    }
   #    r = self.sendWithRetry(json.dumps(payload), orders.finalize, [200])
   #    # print(self.parseResponse(r))
   #    return r
   
   # 7.4.2
   def downloadCertificate(self, orders: ACMEOrder, csr: bytes) -> str:
      # finalize (7.4)
      payload = {
         'csr': JWS.base64UrlEnc(csr).decode()
      }
      r = self.sendWithRetry(json.dumps(payload), orders.finalize, [200])
      # r = self.finalize(orders, csr)
      d = r.json()
      while not 'certificate' in d:
         r = self.sendWithRetry('', orders.location, [200])
         d = r.json()
      cert = d['certificate']
      # download (7.4.2)
      # TODO: Accept: application/pem-certificate-chain
      # TODO: "alternate" certificate chain
      r = self.sendWithRetry('', cert, [200])
      return r.text

   # 7.5
   def getAuthorization(self, orders: ACMEOrder, challengeType: str = 'http-01'):
      # TODO: multi-thread?
      # TODO: Seems wildcard cannot be verified with http-01
      for authUrl in orders.authorizations:
         r = self.sendWithRetry('', authUrl, [200])
         auth = r.json()
         domainName = auth['identifier']['value']
         if not ((domainName in orders.domainNames) or (f'*.{domainName}' in orders.domainNames)):
            raise ACMEClientError(f'getAuthorization unknown domain name {domainName} in Authorization\n{self.parseResponse(r)}')
         # print(f'domain name: {domainName}')
         # print(auth)
         # print(self.parseResponse(r))
         challenge = None
         for c in auth['challenges']:
            if c['type'] == challengeType:
               challenge = (c['url'], c['token'], domainName)
               break
         if challenge is None:
            raise ACMEClientError(f'getAuthorization challenge type {challengeType} not found\n{self.parseResponse(r)}')
         self.respondChallenge(challenge, challengeType)
         self.pollValidation(authUrl)

   # 7.5.1
   def respondChallenge(self, challenge: tuple[str, str, str], challengeType: str):
      url, token, domainName = challenge
      keyAuthorization = token+'.'+self.jose.thumbprint().decode()
      if challengeType == 'http-01':
         self.httpServer.setToken(token, keyAuthorization)
      else:
         print('Setting Token:', domainName, JWS.base64UrlEnc(SHA256.new(keyAuthorization.encode()).digest()))
         self.dnsServer.setToken(domainName, JWS.base64UrlEnc(SHA256.new(keyAuthorization.encode()).digest()))
      # inform server of completion of challenge
      payload = {}
      r = self.sendWithRetry(json.dumps(payload), url, [200])
      
   # 7.5.1
   def pollValidation(self, url: str):
      while True:
         r = self.sendWithRetry('', url, [200])
         if r.json()['status'] == 'valid':
            break
         if r.json()['status'] != 'pending':
            raise ACMEClientError(f'pollValidation challenge validation failed {self.parseResponse(r)}')
         # TODO: follow Retry-After
         time.sleep(1)

   # 7.5.2
   def deactivateAuthorization(self, url: str):
      payload = {
         'status': 'deactivated'
      }
      r = self.sendWithRetry(json.dumps(payload), url, [200])
      
   # 7.6
   def revokeCertification(self, certChain: str):
      c = x509.load_pem_x509_certificate(certChain.encode()) 
      payload = {
         'certificate': JWS.base64UrlEnc(c.public_bytes(serialization.Encoding.DER)).decode()
      }
      r = self.sendWithRetry(json.dumps(payload), self.serverDir.revokeCert, [200])
      # print(self.parseResponse(r))
   
   @staticmethod
   def parseResponse(r: requests.Response) -> str:
      return f'{r.status_code} {r.reason}\n{r.headers}\n\n{r.text}\n'