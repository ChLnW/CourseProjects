"""
Skeleton credential module for implementing PS credentials

The goal of this skeleton is to help you implementing PS credentials. Following
this API is not mandatory and you can change it as you see fit. This skeleton
only provides major functionality that you will need.

You will likely have to define more functions and/or classes. In particular, to
maintain clean code, we recommend to use classes for things that you want to
send between parties. You can then use `jsonpickle` serialization to convert
these classes to byte arrays (as expected by the other classes) and back again.

We also avoided the use of classes in this template so that the code more closely
resembles the original scheme definition. However, you are free to restructure
the functions provided to resemble a more object-oriented interface.
"""

from typing import Any, List, Tuple, Dict, Union

# from astroid import Raise

from serialization import jsonpickle
from functools import reduce
from petrelic.multiplicative.pairing import G1, G2, GT, G1Element, G2Element, GTElement
from petrelic.bn import Bn
from hashlib import sha256


# Type hint aliases
# Feel free to change them as you see fit.
# Maybe at the end, you will not need aliases at all!
#SecretKey = Any
#PublicKey = Any
Signature = Tuple[G1Element, G1Element]
AttributeValue = bytes
AttributeName = bytes
AttributeMap = Dict[int, AttributeValue]
# IssueRequest = Tuple[G1Element, tuple[int, list[Bn]]]
#BlindSignature = Any
# AnonymousCredential = Tuple[Signature, List[Attribute]]
# DisclosureProof = Any
AttributeIndex = Dict[AttributeName, int]



######################
## SIGNATURE SCHEME ##
######################

class PublicKey:
    def __init__(self, 
                 g: G1Element, 
                 Y: List[G1Element], 
                 g_tilde: G2Element, 
                 X_tilde: G2Element, 
                 Y_tilde: List[G2Element],
                 attributes: List[AttributeName]):
        self.g1 = g              # Generator for G1
        self.Y1 = Y              # Public keys of y for G1
        self.g2 = g_tilde  # Generator for G2
        self.X2 = X_tilde  # Public key of x for G2
        self.Y2 = Y_tilde  # Public keys of y for G2
        # the order of attributes must be enforced, otherwise issuer can distinguish users by order of attributes
        self.attributes_name = attributes  

    def __eq__(self, value):
        return (self.g1 == value.g1 and 
                self.Y1 == value.Y1 and 
                self.g2 == value.g2 and
                self.X2 == value.X2 and
                self.Y2 == value.Y2 and
                self.attributes_name == value.attributes_name)
    
    def attribute_index(self) -> AttributeIndex:
        return {a:i for i, a in enumerate(self.attributes_name)}


class SecretKey:
    def __init__(self, 
                 x: Bn, 
                 X: G1Element, 
                 y: List[Bn],
                 attributes: List[AttributeName]):
        self.x  = x  # Private key x
        self.X1 = X  # Public key x for group G1
        self.y  = y  # Private keys for attributes
        # keep a copy of same attribute mapping in sk as well
        self.attributes = attributes  

class IssueRequest:
    def __init__(self, commitment: G1Element, c: int, ss: List[Bn]):
        self.commitment = commitment            # Commitment to user defined attributes
        self.c = c
        self.ss = ss

class BlindSignature:
    def __init__(self, sig1: G1Element, sig2: G1Element, issuer_attributes: AttributeMap):
        self.sig1 = sig1            # g**u
        self.sig2 = sig2            # Signature over attributes
        self.issuer_attributes = issuer_attributes  # Issuer returns attributes chosen by them

class AnonymousCredential:
    def __init__(self, sig1: G1Element, sig2: G1Element, attributes: List[AttributeValue]):
        self.sig1 = sig1
        self.sig2 = sig2
        self.attributes = attributes

