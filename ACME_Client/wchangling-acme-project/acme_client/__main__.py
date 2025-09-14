from http.server import HTTPServer
from threading import Thread
from dnslib.server import DNSServer

from acme_client.http01_handler import *
from acme_client.dns01_handler import *
from acme_client.client import *

import argparse

import ssl

def get_ssl_context(certfile, keyfile):
    context = ssl.SSLContext(ssl.PROTOCOL_TLSv1_2)
    context.load_cert_chain(certfile, keyfile)
    context.set_ciphers("@SECLEVEL=1:ALL")
    return context

if __name__ == "__main__":
    # Hint: You may want to start by parsing command line arguments and
    # perform some sanity checks first. The built-in `argparse` library will suffice.
    parser = argparse.ArgumentParser()
    parser.add_argument("type", help="challenge type: {dns01 | http01}")
    parser.add_argument("--dir", help="directory URL of the ACME server")
    parser.add_argument("--record", help="IPv4 address which must be returned by your DNS server for all A-record queries")
    parser.add_argument("--domain", help="domain names", action='append')
    parser.add_argument("--revoke", help="revoke after issued", action='store_true')
    args = parser.parse_args()

    # http01_server = HTTPServer(("0.0.0.0", 5002), HTTP01Handler)
    resolver = FixedResolver(f". 60 IN A {args.record}")
    http01_server = HTTP01Server((args.record, 5002), HTTP01Handler)
    dns01_server = DNS01Server(resolver, port=10053, address=args.record)

    http01_thread = Thread(target = http01_server.run)
    dns01_thread = Thread(target = dns01_server.run)
    http01_thread.daemon = True
    dns01_thread.daemon = True

    http01_thread.start()
    dns01_thread.start()

    certServer = HTTP01Server((args.record, 5001), HTTPCertificateHandler)
    
    terminateServer = TerminateServer((args.record, 5003), TerminateHandler)
    terminateThread = Thread(target = terminateServer.run)

    # Your code should go here
    c = Client(dirUrl=args.dir, httpServer=http01_server, dnsServer=dns01_server)
    c.newAccount()
    challengeType = 'http-01' if args.type == 'http01' else 'dns-01'
    key, cert = c.applyCertificate(args.domain, challengeType)
    
    with open('acme_client/key.pem', 'wb') as f:
        f.write(key.private_bytes(serialization.Encoding.PEM, 
                                  format=serialization.PrivateFormat.TraditionalOpenSSL, 
                                  encryption_algorithm=serialization.NoEncryption()))
    with open('acme_client/cert.pem', 'wb') as f:
        f.write(cert.encode())
    context = get_ssl_context("acme_client/cert.pem", "acme_client/key.pem")
    certServer.socket = context.wrap_socket(certServer.socket, server_side=True)
    certThread = Thread(target = certServer.run)
    certThread.daemon = True
    certServer.setToken('cert', cert)
    
    
    
    if args.revoke:
        c.revokeCertification(cert)

    terminateServer.terminate.wait()
    
    terminateServer.shutdown()
    http01_server.shutdown()
    dns01_server.stop()
    certServer.shutdown()
    
    terminateThread.join()
    http01_thread.join()
    dns01_thread.join()
    certThread.join()