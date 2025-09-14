from client import *

hostname =  'abc.cd'
port = 5002

from http.server import HTTPServer
from threading import Thread

from http01_handler import *
from dns01_handler import *

import requests

ip = "0.0.0.0"

http01_server = HTTP01Server((ip, port), HTTP01Handler)
http01_thread = Thread(target = http01_server.serve_forever)
http01_thread.daemon = True
http01_thread.start()

resolver = FixedResolver(f". 60 IN A {ip}")
dns01_server = DNS01Server(resolver, port=10053, address=ip)
dns01_thread = Thread(target = dns01_server.run)
dns01_thread.daemon = True
dns01_thread.start()


# r = requests.get(f'http://{hostname}:5002/')

c = Client(httpServer=http01_server, dnsServer=dns01_server, verify='./pebble.minica.pem')
c.newAccount()
c.newAccount()
c.changeKey()
c.newAccount()
# key, cert = c.applyCertificate([hostname], 'dns-01')

# import dnslib 
# from dns01_handler import *

# record = '0.0.0.0'
# resolver = FixedResolver(f". 60 IN A 1.2.3.4")

# dnss = DNS01Server(resolver, record, 10053)

# dnss.setToken('abc', 'sadfasdfs')

# # q = dnslib.DNSRecord.question('haha.co')
# # dnss.start()
# dnss.start_thread()

# dnss.setToken('abcd', 'sadfasdfs')