class DisclosureProof:
    def __init__(self, sig1: G1Element, sig2: G1Element, 
                 disclosed_attributes: AttributeMap, 
                 c: int, ss: List[Bn]):
        self.sig1 = sig1
        self.sig2 = sig2
        self.disclosed_attributes = disclosed_attributes
        self.c = c
        self.ss = ss
        

def sampleNon0(p: Bn) -> Bn:
    # generate a random number in [1, p-1]
    # currently random() itself doesn't 0 either,
    # but may change in the future
    r = p.random()
    while r == 0:
        r = p.random()
    return r

def generate_key(
        attributes: List[AttributeName]
    ) -> Tuple[SecretKey, PublicKey]:
    """ Generate signer key pair """

    L = len(attributes)

    # order of the groups seems to have been fixed
    p = G1.order()

    # Original PS signature paper does not require non-zero x 
    # But here x being zero leaks X in sk, which seems undesirable
    x = sampleNon0(p)
    # g1, g2 respectively refer to g and g_tilde in the handout
    # since G1, G2 are of prime order (therefore cyclic)
    # any non-identity member would be a generator of the group
    g1 = G1.generator() ** sampleNon0(p)
    g2 = G2.generator() ** sampleNon0(p)
    # similarly X1, X2 refer to X and X_tilde in the handout
    X1 = g1 ** x
    X2 = g2 ** x
    # y_i cannot be 0, otherwise enables forgery of the corresponding entry of messages
    y = [sampleNon0(p) for _ in range(L)]
    # similarly Y1, Y2 refer to Y and Y_tilde in the handout
    Y1 = [g1 ** elem for elem in y]
    Y2 = [g2 ** elem for elem in y]

    pk = PublicKey(g1, Y1, g2, X2, Y2, attributes)
    sk = SecretKey(x, X1, y, attributes)

    return sk, pk

# cannot use hash_to_point as 
# log|G| ~ 255, sha256 about right
def msg_to_int(m: bytes):
    """ Hash the message bytest and return as int. Hashing is done to accomodate for messages of any length. """
    return int(sha256(m).hexdigest(), 16)

def sign(
        sk: SecretKey,
        msgs: List[bytes]
    ) -> Signature:
    """ Sign the vector of messages `msgs` """

    assert len(sk.y) == len(msgs)
    h = G1.generator() ** sampleNon0(G1.order())
    # TODO: use G1.prod / G1.wprod
    n = sk.x + sum(yi * msg_to_int(mi) for yi, mi in zip(sk.y, msgs))
    sig = h ** n
    return h, sig

def verify(
        pk: PublicKey,
        signature: Signature,
        msgs: List[bytes]
    ) -> bool:
    """ Verify the signature on a vector of messages """
    sig1, sig2 = signature

    # Assert that sig1 is not the unity element of G1
    assert not sig1.eq(G1.unity())

    # Calculate the product for G2
    # TODO: use G2.prod / G2.wprod
    left_product = pk.X2 * reduce(lambda x,y: x*y, [Yi ** msg_to_int(mi) for Yi, mi in zip(pk.Y2, msgs)])

    # Check the equality of pairings: e(sigma1, product) = e(sigma2, g_tilde)
    return sig1.pair(left_product).eq(sig2.pair(pk.g2))



#################################
## ATTRIBUTE-BASED CREDENTIALS ##
#################################

## ISSUANCE PROTOCOL ##

def hash_PK(g, Y: List, C, R, message: Union[bytes, None] = None) -> int:
    m = str(g)
    for Yi in Y:
        m += str(Yi)
    m += str(C)
    m += str(R)
    return msg_to_int(m.encode() + (b'' if message is None else message))

