import hashlib
from collections import OrderedDict, namedtuple

import boto3
import botocore.session
from botocore.exceptions import ClientError
from botocore.exceptions import ParamValidationError
# from nose.tools import eq_ as eq
# from nose.plugins.attrib import attr
# from nose.plugins.skip import SkipTest

import isodate
import email.utils
import datetime
import threading
import unittest
import re
import pytz
from io import StringIO
# from cStringIO import StringIO
import requests
import json
import base64
import hmac
import _sha1 as sha
import xml.etree.ElementTree as ET
import time
import operator
# import nose
import os
import string
import random
import socket
import ssl
# from collections import namedtuple, OrderedDict

from email.header import decode_header

# from UserDict import DictMixin

from .utils import assert_raises, Counter
from .utils import generate_random
from .utils import _get_status_and_error_code
from .utils import _get_status
from .file_utils import *

from .policy import Policy, Statement, make_json_policy

from . import (
    setup,
    teardown,
    get_client,
    get_prefix,
    get_unauthenticated_client,
    get_bad_auth_client,
    get_v2_client,
    get_new_bucket,
    get_new_bucket_name,
    get_new_bucket_resource,
    get_config_is_secure,
    get_config_host,
    get_config_port,
    get_config_endpoint,
    get_main_aws_access_key,
    get_main_aws_secret_key,
    get_main_display_name,
    get_main_user_id,
    get_main_email,
    get_main_api_name,
    get_alt_aws_access_key,
    get_alt_aws_secret_key,
    get_alt_display_name,
    get_alt_user_id,
    get_alt_email,
    get_alt_client,
    get_tenant_client,
    get_buckets_list,
    get_objects_list,
    get_main_kms_keyid,
    get_secondary_kms_keyid,
    nuke_prefixed_buckets,
)


