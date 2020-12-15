import hashlib
from collections import OrderedDict, namedtuple

import boto3
import botocore.session
from boto3.s3.transfer import TransferConfig
from botocore.exceptions import ClientError
from botocore.exceptions import ParamValidationError
# from nose.tools import eq_ as eq
# from nose.plugins.attrib import attr
# from nose.plugins.skip import SkipTest
import datetime
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
from lib.testlib import S3TestBase
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


class TestCORS(S3TestBase):

    def _simple_http_req_100_cont(self, host, port, is_secure, method, resource):
        """
        Send the specified request w/expect 100-continue
        and await confirmation.
        """
        req = '{method} {resource} HTTP/1.1\r\nHost: {host}\r\nAccept-Encoding: identity\r\nContent-Length: 123\r\nExpect: 100-continue\r\n\r\n'.format(
            method=method,
            resource=resource,
            host=host,
        )

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        if is_secure:
            s = ssl.wrap_socket(s);
        s.settimeout(5)
        s.connect((host, port))
        s.send(req)

        try:
            data = s.recv(1024)
        except socket.erro as msg:
            print
            'got response: ', msg
            print
            'most likely server doesn\'t support 100-continue'

        s.close()
        l = data.split(' ')

        assert l[0].startswith('HTTP')

        return l[1]

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='w/expect continue')
    # @attr(assertion='succeeds if object is public-read-write')
    # @attr('100_continue')
    # @attr('fails_on_mod_proxy_fcgi')
    # TODO: sequoias3不支持给桶或者对象设置ACL
    def test_100_continue(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name)
        objname = 'testobj'
        resource = '/{bucket}/{obj}'.format(bucket=bucket_name, obj=objname)

        host = get_config_host()
        port = get_config_port()
        is_secure = get_config_is_secure()

        # NOTES: this test needs to be tested when is_secure is True
        status = self._simple_http_req_100_cont(host, port, is_secure, 'PUT', resource)
        self.assertEqual(status, '403')

        client.put_bucket_acl(Bucket=bucket_name, ACL='public-read-write')

        status = self._simple_http_req_100_cont(host, port, is_secure, 'PUT', resource)
        self.assertEqual(status, '100')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set cors')
    # @attr(assertion='succeeds')
    # @attr('cors')
    # TODO: sequoias3不支持cors
    def test_set_cors(self):
        bucket_name = get_new_bucket()
        client = get_client()
        allowed_methods = ['GET', 'PUT']
        allowed_origins = ['*.get', '*.put']

        cors_config = {
            'CORSRules': [
                {'AllowedMethods': allowed_methods,
                 'AllowedOrigins': allowed_origins,
                 },
            ]
        }

        e = assert_raises(ClientError, client.get_bucket_cors, Bucket=bucket_name)
        status = _get_status(e.response)
        self.assertEqual(status, 404)

        client.put_bucket_cors(Bucket=bucket_name, CORSConfiguration=cors_config)
        response = client.get_bucket_cors(Bucket=bucket_name)
        self.assertEqual(response['CORSRules'][0]['AllowedMethods'], allowed_methods)
        self.assertEqual(response['CORSRules'][0]['AllowedOrigins'], allowed_origins)

        client.delete_bucket_cors(Bucket=bucket_name)
        e = assert_raises(ClientError, client.get_bucket_cors, Bucket=bucket_name)
        status = _get_status(e.response)
        self.assertEqual(status, 404)

    def _cors_request_and_check(self, func, url, headers, expect_status, expect_allow_origin, expect_allow_methods):
        r = func(url, headers=headers)
        self.assertEqual(r.status_code, expect_status)

        assert r.headers.get('access-control-allow-origin', None) == expect_allow_origin
        assert r.headers.get('access-control-allow-methods', None) == expect_allow_methods

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='check cors response when origin header set')
    # @attr(assertion='returning cors header')
    # @attr('cors')
    # TODO: sequoias3不支持cors
    def test_cors_origin_response(self):
        bucket_name = self._setup_bucket_acl(bucket_acl='public-read')
        client = get_client()

        cors_config = {
            'CORSRules': [
                {'AllowedMethods': ['GET'],
                 'AllowedOrigins': ['*suffix'],
                 },
                {'AllowedMethods': ['GET'],
                 'AllowedOrigins': ['start*end'],
                 },
                {'AllowedMethods': ['GET'],
                 'AllowedOrigins': ['prefix*'],
                 },
                {'AllowedMethods': ['PUT'],
                 'AllowedOrigins': ['*.put'],
                 }
            ]
        }

        e = assert_raises(ClientError, client.get_bucket_cors, Bucket=bucket_name)
        status = _get_status(e.response)
        self.assertEqual(status, 404)

        client.put_bucket_cors(Bucket=bucket_name, CORSConfiguration=cors_config)

        time.sleep(3)

        url = self._get_post_url(bucket_name)

        self._cors_request_and_check(requests.get, url, None, 200, None, None)
        self._cors_request_and_check(requests.get, url, {'Origin': 'foo.suffix'}, 200, 'foo.suffix', 'GET')
        self._cors_request_and_check(requests.get, url, {'Origin': 'foo.bar'}, 200, None, None)
        self._cors_request_and_check(requests.get, url, {'Origin': 'foo.suffix.get'}, 200, None, None)
        self._cors_request_and_check(requests.get, url, {'Origin': 'startend'}, 200, 'startend', 'GET')
        self._cors_request_and_check(requests.get, url, {'Origin': 'start1end'}, 200, 'start1end', 'GET')
        self._cors_request_and_check(requests.get, url, {'Origin': 'start12end'}, 200, 'start12end', 'GET')
        self._cors_request_and_check(requests.get, url, {'Origin': '0start12end'}, 200, None, None)
        self._cors_request_and_check(requests.get, url, {'Origin': 'prefix'}, 200, 'prefix', 'GET')
        self._cors_request_and_check(requests.get, url, {'Origin': 'prefix.suffix'}, 200, 'prefix.suffix', 'GET')
        self._cors_request_and_check(requests.get, url, {'Origin': 'bla.prefix'}, 200, None, None)

        obj_url = '{u}/{o}'.format(u=url, o='bar')
        self._cors_request_and_check(requests.get, obj_url, {'Origin': 'foo.suffix'}, 404, 'foo.suffix', 'GET')
        self._cors_request_and_check(requests.put, obj_url,
                                     {'Origin': 'foo.suffix', 'Access-Control-Request-Method': 'GET',
                                      'content-length': '0'}, 403, 'foo.suffix', 'GET')
        self._cors_request_and_check(requests.put, obj_url,
                                     {'Origin': 'foo.suffix', 'Access-Control-Request-Method': 'PUT',
                                      'content-length': '0'}, 403, None, None)

        self._cors_request_and_check(requests.put, obj_url,
                                     {'Origin': 'foo.suffix', 'Access-Control-Request-Method': 'DELETE',
                                      'content-length': '0'}, 403, None, None)
        self._cors_request_and_check(requests.put, obj_url, {'Origin': 'foo.suffix', 'content-length': '0'}, 403, None,
                                     None)

        self._cors_request_and_check(requests.put, obj_url, {'Origin': 'foo.put', 'content-length': '0'}, 403,
                                     'foo.put',
                                     'PUT')

        self._cors_request_and_check(requests.get, obj_url, {'Origin': 'foo.suffix'}, 404, 'foo.suffix', 'GET')

        self._cors_request_and_check(requests.options, url, None, 400, None, None)
        self._cors_request_and_check(requests.options, url, {'Origin': 'foo.suffix'}, 400, None, None)
        self._cors_request_and_check(requests.options, url, {'Origin': 'bla'}, 400, None, None)
        self._cors_request_and_check(requests.options, obj_url,
                                     {'Origin': 'foo.suffix', 'Access-Control-Request-Method': 'GET',
                                      'content-length': '0'}, 200, 'foo.suffix', 'GET')
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'foo.bar', 'Access-Control-Request-Method': 'GET'},
                                     403,
                                     None, None)
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'foo.suffix.get', 'Access-Control-Request-Method': 'GET'},
                                     403, None, None)
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'startend', 'Access-Control-Request-Method': 'GET'},
                                     200,
                                     'startend', 'GET')
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'start1end', 'Access-Control-Request-Method': 'GET'},
                                     200,
                                     'start1end', 'GET')
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'start12end', 'Access-Control-Request-Method': 'GET'},
                                     200, 'start12end', 'GET')
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': '0start12end', 'Access-Control-Request-Method': 'GET'},
                                     403, None, None)
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'prefix', 'Access-Control-Request-Method': 'GET'},
                                     200,
                                     'prefix', 'GET')
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'prefix.suffix', 'Access-Control-Request-Method': 'GET'},
                                     200, 'prefix.suffix', 'GET')
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'bla.prefix', 'Access-Control-Request-Method': 'GET'},
                                     403, None, None)
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'foo.put', 'Access-Control-Request-Method': 'GET'},
                                     403,
                                     None, None)
        self._cors_request_and_check(requests.options, url,
                                     {'Origin': 'foo.put', 'Access-Control-Request-Method': 'PUT'},
                                     200,
                                     'foo.put', 'PUT')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='check cors response when origin is set to wildcard')
    # @attr(assertion='returning cors header')
    # @attr('cors')
    # TODO: sequoias3不支持cors
    def test_cors_origin_wildcard(self):
        bucket_name = self._setup_bucket_acl(bucket_acl='public-read')
        client = get_client()

        cors_config = {
            'CORSRules': [
                {'AllowedMethods': ['GET'],
                 'AllowedOrigins': ['*'],
                 },
            ]
        }

        e = assert_raises(ClientError, client.get_bucket_cors, Bucket=bucket_name)
        status = _get_status(e.response)
        self.assertEqual(status, 404)

        client.put_bucket_cors(Bucket=bucket_name, CORSConfiguration=cors_config)

        time.sleep(3)

        url = self._get_post_url(bucket_name)

        self._cors_request_and_check(requests.get, url, None, 200, None, None)
        self._cors_request_and_check(requests.get, url, {'Origin': 'example.origin'}, 200, '*', 'GET')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='check cors response when Access-Control-Request-Headers is set in option request')
    # @attr(assertion='returning cors header')
    # @attr('cors')
    # TODO: sequoias3不支持cors
    def test_cors_header_option(self):
        bucket_name = self._setup_bucket_acl(bucket_acl='public-read')
        client = get_client()

        cors_config = {
            'CORSRules': [
                {'AllowedMethods': ['GET'],
                 'AllowedOrigins': ['*'],
                 'ExposeHeaders': ['x-amz-meta-header1'],
                 },
            ]
        }

        e = assert_raises(ClientError, client.get_bucket_cors, Bucket=bucket_name)
        status = _get_status(e.response)
        self.assertEqual(status, 404)

        client.put_bucket_cors(Bucket=bucket_name, CORSConfiguration=cors_config)

        time.sleep(3)

        url = self._get_post_url(bucket_name)
        obj_url = '{u}/{o}'.format(u=url, o='bar')

        self._cors_request_and_check(requests.options, obj_url,
                                     {'Origin': 'example.origin',
                                      'Access-Control-Request-Headers': 'x-amz-meta-header2',
                                      'Access-Control-Request-Method': 'GET'}, 403, None, None)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='put tags')
    # @attr(assertion='succeeds')
    # @attr('tagging')
    # TODO:sequoias3不支持给桶设置标签
    def test_set_tagging(self):
        bucket_name = get_new_bucket()
        client = get_client()

        tags = {
            'TagSet': [
                {
                    'Key': 'Hello',
                    'Value': 'World'
                },
            ]
        }

        e = assert_raises(ClientError, client.get_bucket_tagging, Bucket=bucket_name)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchTagSetError')

        client.put_bucket_tagging(Bucket=bucket_name, Tagging=tags)

        response = client.get_bucket_tagging(Bucket=bucket_name)
        self.assertEqual(len(response['TagSet']), 1)
        self.assertEqual(response['TagSet'][0]['Key'], 'Hello')
        self.assertEqual(response['TagSet'][0]['Value'], 'World')

        client.delete_bucket_tagging(Bucket=bucket_name)
        e = assert_raises(ClientError, client.get_bucket_tagging, Bucket=bucket_name)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchTagSetError')

    def _verify_atomic_key_data(self, bucket_name, key, size=-1, char=None):
        """
        Make sure file is of the expected size and (simulated) content
        """
        fp_verify = FakeFileVerifier(char)
        client = get_client()
        client.download_fileobj(bucket_name, key, fp_verify)
        if size >= 0:
            self.assertEqual(fp_verify.size, size)

    def _test_atomic_read(self, file_size):
        """
        Create a file of A's, use it to set_contents_from_file.
        Create a file of B's, use it to re-set_contents_from_file.
        Re-read the contents, and confirm we get B's
        """
        bucket_name = get_new_bucket()
        client = get_client()

        fp_a = FakeWriteFile(file_size, 'A')
        client.put_object(Bucket=bucket_name, Key='testobj', Body=fp_a)

        fp_b = FakeWriteFile(file_size, 'B')

        # client.put_object(Bucket=bucket_name, Key='testobj', Body=fp_b)

        fp_a2 = FakeReadFile(file_size, 'A',
                             lambda: client.put_object(Bucket=bucket_name, Key='testobj', Body=fp_b)
                             )

        read_client = get_client()

        # config = TransferConfig(max_concurrency=2, use_threads=True, max_io_queue=100, multipart_chunksize=1024*1024*8,
        #                         multipart_threshold=1024*1024*8, io_chunksize=256*1024)

        # read_client.download_fileobj(Bucket=bucket_name, Key='testobj', Fileobj=fp_a2, Config=config)
        read_client.download_fileobj(Bucket=bucket_name, Key='testobj', Fileobj=fp_a2)

        fp_a2.close()

        self._verify_atomic_key_data(bucket_name, 'testobj', file_size, 'B')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='read atomicity')
    # @attr(assertion='1MB successful')
    def test_atomic_read_1mb(self):
        self._test_atomic_read(1024*1024*1)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='read atomicity')
    # @attr(assertion='4MB successful')
    def test_atomic_read_4mb(self):
        self._test_atomic_read(1024 * 200)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='read atomicity')
    # @attr(assertion='8MB successful')
    def test_atomic_read_8mb(self):
        self._test_atomic_read(1024 * 200)

    def _test_atomic_write(self, file_size):
        """
        Create a file of A's, use it to set_contents_from_file.
        Verify the contents are all A's.
        Create a file of B's, use it to re-set_contents_from_file.
        Before re-set continues, verify content's still A's
        Re-read the contents, and confirm we get B's
        """
        bucket_name = get_new_bucket()
        client = get_client()
        objname = 'testobj'

        # create <file_size> file of A's
        fp_a = FakeWriteFile(file_size, 'A')
        client.put_object(Bucket=bucket_name, Key=objname, Body=fp_a)

        # verify A's
        self._verify_atomic_key_data(bucket_name, objname, file_size, 'A')

        # create <file_size> file of B's
        # but try to verify the file before we finish writing all the B's
        fp_b = FakeWriteFile(file_size, 'B',
                             lambda: self._verify_atomic_key_data(bucket_name, objname, file_size)
                             )

        client.put_object(Bucket=bucket_name, Key=objname, Body=fp_b)

        # verify B's
        self._verify_atomic_key_data(bucket_name, objname, file_size, 'B')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write atomicity')
    # @attr(assertion='1MB successful')
    def test_atomic_write_1mb(self):
        self._test_atomic_write(1024 * 128)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write atomicity')
    # @attr(assertion='4MB successful')
    def test_atomic_write_4mb(self):
        self._test_atomic_write(1024 * 1024 * 4)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write atomicity')
    # @attr(assertion='8MB successful')
    def test_atomic_write_8mb(self):
        self._test_atomic_write(1024 * 1024 * 8)

    def _test_atomic_dual_write(self, file_size):
        """
        create an object, two sessions writing different contents
        confirm that it is all one or the other
        """
        bucket_name = get_new_bucket()
        objname = 'testobj'
        client = get_client()
        client.put_object(Bucket=bucket_name, Key=objname)

        # write <file_size> file of B's
        # but before we're done, try to write all A's
        fp_a = FakeWriteFile(file_size, 'A')

        def rewind_put_fp_a():
            fp_a.seek(0)
            client.put_object(Bucket=bucket_name, Key=objname, Body=fp_a)

        fp_b = FakeWriteFile(file_size, 'B', rewind_put_fp_a)
        client.put_object(Bucket=bucket_name, Key=objname, Body=fp_b)

        # verify the file
        self._verify_atomic_key_data(bucket_name, objname, file_size)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write one or the other')
    # @attr(assertion='1MB successful')
    def test_atomic_dual_write_1mb(self):
        self._test_atomic_dual_write(1024 * 1024)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write one or the other')
    # @attr(assertion='4MB successful')
    def test_atomic_dual_write_4mb(self):
        self._test_atomic_dual_write(1024 * 1024 * 4)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write one or the other')
    # @attr(assertion='8MB successful')
    def test_atomic_dual_write_8mb(self):
        self._test_atomic_dual_write(1024 * 1024 * 8)

    def _test_atomic_conditional_write(self, file_size):
        """
        Create a file of A's, use it to set_contents_from_file.
        Verify the contents are all A's.
        Create a file of B's, use it to re-set_contents_from_file.
        Before re-set continues, verify content's still A's
        Re-read the contents, and confirm we get B's
        """
        bucket_name = get_new_bucket()
        objname = 'testobj'
        client = get_client()

        # create <file_size> file of A's
        fp_a = FakeWriteFile(file_size, 'A')
        client.put_object(Bucket=bucket_name, Key=objname, Body=fp_a)

        fp_b = FakeWriteFile(file_size, 'B',
                             lambda: self._verify_atomic_key_data(bucket_name, objname, file_size)
                             )

        # create <file_size> file of B's
        # but try to verify the file before we finish writing all the B's
        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-Match': '*'}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key=objname, Body=fp_b)

        # verify B's
        self._verify_atomic_key_data(bucket_name, objname, file_size, 'B')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write atomicity')
    # @attr(assertion='1MB successful')
    # @attr('fails_on_aws')
    def test_atomic_conditional_write_1mb(self):
        self._test_atomic_conditional_write(1024 * 1024)

    def _test_atomic_dual_conditional_write(self, file_size):
        """
        create an object, two sessions writing different contents
        confirm that it is all one or the other
        """
        bucket_name = get_new_bucket()
        objname = 'testobj'
        client = get_client()

        fp_a = FakeWriteFile(file_size, 'A')
        response = client.put_object(Bucket=bucket_name, Key=objname, Body=fp_a)
        self._verify_atomic_key_data(bucket_name, objname, file_size, 'A')
        etag_fp_a = response['ETag'].replace('"', '')

        # write <file_size> file of C's
        # but before we're done, try to write all B's
        fp_b = FakeWriteFile(file_size, 'B')
        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-Match': etag_fp_a}))
        client.meta.events.register('before-call.s3.PutObject', lf)

        def rewind_put_fp_b():
            fp_b.seek(0)
            client.put_object(Bucket=bucket_name, Key=objname, Body=fp_b)

        fp_c = FakeWriteFile(file_size, 'C', rewind_put_fp_b)

        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=objname, Body=fp_c)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

        # verify the file
        self._verify_atomic_key_data(bucket_name, objname, file_size, 'B')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write one or the other')
    # @attr(assertion='1MB successful')
    # @attr('fails_on_aws')
    # TODO: test not passing with SSL, fix this
    # @attr('fails_on_rgw')
    # TODO: sequoias3不支持事件注册
    def test_atomic_dual_conditional_write_1mb(self):
        self._test_atomic_dual_conditional_write(1024 * 1024)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write file in deleted bucket')
    # @attr(assertion='fail 404')
    # @attr('fails_on_aws')
    # TODO: test not passing with SSL, fix this
    # @attr('fails_on_rgw')
    def test_atomic_write_bucket_gone(self):
        bucket_name = get_new_bucket()
        client = get_client()

        def remove_bucket():
            client.delete_bucket(Bucket=bucket_name)

        objname = 'foo'
        fp_a = FakeWriteFile(1024 * 1024, 'A', remove_bucket)

        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key=objname, Body=fp_a)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='begin to overwrite file with multipart upload then abort')
    # @attr(assertion='read back original key contents')
    def test_atomic_multipart_upload_write(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.create_multipart_upload(Bucket=bucket_name, Key='foo')
        upload_id = response['UploadId']

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

        client.abort_multipart_upload(Bucket=bucket_name, Key='foo', UploadId=upload_id)

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    def setUp(self):
        # print(self.__module__+ " setup: " + str(datetime.now()))
        setup()

    def tearDown(self):
        print("end...")
        # print(self.__module__ + " tearDown: " + str(datetime.now()))
        # teardown()

if __name__ == '__main__':
    unittest.main()