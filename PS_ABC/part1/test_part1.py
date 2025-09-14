import pytest
from credential import *
from stroll import *

def test_key_generation():
    """ Test that sk, pk pairs are generated correctly """

    sk, pk = generate_key([str(i).encode() for i in range(10)])

    g1 = pk.g1
    g2 = pk.g2

    assert sk.X1 == g1 ** sk.x
    assert pk.X2 == g2 ** sk.x

    assert all(g1 ** y == Y for y, Y in zip(sk.y, pk.Y1))
    assert all(g2 ** y == Y for y, Y in zip(sk.y, pk.Y2))

def test_sign_verify():
    """ Test that signing and verification works """

    attributes = [b'0', b'1', b'2', b'3', b'4']
    sk, pk = generate_key(attributes)

    sig = sign(sk, attributes)
    signature_valid = verify(pk, sig, attributes)

    assert signature_valid


def test_sign_verify_failure():
    """ Test that signing and verification works """

    attributes1 = [b'0', b'1', b'2', b'3', b'4']
    attributes2 = [b'0', b'1', b'2', b'3', b'5']
    sk, pk = generate_key(attributes1)

    sig = sign(sk, attributes1)
    signature_valid = verify(pk, sig, attributes2)

    assert signature_valid == False

def test_ABC():
    """ Test a regular run of the protocol using ABC's """
    #### SETUP
    user_attrs = {0: b'0', 2: b'2', 4: b'4'}
    issuer_attrs = {1: b'1', 3: b'3'}
    # attributes = user_attrs | issuer_attrs
    attributes = [str(i).encode() for i in range(5)]

    sk, pk = generate_key(attributes)

    #### Issuance Protocol

    # 2. User input. The user takes as input the issuer’s public key as well as user attributes values.
    user = User(pk)

    # 3. Issuer input. The issuer takes as input its private key and public key as well as attributes determined by the issuer.
    issuer = Issuer(sk, pk)

    # 4. User commitment. User commits to the attributes they want to include in the credential and computes a commitment.
    issue_request = user.create_issue_request(user_attrs)

    # 5. Issuer signing. Issuer verifies the PK with respect to commitment C and completes the signature.
    blind_signature = issuer.sign_issue_request(issue_request, issuer_attrs)

    # 6. Unblinding signature. User receives the signature and unblinds it to receive the final signature.
    anonymous_credential = user.obtain_credential(blind_signature)

    #### Showing Protocol
    attrs_to_show = [b'0', b'3']  # names of attributes
    disclosure_proof = user.create_disclosure_proof(attrs_to_show, b'message')

    disclosure_proof_valid = verify_disclosure_proof(pk, disclosure_proof, b'message')

    assert disclosure_proof_valid

def test_ABC_hidden_attribute_tampering():
    """ This test verifies that the user can't change a hidden attribute after it has been signed """

    #### SETUP
    user_attrs = {0: b'0', 2: b'2', 4: b'4'}
    issuer_attrs = {1: b'1', 3: b'3'}
    # attributes = user_attrs | issuer_attrs
    attributes = [str(i).encode() for i in range(5)]

    sk, pk = generate_key(attributes)

    #### Issuance Protocol

    # 2. User input. The user takes as input the issuer’s public key as well as user attributes values.
    user = User(pk)

    # 3. Issuer input. The issuer takes as input its private key and public key as well as attributes determined by the issuer.
    issuer = Issuer(sk, pk)

    # 4. User commitment. User commits to the attributes they want to include in the credential and computes a commitment.
    issue_request = user.create_issue_request(user_attrs)

    # 5. Issuer signing. Issuer verifies the PK with respect to commitment C and completes the signature.
    blind_signature = issuer.sign_issue_request(issue_request, issuer_attrs)

    # 6. Unblinding signature. User receives the signature and unblinds it to receive the final signature.
    anonymous_credential = user.obtain_credential(blind_signature)

    #### Showing Protocol
    attrs_to_show = [b'0', b'3']  # names of attributes
    user.attributes[1] = b'tampering'
    disclosure_proof = user.create_disclosure_proof(attrs_to_show, b'message')

    disclosure_proof_valid = verify_disclosure_proof(pk, disclosure_proof, b'message')

    assert not disclosure_proof_valid


