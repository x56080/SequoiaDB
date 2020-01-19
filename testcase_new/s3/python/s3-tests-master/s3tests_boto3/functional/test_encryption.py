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


class TestEncryption(unittest.TestCase):

    def _get_post_url(self, bucket_name):
        endpoint = get_config_endpoint()
        return '{endpoint}/{bucket_name}'.format(endpoint=endpoint, bucket_name=bucket_name)

    def _get_body(self, response):
        body = response['Body']
        got = body.read()
        return got

    def _test_encryption_sse_customer_write(self, file_size):
        """
        Tests Create a file of A's, use it to set_contents_from_file.
        Create a file of B's, use it to re-set_contents_from_file.
        Re-read the contents, and confirm we get B's
        """
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'testobj'
        data = 'A' * file_size
        sse_client_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw=='
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key=key, Body=data)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)
        response = client.get_object(Bucket=bucket_name, Key=key)
        body = self._get_body(response)
        self.assertEqual(body, data.encode())

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-C encrypted transfer 1 byte')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_encrypted_transfer_1b(self):
        self._test_encryption_sse_customer_write(1)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-C encrypted transfer 1KB')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_encrypted_transfer_1kb(self):
        self._test_encryption_sse_customer_write(1024)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-C encrypted transfer 1MB')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_encrypted_transfer_1MB(self):
        self._test_encryption_sse_customer_write(1024 * 1024)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test SSE-C encrypted transfer 13 bytes')
    # @attr(assertion='success')
    # @attr('encryption')
    def test_encrypted_transfer_13b(self):
        self._test_encryption_sse_customer_write(13)

    # @attr(assertion='success')
    # @attr('encryption')
    def test_encryption_sse_c_method_head(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = 'A' * 1000
        key = 'testobj'
        sse_client_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw=='
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key=key, Body=data)

        # e = assert_raises(ClientError, client.head_object, Bucket=bucket_name, Key=key)
        # status, error_code = _get_status_and_error_code(e.response)
        # self.assertEqual(status, 400)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.HeadObject', lf)
        response = client.head_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write encrypted with SSE-C and read without SSE-C')
    # @attr(assertion='operation fails')
    # @attr('encryption')
    # TODO: sequoias3不支持加密传输，所以没有带SSE-C去获取对象，不会失败
    def test_encryption_sse_c_present(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = 'A' * 1000
        key = 'testobj'
        sse_client_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw=='
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key=key, Body=data)

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write encrypted with SSE-C but read with other key')
    # @attr(assertion='operation fails')
    # @attr('encryption')
    # TODO: sequoias3不支持加密传输，所以没有带SSE-C去获取对象，不会失败
    def test_encryption_sse_c_other_key(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = 'A' * 100
        key = 'testobj'
        sse_client_headers_A = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw=='
        }
        sse_client_headers_B = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': '6b+WOZ1T3cqZMxgThRcXAQBrS5mXKdDUphvpxptl9/4=',
            'x-amz-server-side-encryption-customer-key-md5': 'arxBvwY2V4SiOne6yppVPQ=='
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers_A))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key=key, Body=data)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers_B))
        client.meta.events.register('before-call.s3.GetObject', lf)
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write encrypted with SSE-C, but md5 is bad')
    # @attr(assertion='operation fails')
    # @attr('encryption')
    # TODO: sequoias3不支持加密传输，所以没有带SSE-C去获取对象，不会失败
    def test_encryption_sse_c_invalid_md5(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = 'A' * 100
        key = 'testobj'
        sse_client_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'AAAAAAAAAAAAAAAAAAAAAA=='
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=key, Body=data)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write encrypted with SSE-C, but dont provide MD5')
    # @attr(assertion='operation fails')
    # @attr('encryption')
    def test_encryption_sse_c_no_md5(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = 'A' * 100
        key = 'testobj'
        sse_client_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=key, Body=data)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='declare SSE-C but do not provide key')
    # @attr(assertion='operation fails')
    # @attr('encryption')
    # TODO: sequoias3不支持加密传输，所以没有带SSE-C去获取对象，不会失败
    def test_encryption_sse_c_no_key(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = 'A' * 100
        key = 'testobj'
        sse_client_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=key, Body=data)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Do not declare SSE-C but provide key and MD5')
    # @attr(assertion='operation successfull, no encryption')
    # @attr('encryption')
    # TODO: sequoias3不支持加密传输，所以没有带SSE-C去获取对象，不会失败
    def test_encryption_key_no_sse_c(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = 'A' * 100
        key = 'testobj'
        sse_client_headers = {
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw=='
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=key, Body=data)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    def _multipart_upload_enc(self, client, bucket_name, key, size, part_size, init_headers, part_headers, metadata,
                              resend_parts):
        """
        generate a multi-part upload for a random file of specifed size,
        if requested, generate a list of the parts
        return the upload descriptor
        """
        if client is None:
            client = get_client()

        lf = (lambda **kwargs: kwargs['params']['headers'].update(init_headers))
        client.meta.events.register('before-call.s3.CreateMultipartUpload', lf)
        if metadata is None:
            response = client.create_multipart_upload(Bucket=bucket_name, Key=key)
        else:
            response = client.create_multipart_upload(Bucket=bucket_name, Key=key, Metadata=metadata)

        upload_id = response['UploadId']
        s = ''
        parts = []
        for i, part in enumerate(generate_random(size, part_size)):
            # part_num is necessary because PartNumber for upload_part and in parts must start at 1 and i starts at 0
            part_num = i + 1
            s += part
            lf = (lambda **kwargs: kwargs['params']['headers'].update(part_headers))
            client.meta.events.register('before-call.s3.UploadPart', lf)
            response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=part_num,
                                          Body=part)
            parts.append({'ETag': response['ETag'].strip('"'), 'PartNumber': part_num})
            if i in resend_parts:
                lf = (lambda **kwargs: kwargs['params']['headers'].update(part_headers))
                client.meta.events.register('before-call.s3.UploadPart', lf)
                client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=part_num, Body=part)

        return upload_id, s, parts

    def _check_content_using_range_enc(self, client, bucket_name, key, data, step, enc_headers=None):
        response = client.get_object(Bucket=bucket_name, Key=key)
        size = response['ContentLength']
        for ofs in range(0, size, step):
            toread = size - ofs
            if toread > step:
                toread = step
            end = ofs + toread - 1
            lf = (lambda **kwargs: kwargs['params']['headers'].update(enc_headers))
            client.meta.events.register('before-call.s3.GetObject', lf)
            r = 'bytes={s}-{e}'.format(s=ofs, e=end)
            response = client.get_object(Bucket=bucket_name, Key=key, Range=r)
            read_range = response['ContentLength']
            body = self._get_body(response)
            self.assertEqual(read_range, toread)
            self.assertEqual(body, data[ofs:end + 1])

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='complete multi-part upload')
    # @attr(assertion='successful')
    # @attr('encryption')
    # @attr('fails_on_aws') # allow-unordered is a non-standard extension
    # TODO: sequoias3不支持encryption
    def test_encryption_sse_c_multipart_upload(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "multipart_enc"
        content_type = 'text/plain'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        enc_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw==',
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
        client.meta.events.register('before-call.s3.GetObject', lf)
        response = client.get_object(Bucket=bucket_name, Key=key)

        self.assertEqual(response['Metadata'], metadata)
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-type'], content_type)

        body = self._get_body(response)
        self.assertEqual(body, data)
        size = response['ContentLength']
        self.assertEqual(len(body), size)

        self._check_content_using_range_enc(client, bucket_name, key, data, 1000000, enc_headers=enc_headers)
        self._check_content_using_range_enc(client, bucket_name, key, data, 10000000, enc_headers=enc_headers)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='multipart upload with bad key for uploading chunks')
    # @attr(assertion='successful')
    # @attr('encryption')
    # TODO: remove this fails_on_rgw when I fix it
    # @attr('fails_on_rgw')
    # TODO: sequoias3不支持encryption
    def test_encryption_sse_c_multipart_invalid_chunks_1(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "multipart_enc"
        content_type = 'text/plain'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        init_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw==',
            'Content-Type': content_type
        }
        part_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': '6b+WOZ1T3cqZMxgThRcXAQBrS5mXKdDUphvpxptl9/4=',
            'x-amz-server-side-encryption-customer-key-md5': 'arxBvwY2V4SiOne6yppVPQ=='
        }
        resend_parts = []

        e = assert_raises(ClientError, self._multipart_upload_enc, client=client, bucket_name=bucket_name,
                          key=key, size=objlen, part_size=5 * 1024 * 1024, init_headers=init_headers,
                          part_headers=part_headers, metadata=metadata, resend_parts=resend_parts)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='multipart upload with bad md5 for chunks')
    # @attr(assertion='successful')
    # @attr('encryption')
    # TODO: remove this fails_on_rgw when I fix it
    # TODO: sequoias3不支持encryption
    # @attr('fails_on_rgw')
    def test_encryption_sse_c_multipart_invalid_chunks_2(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "multipart_enc"
        content_type = 'text/plain'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        init_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw==',
            'Content-Type': content_type
        }
        part_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'AAAAAAAAAAAAAAAAAAAAAA=='
        }
        resend_parts = []

        e = assert_raises(ClientError, self._multipart_upload_enc, client=client, bucket_name=bucket_name,
                          key=key, size=objlen, part_size=5 * 1024 * 1024, init_headers=init_headers,
                          part_headers=part_headers, metadata=metadata, resend_parts=resend_parts)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='complete multi-part upload and download with bad key')
    # @attr(assertion='successful')
    # @attr('encryption')
    # TODO: sequoias3不支持encryption
    def test_encryption_sse_c_multipart_bad_download(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "multipart_enc"
        content_type = 'text/plain'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        put_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw==',
            'Content-Type': content_type
        }
        get_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': '6b+WOZ1T3cqZMxgThRcXAQBrS5mXKdDUphvpxptl9/4=',
            'x-amz-server-side-encryption-customer-key-md5': 'arxBvwY2V4SiOne6yppVPQ=='
        }
        resend_parts = []

        (upload_id, data, parts) = self._multipart_upload_enc(client, bucket_name, key, objlen,
                                                              part_size=5 * 1024 * 1024, init_headers=put_headers,
                                                              part_headers=put_headers, metadata=metadata,
                                                              resend_parts=resend_parts)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(put_headers))
        client.meta.events.register('before-call.s3.CompleteMultipartUpload', lf)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.head_bucket(Bucket=bucket_name)
        rgw_object_count = int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-object-count'])
        self.assertEqual(rgw_object_count, 1)
        rgw_bytes_used = int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-bytes-used'])
        self.assertEqual(rgw_bytes_used, objlen)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(put_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)
        response = client.get_object(Bucket=bucket_name, Key=key)

        self.assertEqual(response['Metadata'], metadata)
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-type'], content_type)

        lf = (lambda **kwargs: kwargs['params']['headers'].update(get_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr(assertion='succeeds and returns written data')
    # @attr('encryption')
    # TODO: sequoias3不支持encryption
    def test_encryption_sse_c_post_object_authenticated_request(self):
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
                               ["starts-with", "$x-amz-server-side-encryption-customer-algorithm", ""],
                               ["starts-with", "$x-amz-server-side-encryption-customer-key", ""],
                               ["starts-with", "$x-amz-server-side-encryption-customer-key-md5", ""],
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
                               ('x-amz-server-side-encryption-customer-algorithm', 'AES256'),
                               (
                                   'x-amz-server-side-encryption-customer-key',
                                   'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs='),
                               ('x-amz-server-side-encryption-customer-key-md5', 'DWygnHRtgiJ77HCm+1rvHw=='),
                               ('file', 'bar')])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)

        get_headers = {
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw=='
        }
        lf = (lambda **kwargs: kwargs['params']['headers'].update(get_headers))
        client.meta.events.register('before-call.s3.GetObject', lf)
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    def setUp(self):
        setup()

    def tearDown(self):
        teardown()


if __name__ == '__main__':
    unittest.main()