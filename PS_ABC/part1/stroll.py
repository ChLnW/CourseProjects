"""
Classes that you need to complete.
"""

from typing import Any, Dict, List, Union, Tuple, Set

# Optional import
from serialization import jsonpickle

from credential import *

# Type aliases
State = Tuple[str, List[str]]


class Server:
    """Server"""


    def __init__(self):
        """
        Server constructor.
        """
        # raise NotImplementedError
        # Attributes: AttributeMap = [0: user_sk, [1,n]: *all_subscriptions, n+1: username]
        self.issuer: Issuer = None


    @staticmethod
    def generate_ca(
            subscriptions: List[str]
        ) -> Tuple[bytes, bytes]:
        """Initializes the credential system. Runs exactly once in the
        beginning. Decides on schemes public parameters and choses a secret key
        for the server.

        Args:
            subscriptions: a list of all valid attributes. Users cannot get a
                credential with a attribute which is not included here.

        Returns:
            tuple containing:
                - server's secret key
                - server's public information
            You are free to design this as you see fit, but the return types
            should be encoded as bytes.
        """
        # raise NotImplementedError
        # Attributes: AttributeMap = [0: user_sk, [1,n]: *all_subscriptions, n+1: username]
        attributes_bytes: List[AttributeName] = [b'_user_sk'] + [s.encode('utf-8') for s in subscriptions] + [b'_username']
        sk, pk = generate_key(attributes_bytes)
        sk_ser = jsonpickle.encode(sk).encode('utf-8')
        pk_ser = jsonpickle.encode(pk).encode('utf-8')
        return sk_ser, pk_ser



    def process_registration(
            self,
            server_sk: bytes,
            server_pk: bytes,
            issuance_request: bytes,
            username: str,
            subscriptions: List[str]
        ) -> bytes:
        """ Registers a new account on the server.

        Args:
            server_sk: the server's secret key (serialized)
            issuance_request: The issuance request (serialized)
            username: username
            subscriptions: attributes


        Return:
            serialized response (the client should be able to build a
                credential with this response).
        """
        # raise NotImplementedError
        pk: PublicKey = jsonpickle.decode(server_pk.decode('utf-8'))
        sk: SecretKey = jsonpickle.decode(server_sk.decode('utf-8'))
        request: IssueRequest = jsonpickle.decode(issuance_request.decode('utf-8'))
        # Attributes: AttributeMap = [0: user_sk, [1,n]: *all_subscriptions, n+1: username]
        self.issuer = Issuer(sk, pk)
        attribute_index = self.issuer.attribute_index
        # Encoding Scheme: sub_name: (sub_name if subscribed else b'')
        L = len(pk.attributes_name)
        issuer_attributes = {i:b'' for i in range(1, L)}
        for s in subscriptions:
            sb = s.encode()
            assert sb != b'_user_sk' and sb != b'_username' and sb in attribute_index
            issuer_attributes[attribute_index[sb]] = sb
        issuer_attributes[L-1] = username.encode()
        
        response = self.issuer.sign_issue_request(request, issuer_attributes)
        return jsonpickle.encode(response).encode('utf-8')


    def check_request_signature(
        self,
        server_pk: bytes,
        message: bytes,
        revealed_attributes: List[str], # saved in DisclosureProof
        signature: bytes
        ) -> bool:
        """ Verify the signature on the location request

        Args:
            server_pk: the server's public key (serialized)
            message: The message to sign
            revealed_attributes: revealed attributes
            signature: user's authorization (serialized)

        Returns:
            whether a signature is valid
        """
        pk: PublicKey = jsonpickle.decode(server_pk.decode('utf-8'))
        signature: DisclosureProof = jsonpickle.decode(signature.decode('utf-8'))
        if len(revealed_attributes) != len(signature.disclosed_attributes):
            return False
        # checks all disclosed attributes are indeed subscribed
        for i, a in signature.disclosed_attributes.items():
            if a != pk.attributes_name[int(i)]:
                return False
        for a in revealed_attributes:
            if a.encode() not in signature.disclosed_attributes.values():
                return False
        return verify_disclosure_proof(pk, signature, message)


class Client:
    """Client"""

    def __init__(self):
        """
        Client constructor.
        """
        self.user_sk = sampleNon0(G1.order())
        self.user: User = None
        # self.subscriptions: Dict[str, int] = None
        # self.credential: AnonymousCredential = None


    def prepare_registration(
            self,
            server_pk: bytes,
            username: str,
            subscriptions: List[str]
        ) -> Tuple[bytes, State]:
        """Prepare a request to register a new account on the server.

        Args:
            server_pk: a server's public key (serialized)
            username: user's name
            subscriptions: user's subscriptions

        Return:
            A tuple containing:
                - an issuance request
                - A private state. You can use state to store and transfer information
                from prepare_registration to proceed_registration_response.
                You need to design the state yourself.
        """
        #raise NotImplementedError
        # self.subscriptions = {s:(i+1) for i, s in enumerate(subscriptions)}
        pk: PublicKey = jsonpickle.decode(server_pk.decode('utf-8'))
        
        user_attributes = {
            0: jsonpickle.encode(self.user_sk).encode('utf-8')
        }
        self.user = User(pk)
        request = self.user.create_issue_request(user_attributes)
        
        request_bytes = jsonpickle.encode(request).encode('utf-8')
        # username and subscriptions are not used by User in ABC as they are issuer-
        # defined attributes, but stored in State to check correspondence between 
        # request and response
        return request_bytes, [username.encode(), [s.encode() for s in subscriptions]]



    def process_registration_response(
            self,
            server_pk: bytes,
            server_response: bytes,
            private_state: State  # saved in user
        ) -> bytes:
        """Process the response from the server.

        Args:
            server_pk a server's public key (serialized)
            server_response: the response from the server (serialized)
            private_state: state from the prepare_registration
            request corresponding to this response

        Return:
            credentials: create an attribute-based credential for the user
        """
        pk: PublicKey = jsonpickle.decode(server_pk.decode('utf-8'))
        assert pk == self.user.pk
        response: BlindSignature = jsonpickle.decode(server_response.decode('utf-8'))
        credential = self.user.obtain_credential(response)
        # check _username matches
        assert credential.attributes[-1] == private_state[0]
        # check requested subscriptions are granted
        for s in private_state[1]:
            assert credential.attributes[self.user.attribute_index[s]] == s
        # Whether other attributes are modifed is not checked, as it shouldn't 
        # matter if the ABC is implemented correctly
        
        return jsonpickle.encode(credential).encode('utf-8')


    def sign_request(
            self,
            server_pk: bytes,
            credentials: bytes,
            message: bytes,
            types: List[str]
        ) -> bytes:
        """Signs the request with the client's credential.

        Arg:
            server_pk: a server's public key (serialized)
            credential: client's credential (serialized)
            message: message to sign
            types: which attributes should be sent along with the request?

        Returns:
            A message's signature (serialized)
        """
        #raise NotImplementedError
        pk: PublicKey = jsonpickle.decode(server_pk.decode('utf-8'))
        cred: AnonymousCredential = jsonpickle.decode(credentials.decode())
        if self.user is None:
            self.user = User(pk)
        self.user.signature = (cred.sig1, cred.sig2)
        self.user.attributes = cred.attributes
        D = [s.encode() for s in types]
        signature = self.user.create_disclosure_proof(D, message)
        return jsonpickle.encode(signature).encode('utf-8')