def test_ABC_disclosed_user_attribute_tampering():
    """ This test verifies that the user can't change a disclosed user defined attribute after it has been signed """

    #### SETUP
    user_attrs = {0: b'0', 2: b'2', 4: b'4'}
    issuer_attrs = {1: b'1', 3: b'3'}
    # attributes = user_attrs | issuer_attrs
    attributes = [str(i).encode() for i in range(5)]

    sk, pk = generate_key(attributes)

    #### Issuance Protocol

    # 2. User input. The user takes as input the issuer’s public key as well as user attributes values.
    user = User(pk)

    # 3. Issuer input. The issuer takes as input its private key and public key as well as attributes determined by the issuer.
    issuer = Issuer(sk, pk)

    # 4. User commitment. User commits to the attributes they want to include in the credential and computes a commitment.
    issue_request = user.create_issue_request(user_attrs)

    # 5. Issuer signing. Issuer verifies the PK with respect to commitment C and completes the signature.
    blind_signature = issuer.sign_issue_request(issue_request, issuer_attrs)

    # 6. Unblinding signature. User receives the signature and unblinds it to receive the final signature.
    # PS. The credential is stored within the User object so the variable here goes unused.
    anonymous_credential = user.obtain_credential(blind_signature)

    #### Showing Protocol
    attrs_to_show = [b'0', b'3']  # names of attributes
    user.attributes[0] = b'tampering'
    disclosure_proof = user.create_disclosure_proof(attrs_to_show, b'message')

    disclosure_proof_valid = verify_disclosure_proof(pk, disclosure_proof, b'message')

    assert not disclosure_proof_valid


def test_ABC_issuer_attribute_tampering():
    """ This test verifies that the user can't change a disclosed issuer defined attribute after it has been signed """

    #### SETUP
    user_attrs = {0: b'0', 2: b'2', 4: b'4'}
    issuer_attrs = {1: b'1', 3: b'3'}
    # attributes = user_attrs | issuer_attrs
    attributes = [str(i).encode() for i in range(5)]

    sk, pk = generate_key(attributes)

    #### Issuance Protocol

    # 2. User input. The user takes as input the issuer’s public key as well as user attributes values.
    user = User(pk)

    # 3. Issuer input. The issuer takes as input its private key and public key as well as attributes determined by the issuer.
    issuer = Issuer(sk, pk)

    # 4. User commitment. User commits to the attributes they want to include in the credential and computes a commitment.
    issue_request = user.create_issue_request(user_attrs)

    # 5. Issuer signing. Issuer verifies the PK with respect to commitment C and completes the signature.
    blind_signature = issuer.sign_issue_request(issue_request, issuer_attrs)

    # 6. Unblinding signature. User receives the signature and unblinds it to receive the final signature.
    # PS. The credential is stored within the User object so the variable here goes unused.
    anonymous_credential = user.obtain_credential(blind_signature)

    #### Showing Protocol
    attrs_to_show = [b'0', b'3']  # names of attributes
    user.attributes[3] = b'tampering'
    disclosure_proof = user.create_disclosure_proof(attrs_to_show, b'message')

    disclosure_proof_valid = verify_disclosure_proof(pk, disclosure_proof, b'message')

    assert not disclosure_proof_valid