class TestPostObject(unittest.TestCase):

    def _get_post_url(self, bucket_name):
        endpoint = get_config_endpoint()
        return '{endpoint}/{bucket_name}'.format(endpoint=endpoint, bucket_name=bucket_name)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='anonymous browser based upload via POST request')
    # @attr(assertion='succeeds and returns written data')
    def test_post_object_anonymous_request(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        url = self._get_post_url(bucket_name)
        payload = OrderedDict([("key", "foo.txt"), ("acl", "public-read"),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)
        r = requests.post(url, files=payload)
        print('r=', r.content)
        print('headers = ', r.headers)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='succeeds and returns written data')
    def test_post_object_authenticated_request(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key, policy, sha).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request, no content-type header')
    # @attr(assertion='succeeds and returns written data')
    def test_post_object_authenticated_no_content_type(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key, policy, sha).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key="foo.txt")
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request, bad access key')
    # @attr(assertion='fails')
    def test_post_object_authenticated_request_bad_access_key(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document)
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key, policy, sha).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", 'foo'),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 403)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='anonymous browser based upload via POST request')
    # @attr(assertion='succeeds with status 201')
    def test_post_object_set_success_code(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)

        url = self._get_post_url(bucket_name)
        payload = OrderedDict([("key", "foo.txt"), ("acl", "public-read"),
                               ("success_action_status", "201"),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 201)
        message = ET.fromstring(r.content).find('Key')
        self.assertEqual(message.text, 'foo.txt')

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='anonymous browser based upload via POST request')
    # @attr(assertion='succeeds with status 204')
    def test_post_object_set_invalid_success_code(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)

        url = self._get_post_url(bucket_name)
        payload = OrderedDict([("key", "foo.txt"), ("acl", "public-read"),
                               ("success_action_status", "404"),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)
        self.assertEqual(r.content, '')

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='succeeds and returns written data')
    def test_post_object_upload_larger_than_chunk(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 5 * 1024 * 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        foo_string = 'foo' * 1024 * 1024

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', foo_string)])

        r = requests.post(url, files=payload)
        print('content = ', r.content)
        print('headers = ', r.headers)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        body = self._get_body(response)
        self.assertEqual(body, foo_string)

        # @attr(resource='object')
        # @attr(method='post')
        # @attr(operation='authenticated browser based upload via POST request')
        # @attr(assertion='succeeds and returns written data')

    def test_post_object_set_key_from_filename(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "${filename}"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('foo.txt', 'bar'))])

        r = requests.post(url, files=payload)
        print('content = ', r.content)
        print('headers = ', r.headers)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='succeeds with status 204')
    def test_post_object_ignored_header(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ("x-ignore-foo", "bar"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        print('content = ', r.content)
        print('headers = ', r.headers)
        self.assertEqual(r.status_code, 204)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='succeeds with status 204')
    def test_post_object_case_insensitive_condition_fields(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bUcKeT": bucket_name},
                               ["StArTs-WiTh", "$KeY", "foo"],
                               {"AcL": "private"},
                               ["StArTs-WiTh", "$CoNtEnT-TyPe", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        foo_string = 'foo' * 1024 * 1024

        payload = OrderedDict([("kEy", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("aCl", "private"), ("signature", signature), ("pOLICy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        print('content = ', r.content)
        print('headers = ', r.headers)
        self.assertEqual(r.status_code, 204)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='succeeds and returns redirect url')
    def test_post_object_success_redirect_action(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)

        url = self._get_post_url(bucket_name)
        redirect_url = self._get_post_url(bucket_name)

        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["eq", "$success_action_redirect", redirect_url],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ("success_action_redirect", redirect_url),
                               ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 200)
        url = r.url
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        self.assertEqual(url,
                         '{rurl}?bucket={bucket}&key={key}&etag=%22{etag}%22'.format(rurl=redirect_url,
                                                                                     bucket=bucket_name, key='foo.txt',
                                                                                     etag=response['ETag'].strip('"')))

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with invalid signature error')
    def test_post_object_invalid_signature(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", " $foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())[::-1]

        payload = OrderedDict([("key", " $foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 403)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with access key does not exist error')
    def test_post_object_invalid_access_key(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", " $foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", " $foo.txt"), ("AWSAccessKeyId", aws_access_key_id[::-1]),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 403)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with invalid expiration error')
    def test_post_object_invalid_date_format(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": str(expires),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", " $foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", " $foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with missing key error')
    def test_post_object_no_key_specified(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with missing signature error')
    def test_post_object_missing_signature(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", " $foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with extra input fields policy error')
    def test_post_object_missing_policy_condition(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               ["starts-with", "$key", " $foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 403)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='succeeds using starts-with restriction on metadata header')
    def test_post_object_user_specified_header(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024],
                               ["starts-with", "$x-amz-meta-foo", "bar"]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('x-amz-meta-foo', 'barclamp'), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        self.assertEqual(response['Metadata']['foo'], 'barclamp')

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with policy condition failed error due to missing field in POST request')
    def test_post_object_request_missing_policy_specified_field(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024],
                               ["starts-with", "$x-amz-meta-foo", "bar"]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 403)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with conditions must be list error')
    def test_post_object_condition_is_case_sensitive(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "CONDITIONS": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024],
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with expiration must be string error')
    def test_post_object_expires_is_case_sensitive(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"EXPIRATION": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024],
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with policy expired error')
    def test_post_object_expired_policy(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=-6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024],
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 403)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails using equality restriction on metadata header')
    def test_post_object_invalid_request_field_value(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024],
                               ["eq", "$x-amz-meta-foo", ""]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())
        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('x-amz-meta-foo', 'barclamp'), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 403)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with policy missing expiration error')
    def test_post_object_missing_expires_condition(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {
            "conditions": [
                {"bucket": bucket_name},
                ["starts-with", "$key", "foo"],
                {"acl": "private"},
                ["starts-with", "$Content-Type", "text/plain"],
                ["content-length-range", 0, 1024],
            ]
        }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])
        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with policy missing conditions error')
    def test_post_object_missing_conditions_list(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ")}

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with allowable upload size exceeded error')
    def test_post_object_upload_size_limit_exceeded(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 0],
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with invalid content length error')
    def test_post_object_missing_content_length_argument(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0],
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with invalid JSON error')
    def test_post_object_invalid_content_length_argument(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", -1, 0],
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='fails with upload size less than minimum allowable error')
    def test_post_object_upload_size_below_minimum(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 512, 1000],
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='empty conditions return appropriate error response')
    def test_post_object_empty_conditions(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"),
                           "conditions": [
                               {}
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document.encode())
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key.encode(), policy, hashlib.sha256).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"), ('file', ('bar'))])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 400)

    def setUp(self):
        setup()

    def tearDown(self):
        teardown()


if __name__ == '__main__':
    unittest.main()