# Based on commitment scheme from: https://www.zkdocs.com/docs/zkdocs/commitments/pedersen/#proof-of-knowledge-of-secret
def PK(g: Union[G1Element, GTElement],
       t: Bn, 
       Y: Union[List[G1Element], List[GTElement]],
       A: List[int],
       message: Union[bytes, None] = None
    ) -> Tuple[Union[G1Element, GTElement], int, List[Bn]]:
    # dim(r) = dim(x) + dim(t)
    mul = lambda x,y: x*y
    p = G1.order()
    rY = [sampleNon0(p) for _ in range(len(Y))]
    rg = sampleNon0(p)
    R = reduce(mul, [Yi ** ri for Yi, ri in zip(Y, rY)], g ** rg)
    C = reduce(mul, [Yi ** ai for Yi, ai in zip(Y, A)], g ** t)
    # c = Hash(g||Y1||C||R)
    c = hash_PK(g, Y, C, R, message)
    ss = [rg + c*t] + [ri + c * ai for ai, ri in zip(A, rY)]
    return C, c, ss


def verifyPK(C: Union[G1Element, GTElement], 
             c: int,
             ss: List[int], 
             g: Union[G1Element, GTElement],
             Y: Union[List[G1Element], List[GTElement]],
             message: Union[bytes, None] = None
        ) -> bool:
    # R * C^c = g^ss[0] * prod_i~[1, len(ss)] (Yi^ss[i])
    mul = lambda x,y: x*y
    gs = reduce(mul, [Yi ** si for Yi, si in zip(Y, ss[1:])], g ** ss[0])
    R = gs.div(C ** c)
    c2 = hash_PK(g, Y, C, R, message)
    return c2 == c


class User:
    def __init__(self, pk: PublicKey):
        self.pk = pk
        self.attribute_index = pk.attribute_index()  # attribute name -> attribute index
        self.user_attributes: List[AttributeValue] = [None] * len(pk.attributes_name)  # attribute index -> attribute value
        self.t = None
        self.attributes: List[AttributeValue] = None
        self.signature = None

    def get_attributes(self, issuer_attributes: AttributeMap = None) -> List[AttributeValue]:
        if self.attributes is not None:
            return self.attributes
        assert issuer_attributes is not None
        user_attributes = self.user_attributes
        for i in range(len(user_attributes)):
            if user_attributes[i] is None:
                assert i in issuer_attributes
                user_attributes[i] = issuer_attributes[i]
            else:
                assert i not in issuer_attributes
        self.attributes = user_attributes
        return self.attributes

    def create_issue_request(self, user_attributes: AttributeMap) -> IssueRequest:
        """ Create an issuance request

        This corresponds to the "user commitment" step in the issuance protocol.

        Due to constraints on the API, the client has no notion about which of the 
        attributes are user attributes, hence it will treat any attribute not in 
        user_attributes as client attributes. Implication is any empty mapping 
        shall be explicitly included in user_attributes as well.

        *Warning:* You may need to pass state to the `obtain_credential` function.
        """
        pk = self.pk
        # clear previous states (if any)
        self.t = None
        self.attributes: List[AttributeValue] = None
        self.user_attributes = [None] * len(pk.attributes_name)
        for i,a in user_attributes.items():
            self.user_attributes[i] = a
        # Pick random value
        self.t = sampleNon0(G1.order())
        Y = []
        A = []
        # such a iteration guarantees the order as specified in pk,
        # eliminates ambiguity in the interpretation of ss on Issuer side
        for i, a in enumerate(self.user_attributes):
            if a is not None:
                Y.append(pk.Y1[i])
                A.append(msg_to_int(a))
        C, c, ss = PK(pk.g1, self.t, Y, A)
        return IssueRequest(C, c, ss)
    
    def obtain_credential(self, response: BlindSignature) -> AnonymousCredential:
        """ Derive a credential from the issuer's response

        This corresponds to the "Unblinding signature" step.
        """
        pk = self.pk
        t = self.t
        assert t is not None
        sig1 = response.sig1
        sig2 = response.sig2.div(sig1 ** t)
        sig = (sig1, sig2)
        # get_attributes() asserts the Issuer/User have same view on attribute locations
        issuer_attributes = {int(i): a for i,a in response.issuer_attributes.items()}
        attributes = self.get_attributes(issuer_attributes)
        assert verify(pk, sig, attributes)
        self.signature = sig
        self.t = None
        return AnonymousCredential(sig1, sig2, attributes)
    
    ## SHOWING PROTOCOL ##
    def load_credential(self, credential: AnonymousCredential):
        self.signature = (credential.sig1, credential.sig2)
        self.attributes = credential.attributes

    def create_disclosure_proof(
            # pk: PublicKey,
            # credential: AnonymousCredential,
            self,
            D: List[AttributeName],
            message: bytes
        ) -> DisclosureProof:
        """ Create a disclosure proof """
        sig1, sig2 = self.signature
        attributes = self.attributes
        pk = self.pk
        D = set(D)

        r = sampleNon0(G1.order())
        t = sampleNon0(G1.order())
        random_sig1 = sig1 ** r
        random_sig2 = (sig2 * (sig1 ** t)) ** r

        disclosed_attributes = {}
        Y = []
        A = []
        for i,(n,a) in enumerate(zip(pk.attributes_name, attributes)):
            if n in D:
                disclosed_attributes[i] = a
            else:
                Y.append(random_sig1.pair(pk.Y2[i]))
                A.append(msg_to_int(a))
        
        g = random_sig1.pair(pk.g2)
        
        _, c, ss = PK(g, t, Y, A, message)
        return DisclosureProof(random_sig1, random_sig2, disclosed_attributes, c, ss)