def test_ABC_tamper_message():
    """ Test a regular run of the protocol using ABC's """
    #### SETUP
    user_attrs = {0: b'0', 2: b'2', 4: b'4'}
    issuer_attrs = {1: b'1', 3: b'3'}
    # attributes = user_attrs | issuer_attrs
    attributes = [str(i).encode() for i in range(5)]

    sk, pk = generate_key(attributes)

    #### Issuance Protocol

    # 2. User input. The user takes as input the issuer’s public key as well as user attributes values.
    user = User(pk)

    # 3. Issuer input. The issuer takes as input its private key and public key as well as attributes determined by the issuer.
    issuer = Issuer(sk, pk)

    # 4. User commitment. User commits to the attributes they want to include in the credential and computes a commitment.
    issue_request = user.create_issue_request(user_attrs)

    # 5. Issuer signing. Issuer verifies the PK with respect to commitment C and completes the signature.
    blind_signature = issuer.sign_issue_request(issue_request, issuer_attrs)

    # 6. Unblinding signature. User receives the signature and unblinds it to receive the final signature.
    anonymous_credential = user.obtain_credential(blind_signature)

    #### Showing Protocol
    attrs_to_show = [b'0', b'3']  # names of attributes
    disclosure_proof = user.create_disclosure_proof(attrs_to_show, b'message1')

    disclosure_proof_valid = verify_disclosure_proof(pk, disclosure_proof, b'message2')

    assert not disclosure_proof_valid

def test_stroll():
    attributes = ['a', 'b', 'c', 'd', 'e']
    user_attributes = ['d', 'b']
    username = 'somebody'
    message = b'Demacia!'
    sk, pk = Server.generate_ca(attributes)

    server = Server()
    client = Client()

    # registration
    request, state = client.prepare_registration(pk, username, user_attributes)
    response = server.process_registration(sk, pk, request, username, user_attributes)
    credentials = client.process_registration_response(pk, response, state)

    # request subscribed service
    revealed_attributes = ['b', 'd']
    signature = client.sign_request(pk, credentials, message, revealed_attributes)
    assert server.check_request_signature(pk, message, revealed_attributes, signature)

def test_stroll_no_subscription():
    attributes = ['a', 'b', 'c', 'd', 'e']
    user_attributes = ['d', 'b']
    username = 'somebody'
    message = b'Demacia!'
    sk, pk = Server.generate_ca(attributes)

    server = Server()
    client = Client()

    # registration
    request, state = client.prepare_registration(pk, username, user_attributes)
    response = server.process_registration(sk, pk, request, username, user_attributes)
    credentials = client.process_registration_response(pk, response, state)

    # request subscribed service
    revealed_attributes = ['c']
    signature = client.sign_request(pk, credentials, message, revealed_attributes)
    assert not server.check_request_signature(pk, message, revealed_attributes, signature)

def test_stroll_no_subscription_partial():
    attributes = ['a', 'b', 'c', 'd', 'e']
    user_attributes = ['d', 'b']
    username = 'somebody'
    message = b'Demacia!'
    sk, pk = Server.generate_ca(attributes)

    server = Server()
    client = Client()

    # registration
    request, state = client.prepare_registration(pk, username, user_attributes)
    response = server.process_registration(sk, pk, request, username, user_attributes)
    credentials = client.process_registration_response(pk, response, state)

    # request subscribed service
    revealed_attributes = ['d','c']
    signature = client.sign_request(pk, credentials, message, revealed_attributes)
    assert not server.check_request_signature(pk, message, revealed_attributes, signature)

def test_stroll_tamper_message():
    attributes = ['a', 'b', 'c', 'd', 'e']
    user_attributes = ['d', 'b']
    username = 'somebody'
    message = b'Demacia!'
    sk, pk = Server.generate_ca(attributes)

    server = Server()
    client = Client()

    # registration
    request, state = client.prepare_registration(pk, username, user_attributes)
    response = server.process_registration(sk, pk, request, username, user_attributes)
    credentials = client.process_registration_response(pk, response, state)

    # request subscribed service
    revealed_attributes = ['b', 'd']
    signature = client.sign_request(pk, credentials, message, revealed_attributes)
    message2 = b'Noxus!'
    assert not server.check_request_signature(pk, message2, revealed_attributes, signature)

# TODO: malicious server