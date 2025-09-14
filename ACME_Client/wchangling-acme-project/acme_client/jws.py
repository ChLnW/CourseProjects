import json
import base64
from Crypto.Hash import SHA256
from Crypto.PublicKey import ECC
from Crypto.Signature import DSS
from typing import Optional
from binascii import Error as DecodeError
# from typing import Union

# class BadSignatureJOSE(Exception):
#     """Generic ACME error."""

class JWS:
   def __init__(self, payload: str, url: str, nonce: Optional[str] = None):
      self.protected = {'alg': 'ES256', 'url': url}
      # nonce may not exist when crafting inner payload for key change (7.3.5)
      if nonce is not None:
         self.protected["nonce"] = nonce
      self.payload = payload

   @staticmethod
   def base64UrlEnc(s: bytes) -> bytes:
      return base64.urlsafe_b64encode(s).strip(b'=')
   
   @staticmethod
   def base64UrlDec(s: bytes) -> bytes:
      # as required in 6.1: Encoded values that include trailing '=' 
      # characters MUST be rejected as improperly encoded. 
      if '=' in s.decode():
         raise DecodeError
      return base64.urlsafe_b64decode(s + ((4 - (len(s)%4)) * b'='))
   
class JOSE:
   def __init__(self, priKey: Optional[ECC.EccKey] = None):
      self._prikey = ECC.generate(curve='p256') if priKey is None else priKey
      self._signer = DSS.new(self._prikey, 'fips-186-3')
      self.jwk = JOSE.encodePubKey(self._prikey)
      self.kid = None
   
   # def generateSigningKey(self) -> None:
   #    self._priKey = ECC.generate(curve='p256')
   #    self._signer = DSS.new(self._prikey, 'fips-186-3')
   #    self.jwk = JOSE.encodePubKey(self._prikey)

   # @staticmethod
   # def encodePubKey(prikey: ECC.EccKey) -> str:
   #    res = {
   #       'kty': 'EC',
   #       'crv': 'P-256',
   #       'x': JWS.base64UrlEnc(int(prikey.public_key().pointQ.x).to_bytes(32, 'big')).decode(),
   #       'y': JWS.base64UrlEnc(int(prikey.public_key().pointQ.y).to_bytes(32, 'big')).decode()
   #    }
   #    return json.dumps(res)
   
   def thumbprint(self) -> bytes:
      h = SHA256.new(json.dumps(self.jwk, separators=(',', ':')).encode())
      return JWS.base64UrlEnc(h.digest())
   
   @staticmethod
   def encodePubKey(prikey: ECC.EccKey) -> dict:
      res = {
         'crv': 'P-256',
         'kty': 'EC',
         'x': JWS.base64UrlEnc(int(prikey.public_key().pointQ.x).to_bytes(32, 'big')).decode(),
         'y': JWS.base64UrlEnc(int(prikey.public_key().pointQ.y).to_bytes(32, 'big')).decode()
      }
      return res

   def sign(self, jws: bytes) -> bytes:
      h = SHA256.new(jws)
      return self._signer.sign(h)

   def toJSON(self, jws: JWS) -> str:
      if self.kid is None:
         jws.protected['jwk'] = self.jwk
      else:
         jws.protected['kid'] = self.kid
      f = lambda x: jws.base64UrlEnc(json.dumps(x).encode())
      protectedB64 = f(jws.protected)
      payloadB64 = jws.base64UrlEnc(jws.payload.encode())
      jwsSigInput = protectedB64 + b'.' + payloadB64
      signature = JWS.base64UrlEnc(self.sign(jwsSigInput)).decode()
      res = {
         'protected': protectedB64.decode(),
         'payload': payloadB64.decode(),
         'signature': signature
      }
      return json.dumps(res)
   
   # def fromJSON(self, payload: Union[bytes, str]) -> None:
   #    obj = json.loads(payload)
   #    f = lambda k: json.loads(self.base64UrlDec(obj[k].encode()))
   #    self.protected = f('protected')
   #    self.payload = f('payload')
      # TODO: signature parsing
      # self.signature = f('signature')