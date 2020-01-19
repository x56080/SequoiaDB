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


class TestKMS(unittest.TestCase):

    def _get_post_url(self, bucket_name):
        endpoint = get_config_endpoint()
        return '{endpoint}/{bucket_name}'.format(endpoint=endpoint, bucket_name=bucket_name)

    def _get_body(self, response):
        body = response['Body']
        got = body.read()
        return got

    # @attr(assertion='success')
    # @attr('encryption')
    def _test_sse_kms_customer_write(self, file_size, key_id='testkey-1'):
        """
        Tests Create a file of A's, use it to set_contents_from_file.
        Create a file of B's, use it to re-set_contents_from_file.
        Re-read the contents, and confirm we get B's
        """
        bucket_name = get_new_bucket()
        client = get_client()
        sse_kms_client_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': key_id
        }
        data = 'A' * file_size

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_kms_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key='testobj', Body=data)

        response = client.get_object(Bucket=bucket_name, Key='testobj')
        body = self._get_body(response)
        self.assertEqual(body, data)

    # @attr(resource='object')
    # @attr(method='head')
    # @attr(operation='Test SSE-KMS encrypted does perform head properly')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_sse_kms_method_head(self):
        kms_keyid = get_main_kms_keyid()
        bucket_name = get_new_bucket()
        client = get_client()
        sse_kms_client_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': kms_keyid
        }
        data = 'A' * 1000
        key = 'testobj'

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_kms_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key=key, Body=data)

        response = client.head_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['x-amz-server-side-encryption'], 'aws:kms')
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['x-amz-server-side-encryption-aws-kms-key-id'],
                         kms_keyid)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_kms_client_headers))
        client.meta.events.register('before-call.s3.HeadObject', lf)
        e = assert_raises(ClientError, client.head_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write encrypted with SSE-KMS and read without SSE-KMS')
    # @attr(assertion='operation success')
    # @attr('encryption')
    def test_sse_kms_present(self):
        kms_keyid = get_main_kms_keyid()
        bucket_name = get_new_bucket()
        client = get_client()
        sse_kms_client_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': kms_keyid
        }
        data = 'A' * 100
        key = 'testobj'

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_kms_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key=key, Body=data)

        response = client.get_object(Bucket=bucket_name, Key=key)
        body = self._get_body(response)
        self.assertEqual(body, data)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='declare SSE-KMS but do not provide key_id')
    # @attr(assertion='operation fails')
    # @attr('encryption')
    def test_sse_kms_no_key(self):
        bucket_name = get_new_bucket()
        client = get_client()
        sse_kms_client_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
        }
        data = 'A' * 100
        key = 'testobj'

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_kms_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)

        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=key, Body=data)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Do not declare SSE-KMS but provide key_id')
    # @attr(assertion='operation successfull, no encryption')
    # @attr('encryption')
    def test_sse_kms_not_declared(self):
        bucket_name = get_new_bucket()
        client = get_client()
        sse_kms_client_headers = {
            'x-amz-server-side-encryption-aws-kms-key-id': 'testkey-2'
        }
        data = 'A' * 100
        key = 'testobj'

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_kms_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)

        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=key, Body=data)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='complete KMS multi-part upload')
    # @attr(assertion='successful')
    # @attr('encryption')
    def test_sse_kms_multipart_upload(self):
        kms_keyid = get_main_kms_keyid()
        bucket_name = get_new_bucket()
        client = get_client()
        key = "multipart_enc"
        content_type = 'text/plain'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        enc_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': kms_keyid,
            'Content-Type': content_type
        }
        resend_parts = []

        (upload_id, data, parts) = self._multipart_upload_enc(client, bucket_name, key, objlen,
                                                              part_size=5 * 1024 * 1024, init_headers=enc_headers,
                                                              part_headers=enc_headers, metadata=metadata,
                                                              resend_parts=resend_parts)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(enc_headers))
        client.meta.events.register('before-call.s3.CompleteMultipartUpload', lf)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.head_bucket(Bucket=bucket_name)
        rgw_object_count = int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-object-count'])
        self.assertEqual(rgw_object_count, 1)
        rgw_bytes_used = int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-bytes-used'])
        self.assertEqual(rgw_bytes_used, objlen)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(enc_headers))
        client.meta.events.register('before-call.s3.UploadPart', lf)

        response = client.get_object(Bucket=bucket_name, Key=key)

        self.assertEqual(response['Metadata'], metadata)
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-type'], content_type)

        body = self._get_body(response)
        self.assertEqual(body, data)
        size = response['ContentLength']
        self.assertEqual(len(body), size)

        self.check_content_using_range(key, bucket_name, data, 1000000)
        self._check_content_using_range(key, bucket_name, data, 10000000)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='multipart KMS upload with bad key_id for uploading chunks')
    # @attr(assertion='successful')
    # @attr('encryption')
    def test_sse_kms_multipart_invalid_chunks_1(self):
        kms_keyid = get_main_kms_keyid()
        kms_keyid2 = get_secondary_kms_keyid()
        bucket_name = get_new_bucket()
        client = get_client()
        key = "multipart_enc"
        content_type = 'text/bla'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        init_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': kms_keyid,
            'Content-Type': content_type
        }
        part_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': kms_keyid2
        }
        resend_parts = []

        self._multipart_upload_enc(client, bucket_name, key, objlen, part_size=5 * 1024 * 1024,
                                   init_headers=init_headers, part_headers=part_headers, metadata=metadata,
                                   resend_parts=resend_parts)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='multipart KMS upload with unexistent key_id for chunks')
    # @attr(assertion='successful')
    # @attr('encryption')
    def test_sse_kms_multipart_invalid_chunks_2(self):
        kms_keyid = get_main_kms_keyid()
        bucket_name = get_new_bucket()
        client = get_client()
        key = "multipart_enc"
        content_type = 'text/plain'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        init_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': kms_keyid,
            'Content-Type': content_type
        }
        part_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': 'testkey-not-present'
        }
        resend_parts = []

        self._multipart_upload_enc(client, bucket_name, key, objlen, part_size=5 * 1024 * 1024,
                                   init_headers=init_headers, part_headers=part_headers, metadata=metadata,
                                   resend_parts=resend_parts)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated KMS browser based upload via POST request')
    # @attr(assertion='succeeds and returns written data')
    # @attr('encryption')
    def test_sse_kms_post_object_authenticated_request(self):
        kms_keyid = get_main_kms_keyid()
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
                               ["starts-with", "$x-amz-server-side-encryption", ""],
                               ["starts-with", "$x-amz-server-side-encryption-aws-kms-key-id", ""],
                               ["content-length-range", 0, 1024]
                           ]
                           }

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document)
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key, policy, sha).digest())

        payload = OrderedDict([("key", "foo.txt"), ("AWSAccessKeyId", aws_access_key_id),
                               ("acl", "private"), ("signature", signature), ("policy", policy),
                               ("Content-Type", "text/plain"),
                               ('x-amz-server-side-encryption', 'aws:kms'),
                               ('x-amz-server-side-encryption-aws-kms-key-id', kms_keyid),
                               ('file', 'bar')])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)

        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-KMS encrypted transfer 1 byte')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_sse_kms_transfer_1b(self):
        kms_keyid = get_main_kms_keyid()
        if kms_keyid is None:
            raise self.skipTest("skip...")
        self._test_sse_kms_customer_write(1, key_id=kms_keyid)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-KMS encrypted transfer 1KB')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_sse_kms_transfer_1kb(self):
        kms_keyid = get_main_kms_keyid()
        if kms_keyid is None:
            raise self.skipTest('skip...')
        self._test_sse_kms_customer_write(1024, key_id=kms_keyid)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-KMS encrypted transfer 1MB')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_sse_kms_transfer_1MB(self):
        kms_keyid = get_main_kms_keyid()
        if kms_keyid is None:
            raise self.skipTest('skip...')
        self._test_sse_kms_customer_write(1024 * 1024, key_id=kms_keyid)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-KMS encrypted transfer 13 bytes')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_sse_kms_transfer_13b(self):
        kms_keyid = get_main_kms_keyid()
        if kms_keyid is None:
            raise self.skipTest('skip...')
        self._test_sse_kms_customer_write(13, key_id=kms_keyid)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='write encrypted with SSE-KMS and read with SSE-KMS')
    # @attr(assertion='operation fails')
    # @attr('encryption')
    def test_sse_kms_read_declare(self):
        bucket_name = get_new_bucket()
        client = get_client()
        sse_kms_client_headers = {
            'x-amz-server-side-encryption': 'aws:kms',
            'x-amz-server-side-encryption-aws-kms-key-id': 'testkey-1'
        }
        data = 'A' * 100
        key = 'testobj'

        client.put_object(Bucket=bucket_name, Key=key, Body=data)
        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_kms_client_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test Bucket Policy')
    # @attr(assertion='succeeds')
    # @attr('bucket-policy')
    def test_bucket_policy(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'asdf'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')

        resource1 = "arn:aws:s3:::" + bucket_name
        resource2 = "arn:aws:s3:::" + bucket_name + "/*"
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Allow",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "{}".format(resource1),
                        "{}".format(resource2)
                    ]
                }]
            })

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        alt_client = get_alt_client()
        response = alt_client.list_objects(Bucket=bucket_name)
        self.assertEqual(len(response['Contents']), 1)

    # @attr('bucket-policy')
    # @attr('list-objects-v2')
    def test_bucketv2_policy(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'asdf'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')

        resource1 = "arn:aws:s3:::" + bucket_name
        resource2 = "arn:aws:s3:::" + bucket_name + "/*"
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Allow",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "{}".format(resource1),
                        "{}".format(resource2)
                    ]
                }]
            })

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        alt_client = get_alt_client()
        response = alt_client.list_objects_v2(Bucket=bucket_name)
        self.assertEqual(len(response['Contents']), 1)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test Bucket Policy and ACL')
    # @attr(assertion='fails')
    # @attr('bucket-policy')
    def test_bucket_policy_acl(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'asdf'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')

        resource1 = "arn:aws:s3:::" + bucket_name
        resource2 = "arn:aws:s3:::" + bucket_name + "/*"
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Deny",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "{}".format(resource1),
                        "{}".format(resource2)
                    ]
                }]
            })

        client.put_bucket_acl(Bucket=bucket_name, ACL='authenticated-read')
        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        alt_client = get_alt_client()
        e = assert_raises(ClientError, alt_client.list_objects, Bucket=bucket_name)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AccessDenied')

        client.delete_bucket_policy(Bucket=bucket_name)
        client.put_bucket_acl(Bucket=bucket_name, ACL='public-read')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test Bucket Policy and ACL with list-objects-v2')
    # @attr(assertion='fails')
    # @attr('bucket-policy')
    # @attr('list-objects-v2')
    def test_bucketv2_policy_acl(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'asdf'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')

        resource1 = "arn:aws:s3:::" + bucket_name
        resource2 = "arn:aws:s3:::" + bucket_name + "/*"
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Deny",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "{}".format(resource1),
                        "{}".format(resource2)
                    ]
                }]
            })

        client.put_bucket_acl(Bucket=bucket_name, ACL='authenticated-read')
        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        alt_client = get_alt_client()
        e = assert_raises(ClientError, alt_client.list_objects_v2, Bucket=bucket_name)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AccessDenied')

        client.delete_bucket_policy(Bucket=bucket_name)
        client.put_bucket_acl(Bucket=bucket_name, ACL='public-read')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test Bucket Policy for a user belonging to a different tenant')
    # @attr(assertion='succeeds')
    # @attr('bucket-policy')
    # TODO: remove this fails_on_rgw when I fix it
    # @attr('fails_on_rgw')
    def test_bucket_policy_different_tenant(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'asdf'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')

        resource1 = "arn:aws:s3::*:" + bucket_name
        resource2 = "arn:aws:s3::*:" + bucket_name + "/*"
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Allow",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "{}".format(resource1),
                        "{}".format(resource2)
                    ]
                }]
            })

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
    #
    #     # TODO: figure out how to change the bucketname
    #     def change_bucket_name(**kwargs):
    #         kwargs['params']['url'] = "http://localhost:8000/:{bucket_name}?encoding-type=url".format(
    #             bucket_name=bucket_name)
    #         kwargs['params']['url_path'] = "/:{bucket_name}".format(bucket_name=bucket_name)
    #         kwargs['params']['context']['signing']['bucket'] = ":{bucket_name}".format(bucket_name=bucket_name)
    #         print
    #         kwargs['request_signer']
    #         print
    #         kwargs
    #
    #     # bucket_name = ":" + bucket_name
    #     tenant_client = get_tenant_client()
    #     tenant_client.meta.events.register('before-call.s3.ListObjects', change_bucket_name)
    #     response = tenant_client.list_objects(Bucket=bucket_name)
    #     # alt_client = get_alt_client()
    #     # response = alt_client.list_objects(Bucket=bucket_name)
    #
    #     self.assertEqual(len(response['Contents']), 1)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test Bucket Policy for a user belonging to a different tenant')
    # @attr(assertion='succeeds')
    # @attr('bucket-policy')
    # TODO: remove this fails_on_rgw when I fix it
    # @attr('fails_on_rgw')
    # @attr('list-objects-v2')
    def test_bucketv2_policy_different_tenant(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'asdf'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')

        resource1 = "arn:aws:s3::*:" + bucket_name
        resource2 = "arn:aws:s3::*:" + bucket_name + "/*"
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Allow",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "{}".format(resource1),
                        "{}".format(resource2)
                    ]
                }]
            })

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        # TODO: figure out how to change the bucketname
        def change_bucket_name(**kwargs):
            kwargs['params']['url'] = "http://localhost:8000/:{bucket_name}?encoding-type=url".format(
                bucket_name=bucket_name)
            kwargs['params']['url_path'] = "/:{bucket_name}".format(bucket_name=bucket_name)
            kwargs['params']['context']['signing']['bucket'] = ":{bucket_name}".format(bucket_name=bucket_name)
            print
            kwargs['request_signer']
            print
            kwargs

        # bucket_name = ":" + bucket_name
        tenant_client = get_tenant_client()
        tenant_client.meta.events.register('before-call.s3.ListObjects', change_bucket_name)
        response = tenant_client.list_objects_v2(Bucket=bucket_name)
        # alt_client = get_alt_client()
        # response = alt_client.list_objects_v2(Bucket=bucket_name)

        self.assertEqual(len(response['Contents']), 1)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test Bucket Policy on another bucket')
    # @attr(assertion='succeeds')
    # @attr('bucket-policy')
    def test_bucket_policy_another_bucket(self):
        bucket_name = get_new_bucket()
        bucket_name2 = get_new_bucket()
        client = get_client()
        key = 'asdf'
        key2 = 'abcd'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')
        client.put_object(Bucket=bucket_name2, Key=key2, Body='abcd')
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Allow",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "arn:aws:s3:::*",
                        "arn:aws:s3:::*/*"
                    ]
                }]
            })

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
        response = client.get_bucket_policy(Bucket=bucket_name)
        response_policy = response['Policy']

        client.put_bucket_policy(Bucket=bucket_name2, Policy=response_policy)

        alt_client = get_alt_client()
        response = alt_client.list_objects(Bucket=bucket_name)
        self.assertEqual(len(response['Contents']), 1)

        alt_client = get_alt_client()
        response = alt_client.list_objects(Bucket=bucket_name2)
        self.assertEqual(len(response['Contents']), 1)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test Bucket Policy on another bucket with list-objects-v2')
    # @attr(assertion='succeeds')
    # @attr('bucket-policy')
    # @attr('list-objects-v2')
    def test_bucketv2_policy_another_bucket(self):
        bucket_name = get_new_bucket()
        bucket_name2 = get_new_bucket()
        client = get_client()
        key = 'asdf'
        key2 = 'abcd'
        client.put_object(Bucket=bucket_name, Key=key, Body='asdf')
        client.put_object(Bucket=bucket_name2, Key=key2, Body='abcd')
        policy_document = json.dumps(
            {
                "Version": "2012-10-17",
                "Statement": [{
                    "Effect": "Allow",
                    "Principal": {"AWS": "*"},
                    "Action": "s3:ListBucket",
                    "Resource": [
                        "arn:aws:s3:::*",
                        "arn:aws:s3:::*/*"
                    ]
                }]
            })

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
        response = client.get_bucket_policy(Bucket=bucket_name)
        response_policy = response['Policy']

        client.put_bucket_policy(Bucket=bucket_name2, Policy=response_policy)

        alt_client = get_alt_client()
        response = alt_client.list_objects_v2(Bucket=bucket_name)
        self.assertEqual(len(response['Contents']), 1)

        alt_client = get_alt_client()
        response = alt_client.list_objects_v2(Bucket=bucket_name2)
        self.assertEqual(len(response['Contents']), 1)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put condition operator end with ifExists')
    # @attr('bucket-policy')
    # TODO: remove this fails_on_rgw when I fix it
    # @attr('fails_on_rgw')
    def test_bucket_policy_set_condition_operator_end_with_IfExists(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'foo'
        client.put_object(Bucket=bucket_name, Key=key)
        policy = '''{
          "Version":"2012-10-17",
          "Statement": [{
            "Sid": "Allow Public Access to All Objects",
            "Effect": "Allow",
            "Principal": "*",
            "Action": "s3:GetObject",
            "Condition": {
                        "StringLikeIfExists": {
                            "aws:Referer": "http://www.example.com/*"
                        }
                    },
            "Resource": "arn:aws:s3:::%s/*"
          }
         ]
        }''' % bucket_name
        boto3.set_stream_logger(name='botocore')
        client.put_bucket_policy(Bucket=bucket_name, Policy=policy)

        request_headers = {'referer': 'http://www.example.com/'}

        lf = (lambda **kwargs: kwargs['params']['headers'].update(request_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)

        response = client.get_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        request_headers = {'referer': 'http://www.example.com/index.html'}

        lf = (lambda **kwargs: kwargs['params']['headers'].update(request_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)

        response = client.get_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        # the 'referer' headers need to be removed for this one
        # response = client.get_object(Bucket=bucket_name, Key=key)
        # self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        request_headers = {'referer': 'http://example.com'}

        lf = (lambda **kwargs: kwargs['params']['headers'].update(request_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)

        # TODO: Compare Requests sent in Boto3, Wireshark, RGW Log for both boto and boto3
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

        response = client.get_bucket_policy(Bucket=bucket_name)
        print
        response
