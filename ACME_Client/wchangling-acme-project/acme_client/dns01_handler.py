from dnslib import RR,DNSRecord,TXT
from dnslib.server import BaseResolver,DNSServer,DNSHandler
import copy
from typing import Any

class DNS01Handler:
     def resolve(self, request, handler):
         reply = request.reply()
         reply.add_answer(*RR.fromZone("example.com 60 A 1.2.3.4"))
         return reply

# https://github.com/paulc/dnslib/blob/master/dnslib/fixedresolver.py
class FixedResolver(BaseResolver):
    """
        Respond with fixed response to all requests
    """
    def __init__(self,zone):
        # Parse RRs
        self.rr = RR.fromZone(zone)[-1]
        # self.tokens: dict[str: str] = {}

    def resolve(self, request: DNSRecord, handler: DNSHandler):
        # print(request)
        reply = request.reply(ra=0)
        qname = request.q.qname
        qnameStr = str(qname)
        tokens = handler.server.tokens
        # print(f'\n\n\n{qname}:{type(qname)}\t{request.q.qtype}\t{qnameStr in self.tokens}\t{self.tokens}\n\n\n')
        if (request.q.qtype == 16) and (qnameStr in tokens):
            
            reply.add_answer(RR(qname, 16, 1, 60, 
                                TXT(tokens[str(qname)])))
            return reply
        # Replace labels with request label
        a = copy.copy(self.rr)
        a.rname = qname
        reply.add_answer(a)
        return reply
    
    # def setToken(self, token: str, resource: str):
    #     self.tokens[token] = resource
    
    # def getToken(self, token: str):
    #     return self.tokens[token] if token in self.tokens else None
    

class DNS01Server(DNSServer):
    ACME_CHALLENGE_DOMAIN = '_acme-challenge'
    
    def __init__(self, *args: Any, **kwargs: Any):
        super().__init__(*args, **kwargs)
        self.server.tokens = {}

    def setToken(self, token: str, resource: str):
        self.server.tokens[f'{self.ACME_CHALLENGE_DOMAIN}.{token.rstrip('.')}.'] = resource
    
    def getToken(self, token: str):
        return self.server.tokens[token] if token in self.server.tokens else None

    def run(self):
        try:
            self.server.serve_forever()
        except KeyboardInterrupt:
            pass
        finally:
            # Clean-up server (close socket, etc.)
            self.server.server_close()