class Issuer:
    def __init__(self, sk: SecretKey, pk: PublicKey):
        self.sk = sk
        self.pk = pk
        self.attribute_index = pk.attribute_index()
        
    def sign_issue_request(self, request: IssueRequest, issuer_attributes: AttributeMap) -> BlindSignature:
        """ Create a signature corresponding to the user's request

        This corresponds to the "Issuer signing" step in the issuance protocol.
        """

        pk = self.pk
        Y_user = []
        # such an iteration guarantees the order as specified in pk,
        # eliminates ambiguity in the interpretation of ss on Issuer side
        for i, y in enumerate(pk.Y1):
            if i not in issuer_attributes:
                Y_user.append(y)
        # length check ensures the user and client at least agree on the number of 
        # user/issuer attributes. (w.r.t. pk)
        assert len(Y_user) == len(request.ss) - 1
        # if user's attribute_index is different from issuer's, Y_user will be
        # wrong and the verification shall fail
        assert verifyPK(request.commitment, request.c, request.ss, pk.g1, Y_user)
        sk = self.sk
        mul = lambda x,y: x*y

        C = request.commitment
        u = sampleNon0(G1.order())

        sig1 = pk.g1 ** u
        sig2 = reduce(mul, [pk.Y1[i] ** msg_to_int(a) for i, a in issuer_attributes.items()], sk.X1 * C) ** u

        return BlindSignature(sig1, sig2, issuer_attributes)


## SHOWING PROTOCOL ##

def verify_disclosure_proof(
        pk: PublicKey,
        disclosure_proof: DisclosureProof,
        message: bytes
    ) -> bool:
    """ Verify the disclosure proof

    Hint: The verifier may also want to retrieve the disclosed attributes
    """
    sig1 = disclosure_proof.sig1
    assert not sig1.eq(G1.unity())
    sig2 = disclosure_proof.sig2
    disclosed_attributes = {int(k):v for k, v in disclosure_proof.disclosed_attributes.items()}
    c = disclosure_proof.c
    ss = disclosure_proof.ss
    Y_H = []
    C = sig2.pair(pk.g2).div(sig1.pair(pk.X2))
    for i in range(len(pk.Y1)):
        if i in disclosed_attributes:
            C = C.div(sig1.pair(pk.Y2[i]) ** msg_to_int(disclosed_attributes[i]))
        else:
            Y_H.append(sig1.pair(pk.Y2[i]))

    return verifyPK(C, c, ss, sig1.pair(pk.g2), Y_H, message)