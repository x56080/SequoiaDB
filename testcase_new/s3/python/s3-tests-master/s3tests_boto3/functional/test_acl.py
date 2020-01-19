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


class TestACL(unittest.TestCase):

    def _do_set_bucket_canned_acl(self, client, bucket_name, canned_acl, i, results):
        try:
            client.put_bucket_acl(ACL=canned_acl, Bucket=bucket_name)
            results[i] = True
        except:
            results[i] = False

    def _do_set_bucket_canned_acl_concurrent(self, client, bucket_name, canned_acl, num, results):
        t = []
        for i in range(num):
            thr = threading.Thread(target=self._do_set_bucket_canned_acl,
                                   args=(client, bucket_name, canned_acl, i, results))
            thr.start()
            t.append(thr)
        return t

    def _do_wait_completion(self, t):
        for thr in t:
            thr.join()

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='concurrent set of acls on a bucket')
    # @attr(assertion='works')
    # TODO:目前sequoias3不支持acl
    def test_bucket_concurrent_set_canned_acl(self):
        bucket_name = get_new_bucket()
        client = get_client()

        num_threads = 50  # boto2 retry defaults to 5 so we need a thread to fail at least 5 times
        # this seems like a large enough number to get through retry (if bug
        # exists)
        results = [None] * num_threads

        t = self._do_set_bucket_canned_acl_concurrent(client, bucket_name, 'public-read', num_threads, results)
        self._do_wait_completion(t)

        for r in results:
            self.assertEqual(r, True)

    http_response = None

    def get_http_response(self, **kwargs):
        global http_response
        http_response = kwargs['http_response'].__dict__

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='read contents that were never written to raise one error response')
    # @attr(assertion='RequestId appears in the error response')
    # TODO: 需要弄清楚 client.meta.events.register的作用
    def test_object_requestid_matches_header_on_error(self):
        bucket_name = get_new_bucket()
        client = get_client()

        # get http response after failed request
        client.meta.events.register('after-call.s3.GetObject', self.get_http_response())
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='bar')
        response_body = http_response['_content']
        request_id = re.search(r'<RequestId>(.*)</RequestId>', response_body.encode('utf-8')).group(1)
        assert request_id is not None
        self.assertEqual(request_id, e.response['ResponseMetadata']['RequestId'])

    def _setup_bucket_object_acl(bucket_acl, object_acl):
        """
        add a foo key, and specified key and bucket acls to
        a (new or existing) bucket.
        """
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL=bucket_acl, Bucket=bucket_name)
        client.put_object(ACL=object_acl, Bucket=bucket_name, Key='foo')

        return bucket_name

    def _setup_bucket_acl(bucket_acl=None):
        """
        set up a new bucket with specified acl
        """
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL=bucket_acl, Bucket=bucket_name)

        return bucket_name

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='publically readable bucket')
    # @attr(assertion='bucket is readable')
    # TODO:sequoias3不支持桶和对象的ACL
    def test_object_raw_get(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')

        unauthenticated_client = get_unauthenticated_client()
        response = unauthenticated_client.get_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='deleted object and bucket')
    # @attr(assertion='fails 404')
    # TODO:sequoias3不支持桶和对象的ACL
    def test_object_raw_get_bucket_gone(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()

        client.delete_object(Bucket=bucket_name, Key='foo')
        client.delete_bucket(Bucket=bucket_name)

        unauthenticated_client = get_unauthenticated_client()

        e = assert_raises(ClientError, unauthenticated_client.get_object, Bucket=bucket_name, Key='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='deleted object and bucket')
    # @attr(assertion='fails 404')
    # TODO:sequoias3不支持桶和对象的ACL
    def test_object_delete_key_bucket_gone(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()

        client.delete_object(Bucket=bucket_name, Key='foo')
        client.delete_bucket(Bucket=bucket_name)

        unauthenticated_client = get_unauthenticated_client()

        e = assert_raises(ClientError, unauthenticated_client.delete_object, Bucket=bucket_name, Key='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='deleted object')
    # @attr(assertion='fails 404')
    # TODO:sequoias3不支持桶和对象的ACL
    def test_object_raw_get_object_gone(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()

        client.delete_object(Bucket=bucket_name, Key='foo')

        unauthenticated_client = get_unauthenticated_client()

        e = assert_raises(ClientError, unauthenticated_client.get_object, Bucket=bucket_name, Key='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchKey')

    # @attr(resource='bucket')
    # @attr(method='head')
    # @attr(operation='head bucket')
    # @attr(assertion='succeeds')
    def test_bucket_head(self):
        bucket_name = get_new_bucket()
        client = get_client()

        response = client.head_bucket(Bucket=bucket_name)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr('fails_on_aws')
    # @attr(resource='bucket')
    # @attr(method='head')
    # @attr(operation='read bucket extended information')
    # @attr(assertion='extended information is getting updated')
    # TODO:ceph s3扩展的头部，amazons3没有这些头部
    def test_bucket_head_extended(self):
        bucket_name = get_new_bucket()
        client = get_client()

        response = client.head_bucket(Bucket=bucket_name)
        self.assertEqual(int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-object-count']), 0)
        self.assertEqual(int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-bytes-used']), 0)

        self._create_objects(bucket_name=bucket_name, keys=['foo', 'bar', 'baz'])
        response = client.head_bucket(Bucket=bucket_name)

        self.assertEqual(int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-object-count']), 3)
        self.assertEqual(int(response['ResponseMetadata']['HTTPHeaders']['x-rgw-bytes-used']), 9)

    # @attr(resource='bucket.acl')
    # @attr(method='get')
    # @attr(operation='unauthenticated on private bucket')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_get_bucket_acl(self):
        bucket_name = self._setup_bucket_object_acl('private', 'public-read')

        unauthenticated_client = get_unauthenticated_client()
        response = unauthenticated_client.get_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object.acl')
    # @attr(method='get')
    # @attr(operation='unauthenticated on private object')
    # @attr(assertion='fails 403')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_get_object_acl(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'private')

        unauthenticated_client = get_unauthenticated_client()
        e = assert_raises(ClientError, unauthenticated_client.get_object, Bucket=bucket_name, Key='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AccessDenied')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='authenticated on public bucket/object')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_authenticated(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')

        client = get_client()
        response = client.get_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='authenticated on private bucket/private object with modified response headers')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_response_headers(self):
        bucket_name = self._setup_bucket_object_acl('private', 'private')

        client = get_client()

        response = client.get_object(Bucket=bucket_name, Key='foo', ResponseCacheControl='no-cache',
                                     ResponseContentDisposition='bla', ResponseContentEncoding='aaa',
                                     ResponseContentLanguage='esperanto', ResponseContentType='foo/bar',
                                     ResponseExpires='123')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-type'], 'foo/bar')
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-disposition'], 'bla')
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-language'], 'esperanto')
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-encoding'], 'aaa')
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['cache-control'], 'no-cache')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='authenticated on private bucket/public object')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_authenticated_bucket_acl(self):
        bucket_name = self._setup_bucket_object_acl('private', 'public-read')

        client = get_client()
        response = client.get_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='authenticated on public bucket/private object')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_authenticated_object_acl(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'private')

        client = get_client()
        response = client.get_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='authenticated on deleted object and bucket')
    # @attr(assertion='fails 404')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_authenticated_bucket_gone(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()

        client.delete_object(Bucket=bucket_name, Key='foo')
        client.delete_bucket(Bucket=bucket_name)

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='authenticated on deleted object')
    # @attr(assertion='fails 404')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_authenticated_object_gone(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()

        client.delete_object(Bucket=bucket_name, Key='foo')

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchKey')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='x-amz-expires check not expired')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_get_x_amz_expires_not_expired(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()
        params = {'Bucket': bucket_name, 'Key': 'foo'}

        url = client.generate_presigned_url(ClientMethod='get_object', Params=params, ExpiresIn=100000,
                                            HttpMethod='GET')

        res = requests.get(url).__dict__
        self.assertEqual(res['status_code'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='check x-amz-expires value out of range zero')
    # @attr(assertion='fails 403')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_get_x_amz_expires_out_range_zero(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()
        params = {'Bucket': bucket_name, 'Key': 'foo'}

        url = client.generate_presigned_url(ClientMethod='get_object', Params=params, ExpiresIn=0, HttpMethod='GET')

        res = requests.get(url).__dict__
        self.assertEqual(res['status_code'], 403)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='check x-amz-expires value out of max range')
    # @attr(assertion='fails 403')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_get_x_amz_expires_out_max_range(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()
        params = {'Bucket': bucket_name, 'Key': 'foo'}

        url = client.generate_presigned_url(ClientMethod='get_object', Params=params, ExpiresIn=609901,
                                            HttpMethod='GET')

        res = requests.get(url).__dict__
        self.assertEqual(res['status_code'], 403)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='check x-amz-expires value out of positive range')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_raw_get_x_amz_expires_out_positive_range(self):
        bucket_name = self._setup_bucket_object_acl('public-read', 'public-read')
        client = get_client()
        params = {'Bucket': bucket_name, 'Key': 'foo'}

        url = client.generate_presigned_url(ClientMethod='get_object', Params=params, ExpiresIn=-7, HttpMethod='GET')

        res = requests.get(url).__dict__
        self.assertEqual(res['status_code'], 403)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='unauthenticated, no object acls')
    # @attr(assertion='fails 403')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_anon_put(self):
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='foo')

        unauthenticated_client = get_unauthenticated_client()

        e = assert_raises(ClientError, unauthenticated_client.put_object, Bucket=bucket_name, Key='foo', Body='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AccessDenied')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='unauthenticated, publically writable object')
    # @attr(assertion='succeeds')
    # TODO:sequoias3不支持桶和对象的acl
    def test_object_anon_put_write_access(self):
        bucket_name = self._setup_bucket_acl('public-read-write')
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo')

        unauthenticated_client = get_unauthenticated_client()

        response = unauthenticated_client.put_object(Bucket=bucket_name, Key='foo', Body='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='authenticated, no object acls')
    # @attr(assertion='succeeds')
    def test_object_put_authenticated(self):
        bucket_name = get_new_bucket()
        client = get_client()

        response = client.put_object(Bucket=bucket_name, Key='foo', Body='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='authenticated, no object acls')
    # @attr(assertion='succeeds')
    def test_object_raw_put_authenticated_expired(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo')

        params = {'Bucket': bucket_name, 'Key': 'foo'}
        url = client.generate_presigned_url(ClientMethod='put_object', Params=params, ExpiresIn=-1000, HttpMethod='PUT')

        # params wouldn't take a 'Body' parameter so we're passing it in here
        res = requests.put(url, data="foo").__dict__
        self.assertEqual(res['status_code'], 403)

    def check_access_denied(self, fn, *args, **kwargs):
        e = assert_raises(ClientError, fn, *args, **kwargs)
        status = self._get_status(e.response)
        self.assertEqual(status, 403)

    def check_grants(self, got, want):
        """
        Check that grants list in got matches the dictionaries in want,
        in any order.
        """
        self.assertEqual(len(got), len(want))
        for g, w in zip(got, want):
            w = dict(w)
            g = dict(g)
            self.assertEqual(g.pop('Permission', None), w['Permission'])
            self.assertEqual(g['Grantee'].pop('DisplayName', None), w['DisplayName'])
            self.assertEqual(g['Grantee'].pop('ID', None), w['ID'])
            self.assertEqual(g['Grantee'].pop('Type', None), w['Type'])
            self.assertEqual(g['Grantee'].pop('URI', None), w['URI'])
            self.assertEqual(g['Grantee'].pop('EmailAddress', None), w['EmailAddress'])
            self.assertEqual(g, {'Grantee': {}})

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='default acl')
    # @attr(assertion='read back expected defaults')
    def test_bucket_acl_default(self):
        bucket_name = get_new_bucket()
        client = get_client()

        response = client.get_bucket_acl(Bucket=bucket_name)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        self.assertEqual(response['Owner']['DisplayName'], display_name)
        self.assertEqual(response['Owner']['ID'], user_id)

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='public-read acl')
    # @attr(assertion='read back expected defaults')
    # @attr('fails_on_aws') # <Error><Code>IllegalLocationConstraintException</Code><Message>The unspecified location constraint is incompatible for the region specific endpoint this request was sent to.</Message>
    def test_bucket_acl_canned_during_create(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read', Bucket=bucket_name)
        response = client.get_bucket_acl(Bucket=bucket_name)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='acl: public-read,private')
    # @attr(assertion='read back expected values')
    def test_bucket_acl_canned(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read', Bucket=bucket_name)
        response = client.get_bucket_acl(Bucket=bucket_name)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

        client.put_bucket_acl(ACL='private', Bucket=bucket_name)
        response = client.get_bucket_acl(Bucket=bucket_name)

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='bucket.acls')
    # @attr(method='put')
    # @attr(operation='acl: public-read-write')
    # @attr(assertion='read back expected values')
    def test_bucket_acl_canned_publicreadwrite(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)
        response = client.get_bucket_acl(Bucket=bucket_name)

        display_name = get_main_display_name()
        user_id = get_main_user_id()
        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='WRITE',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='acl: authenticated-read')
    # @attr(assertion='read back expected values')
    def test_bucket_acl_canned_authenticatedread(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(ACL='authenticated-read', Bucket=bucket_name)
        response = client.get_bucket_acl(Bucket=bucket_name)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AuthenticatedUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object.acls')
    # @attr(method='get')
    # @attr(operation='default acl')
    # @attr(assertion='read back expected defaults')
    def test_object_acl_default(self):
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object_acl(Bucket=bucket_name, Key='foo')

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object.acls')
    # @attr(method='put')
    # @attr(operation='acl public-read')
    # @attr(assertion='read back expected values')
    def test_object_acl_canned_during_create(self):
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(ACL='public-read', Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object_acl(Bucket=bucket_name, Key='foo')

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object.acls')
    # @attr(method='put')
    # @attr(operation='acl public-read,private')
    # @attr(assertion='read back expected values')
    def test_object_acl_canned(self):
        bucket_name = get_new_bucket()
        client = get_client()

        # Since it defaults to private, set it public-read first
        client.put_object(ACL='public-read', Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object_acl(Bucket=bucket_name, Key='foo')

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

        # Then back to private.
        client.put_object_acl(ACL='private', Bucket=bucket_name, Key='foo')
        response = client.get_object_acl(Bucket=bucket_name, Key='foo')
        grants = response['Grants']

        self.check_grants(
            grants,
            [
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='acl public-read-write')
    # @attr(assertion='read back expected values')
    def test_object_acl_canned_publicreadwrite(self):
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(ACL='public-read-write', Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object_acl(Bucket=bucket_name, Key='foo')

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='WRITE',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object.acls')
    # @attr(method='put')
    # @attr(operation='acl authenticated-read')
    # @attr(assertion='read back expected values')
    def test_object_acl_canned_authenticatedread(self):
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(ACL='authenticated-read', Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object_acl(Bucket=bucket_name, Key='foo')

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AuthenticatedUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object.acls')
    # @attr(method='put')
    # @attr(operation='acl bucket-owner-read')
    # @attr(assertion='read back expected values')
    def test_object_acl_canned_bucketownerread(self):
        bucket_name = get_new_bucket_name()
        main_client = get_client()
        alt_client = get_alt_client()

        main_client.create_bucket(Bucket=bucket_name, ACL='public-read-write')

        alt_client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        bucket_acl_response = main_client.get_bucket_acl(Bucket=bucket_name)
        bucket_owner_id = bucket_acl_response['Grants'][2]['Grantee']['ID']
        bucket_owner_display_name = bucket_acl_response['Grants'][2]['Grantee']['DisplayName']

        alt_client.put_object(ACL='bucket-owner-read', Bucket=bucket_name, Key='foo')
        response = alt_client.get_object_acl(Bucket=bucket_name, Key='foo')

        alt_display_name = get_alt_display_name()
        alt_user_id = get_alt_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='FULL_CONTROL',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='READ',
                    ID=bucket_owner_id,
                    DisplayName=bucket_owner_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object.acls')
    # @attr(method='put')
    # @attr(operation='acl bucket-owner-read')
    # @attr(assertion='read back expected values')
    def test_object_acl_canned_bucketownerfullcontrol(self):
        bucket_name = get_new_bucket_name()
        main_client = get_client()
        alt_client = get_alt_client()

        main_client.create_bucket(Bucket=bucket_name, ACL='public-read-write')

        alt_client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        bucket_acl_response = main_client.get_bucket_acl(Bucket=bucket_name)
        bucket_owner_id = bucket_acl_response['Grants'][2]['Grantee']['ID']
        bucket_owner_display_name = bucket_acl_response['Grants'][2]['Grantee']['DisplayName']

        alt_client.put_object(ACL='bucket-owner-full-control', Bucket=bucket_name, Key='foo')
        response = alt_client.get_object_acl(Bucket=bucket_name, Key='foo')

        alt_display_name = get_alt_display_name()
        alt_user_id = get_alt_user_id()

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='FULL_CONTROL',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=bucket_owner_id,
                    DisplayName=bucket_owner_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object.acls')
    # @attr(method='put')
    # @attr(operation='set write-acp')
    # @attr(assertion='does not modify owner')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${ALTUSER}</ArgumentValue>
    def test_object_acl_full_control_verify_owner(self):
        bucket_name = get_new_bucket_name()
        main_client = get_client()
        alt_client = get_alt_client()

        main_client.create_bucket(Bucket=bucket_name, ACL='public-read-write')

        main_client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        alt_user_id = get_alt_user_id()
        alt_display_name = get_alt_display_name()

        main_user_id = get_main_user_id()
        main_display_name = get_main_display_name()

        grant = {'Grants': [{'Grantee': {'ID': alt_user_id, 'Type': 'CanonicalUser'}, 'Permission': 'FULL_CONTROL'}],
                 'Owner': {'DisplayName': main_display_name, 'ID': main_user_id}}

        main_client.put_object_acl(Bucket=bucket_name, Key='foo', AccessControlPolicy=grant)

        grant = {'Grants': [{'Grantee': {'ID': alt_user_id, 'Type': 'CanonicalUser'}, 'Permission': 'READ_ACP'}],
                 'Owner': {'DisplayName': main_display_name, 'ID': main_user_id}}

        alt_client.put_object_acl(Bucket=bucket_name, Key='foo', AccessControlPolicy=grant)

        response = alt_client.get_object_acl(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['Owner']['ID'], main_user_id)

    def add_obj_user_grant(self, bucket_name, key, grant):
        """
        Adds a grant to the existing grants meant to be passed into
        the AccessControlPolicy argument of put_object_acls for an object
        owned by the main user, not the alt user
        A grant is a dictionary in the form of:
        {u'Grantee': {u'Type': 'type', u'DisplayName': 'name', u'ID': 'id'}, u'Permission': 'PERM'}

        """
        client = get_client()
        main_user_id = get_main_user_id()
        main_display_name = get_main_display_name()

        response = client.get_object_acl(Bucket=bucket_name, Key=key)

        grants = response['Grants']
        grants.append(grant)

        grant = {'Grants': grants, 'Owner': {'DisplayName': main_display_name, 'ID': main_user_id}}

        return grant

    # @attr(resource='object.acls')
    # @attr(method='put')
    # @attr(operation='set write-acp')
    # @attr(assertion='does not modify other attributes')
    def test_object_acl_full_control_verify_attributes(self):
        bucket_name = get_new_bucket_name()
        main_client = get_client()
        alt_client = get_alt_client()

        main_client.create_bucket(Bucket=bucket_name, ACL='public-read-write')

        header = {'x-amz-foo': 'bar'}
        # lambda to add any header
        add_header = (lambda **kwargs: kwargs['params']['headers'].update(header))

        main_client.meta.events.register('before-call.s3.PutObject', add_header)
        main_client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = main_client.get_object(Bucket=bucket_name, Key='foo')
        content_type = response['ContentType']
        etag = response['ETag']

        alt_user_id = get_alt_user_id()

        grant = {'Grantee': {'ID': alt_user_id, 'Type': 'CanonicalUser'}, 'Permission': 'FULL_CONTROL'}

        grants = self.add_obj_user_grant(bucket_name, 'foo', grant)

        main_client.put_object_acl(Bucket=bucket_name, Key='foo', AccessControlPolicy=grants)

        response = main_client.get_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(content_type, response['ContentType'])
        self.assertEqual(etag, response['ETag'])

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='set acl private')
    # @attr(assertion='a private object can be set to private')
    def test_bucket_acl_canned_private_to_private(self):
        bucket_name = get_new_bucket()
        client = get_client()

        response = client.put_bucket_acl(Bucket=bucket_name, ACL='private')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    def add_bucket_user_grant(self, bucket_name, grant):
        """
        Adds a grant to the existing grants meant to be passed into
        the AccessControlPolicy argument of put_object_acls for an object
        owned by the main user, not the alt user
        A grant is a dictionary in the form of:
        {u'Grantee': {u'Type': 'type', u'DisplayName': 'name', u'ID': 'id'}, u'Permission': 'PERM'}
        """
        client = get_client()
        main_user_id = get_main_user_id()
        main_display_name = get_main_display_name()

        response = client.get_bucket_acl(Bucket=bucket_name)

        grants = response['Grants']
        grants.append(grant)

        grant = {'Grants': grants, 'Owner': {'DisplayName': main_display_name, 'ID': main_user_id}}

        return grant

    def _check_object_acl(self, permission):
        """
        Sets the permission on an object then checks to see
        if it was set
        """
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object_acl(Bucket=bucket_name, Key='foo')

        policy = {}
        policy['Owner'] = response['Owner']
        policy['Grants'] = response['Grants']
        policy['Grants'][0]['Permission'] = permission

        client.put_object_acl(Bucket=bucket_name, Key='foo', AccessControlPolicy=policy)

        response = client.get_object_acl(Bucket=bucket_name, Key='foo')
        grants = response['Grants']

        main_user_id = get_main_user_id()
        main_display_name = get_main_display_name()
        self.check_grants(
            grants,
            [
                dict(
                    Permission=permission,
                    ID=main_user_id,
                    DisplayName=main_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='set acl FULL_CONTRO')
    # @attr(assertion='reads back correctly')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${USER}</ArgumentValue>
    def test_object_acl(self):
        self._check_object_acl('FULL_CONTROL')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='set acl WRITE')
    # @attr(assertion='reads back correctly')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${USER}</ArgumentValue>
    def test_object_acl_write(self):
        self._check_object_acl('WRITE')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='set acl WRITE_ACP')
    # @attr(assertion='reads back correctly')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${USER}</ArgumentValue>
    def test_object_acl_writeacp(self):
        self._check_object_acl('WRITE_ACP')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='set acl READ')
    # @attr(assertion='reads back correctly')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${USER}</ArgumentValue>
    def test_object_acl_read(self):
        self._check_object_acl('READ')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='set acl READ_ACP')
    # @attr(assertion='reads back correctly')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${USER}</ArgumentValue>
    def test_object_acl_readacp(self):
        self._check_object_acl('READ_ACP')

    def _bucket_acl_grant_userid(self, permission):
        """
        create a new bucket, grant a specific user the specified
        permission, read back the acl and verify correct setting
        """
        bucket_name = get_new_bucket()
        client = get_client()

        main_user_id = get_main_user_id()
        main_display_name = get_main_display_name()

        alt_user_id = get_alt_user_id()
        alt_display_name = get_alt_display_name()

        grant = {'Grantee': {'ID': alt_user_id, 'Type': 'CanonicalUser'}, 'Permission': permission}

        grant = self.add_bucket_user_grant(bucket_name, grant)

        client.put_bucket_acl(Bucket=bucket_name, AccessControlPolicy=grant)

        response = client.get_bucket_acl(Bucket=bucket_name)

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission=permission,
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=main_user_id,
                    DisplayName=main_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

        return bucket_name

    def _check_bucket_acl_grant_can_read(self, bucket_name):
        """
        verify ability to read the specified bucket
        """
        alt_client = get_alt_client()
        response = alt_client.head_bucket(Bucket=bucket_name)

    def _check_bucket_acl_grant_cant_read(self, bucket_name):
        """
        verify inability to read the specified bucket
        """
        alt_client = get_alt_client()
        self.check_access_denied(alt_client.head_bucket, Bucket=bucket_name)

    def _check_bucket_acl_grant_can_readacp(self, bucket_name):
        """
        verify ability to read acls on specified bucket
        """
        alt_client = get_alt_client()
        alt_client.get_bucket_acl(Bucket=bucket_name)

    def _check_bucket_acl_grant_cant_readacp(self, bucket_name):
        """
        verify inability to read acls on specified bucket
        """
        alt_client = get_alt_client()
        self.check_access_denied(alt_client.get_bucket_acl, Bucket=bucket_name)

    def _check_bucket_acl_grant_can_write(self, bucket_name):
        """
        verify ability to write the specified bucket
        """
        alt_client = get_alt_client()
        alt_client.put_object(Bucket=bucket_name, Key='foo-write', Body='bar')

    def _check_bucket_acl_grant_cant_write(self, bucket_name):
        """
        verify inability to write the specified bucket
        """
        alt_client = get_alt_client()
        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key='foo-write', Body='bar')

    def _check_bucket_acl_grant_can_writeacp(self, bucket_name):
        """
        verify ability to set acls on the specified bucket
        """
        alt_client = get_alt_client()
        alt_client.put_bucket_acl(Bucket=bucket_name, ACL='public-read')

    def _check_bucket_acl_grant_cant_writeacp(self, bucket_name):
        """
        verify inability to set acls on the specified bucket
        """
        alt_client = get_alt_client()
        self.check_access_denied(alt_client.put_bucket_acl, Bucket=bucket_name, ACL='public-read')

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='set acl w/userid FULL_CONTROL')
    # @attr(assertion='can read/write data/acls')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${USER}</ArgumentValue>
    def test_bucket_acl_grant_userid_fullcontrol(self):
        bucket_name = self._bucket_acl_grant_userid('FULL_CONTROL')

        # alt user can read
        self._check_bucket_acl_grant_can_read(bucket_name)
        # can read acl
        self._check_bucket_acl_grant_can_readacp(bucket_name)
        # can write
        self._check_bucket_acl_grant_can_write(bucket_name)
        # can write acl
        self._check_bucket_acl_grant_can_writeacp(bucket_name)

        client = get_client()

        bucket_acl_response = client.get_bucket_acl(Bucket=bucket_name)
        owner_id = bucket_acl_response['Owner']['ID']
        owner_display_name = bucket_acl_response['Owner']['DisplayName']

        main_display_name = get_main_display_name()
        main_user_id = get_main_user_id()

        self.assertEqual(owner_id, main_user_id)
        self.assertEqual(owner_display_name, main_display_name)

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='set acl w/userid READ')
    # @attr(assertion='can read data, no other r/w')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${ALTUSER}</ArgumentValue>
    def test_bucket_acl_grant_userid_read(self):
        bucket_name = self._bucket_acl_grant_userid('READ')

        # alt user can read
        self._check_bucket_acl_grant_can_read(bucket_name)
        # can't read acl
        self._check_bucket_acl_grant_cant_readacp(bucket_name)
        # can't write
        self._check_bucket_acl_grant_cant_write(bucket_name)
        # can't write acl
        self._check_bucket_acl_grant_cant_writeacp(bucket_name)

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='set acl w/userid READ_ACP')
    # @attr(assertion='can read acl, no other r/w')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${ALTUSER}</ArgumentValue>
    def test_bucket_acl_grant_userid_readacp(self):
        bucket_name = self._bucket_acl_grant_userid('READ_ACP')

        # alt user can't read
        self._check_bucket_acl_grant_cant_read(bucket_name)
        # can read acl
        self._check_bucket_acl_grant_can_readacp(bucket_name)
        # can't write
        self._check_bucket_acl_grant_cant_write(bucket_name)
        # can't write acp
        # _check_bucket_acl_grant_cant_writeacp_can_readacp(bucket)
        self._check_bucket_acl_grant_cant_writeacp(bucket_name)

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='set acl w/userid WRITE')
    # @attr(assertion='can write data, no other r/w')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${ALTUSER}</ArgumentValue>
    def test_bucket_acl_grant_userid_write(self):
        bucket_name = self._bucket_acl_grant_userid('WRITE')

        # alt user can't read
        self._check_bucket_acl_grant_cant_read(bucket_name)
        # can't read acl
        self._check_bucket_acl_grant_cant_readacp(bucket_name)
        # can write
        self._check_bucket_acl_grant_can_write(bucket_name)
        # can't write acl
        self._check_bucket_acl_grant_cant_writeacp(bucket_name)

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='set acl w/userid WRITE_ACP')
    # @attr(assertion='can write acls, no other r/w')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${ALTUSER}</ArgumentValue>
    def test_bucket_acl_grant_userid_writeacp(self):
        bucket_name = self._bucket_acl_grant_userid('WRITE_ACP')

        # alt user can't read
        self._check_bucket_acl_grant_cant_read(bucket_name)
        # can't read acl
        self._check_bucket_acl_grant_cant_readacp(bucket_name)
        # can't write
        self._check_bucket_acl_grant_cant_write(bucket_name)
        # can write acl
        self._check_bucket_acl_grant_can_writeacp(bucket_name)

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='set acl w/invalid userid')
    # @attr(assertion='fails 400')
    def test_bucket_acl_grant_nonexist_user(self):
        bucket_name = get_new_bucket()
        client = get_client()

        bad_user_id = '_foo'

        # response = client.get_bucket_acl(Bucket=bucket_name)
        grant = {'Grantee': {'ID': bad_user_id, 'Type': 'CanonicalUser'}, 'Permission': 'FULL_CONTROL'}

        grant = self.add_bucket_user_grant(bucket_name, grant)

        e = assert_raises(ClientError, client.put_bucket_acl, Bucket=bucket_name, AccessControlPolicy=grant)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidArgument')

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='revoke all ACLs')
    # @attr(assertion='can: read obj, get/set bucket acl, cannot write objs')
    def test_bucket_acl_no_grants(self):
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_bucket_acl(Bucket=bucket_name)
        old_grants = response['Grants']
        policy = {}
        policy['Owner'] = response['Owner']
        # clear grants
        policy['Grants'] = []

        # remove read/write permission
        response = client.put_bucket_acl(Bucket=bucket_name, AccessControlPolicy=policy)

        # can read
        client.get_object(Bucket=bucket_name, Key='foo')

        # can't write
        self.check_access_denied(client.put_object, Bucket=bucket_name, Key='baz', Body='a')

        # TODO fix this test once a fix is in for same issues in
        # test_access_bucket_private_object_private
        client2 = get_client()
        # owner can read acl
        client2.get_bucket_acl(Bucket=bucket_name)

        # owner can write acl
        client2.put_bucket_acl(Bucket=bucket_name, ACL='private')

        # set policy back to original so that bucket can be cleaned up
        policy['Grants'] = old_grants
        client2.put_bucket_acl(Bucket=bucket_name, AccessControlPolicy=policy)

    def _get_acl_header(self, user_id=None, perms=None):
        all_headers = ["read", "write", "read-acp", "write-acp", "full-control"]
        headers = []

        if user_id == None:
            user_id = get_alt_user_id()

        if perms != None:
            for perm in perms:
                header = ("x-amz-grant-{perm}".format(perm=perm), "id={uid}".format(uid=user_id))
                headers.append(header)

        else:
            for perm in all_headers:
                header = ("x-amz-grant-{perm}".format(perm=perm), "id={uid}".format(uid=user_id))
                headers.append(header)

        return headers

    # @attr(resource='object')
    # @attr(method='PUT')
    # @attr(operation='add all grants to user through headers')
    # @attr(assertion='adds all grants individually to second user')
    # @attr('fails_on_dho')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${ALTUSER}</ArgumentValue>
    def test_object_header_acl_grants(self):
        bucket_name = get_new_bucket()
        client = get_client()

        alt_user_id = get_alt_user_id()
        alt_display_name = get_alt_display_name()

        headers = self._get_acl_header()

        def add_headers_before_sign(**kwargs):
            updated_headers = (kwargs['request'].__dict__['headers'].__dict__['_headers'] + headers)
            kwargs['request'].__dict__['headers'].__dict__['_headers'] = updated_headers

        client.meta.events.register('before-sign.s3.PutObject', add_headers_before_sign)

        client.put_object(Bucket=bucket_name, Key='foo_key', Body='bar')

        response = client.get_object_acl(Bucket=bucket_name, Key='foo_key')

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='WRITE',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='READ_ACP',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='WRITE_ACP',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    # @attr(resource='bucket')
    # @attr(method='PUT')
    # @attr(operation='add all grants to user through headers')
    # @attr(assertion='adds all grants individually to second user')
    # @attr('fails_on_dho')
    # @attr('fails_on_aws') #  <Error><Code>InvalidArgument</Code><Message>Invalid id</Message><ArgumentName>CanonicalUser/ID</ArgumentName><ArgumentValue>${ALTUSER}</ArgumentValue>
    def test_bucket_header_acl_grants(self):
        headers = self._get_acl_header()
        bucket_name = get_new_bucket_name()
        client = get_client()

        headers = self._get_acl_header()

        def add_headers_before_sign(**kwargs):
            updated_headers = (kwargs['request'].__dict__['headers'].__dict__['_headers'] + headers)
            kwargs['request'].__dict__['headers'].__dict__['_headers'] = updated_headers

        client.meta.events.register('before-sign.s3.CreateBucket', add_headers_before_sign)

        client.create_bucket(Bucket=bucket_name)

        response = client.get_bucket_acl(Bucket=bucket_name)

        grants = response['Grants']
        alt_user_id = get_alt_user_id()
        alt_display_name = get_alt_display_name()

        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='WRITE',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='READ_ACP',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='WRITE_ACP',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

        alt_client = get_alt_client()

        alt_client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        # set bucket acl to public-read-write so that teardown can work
        alt_client.put_bucket_acl(Bucket=bucket_name, ACL='public-read-write')

    # This test will fail on DH Objects. DHO allows multiple users with one account, which
    # would violate the uniqueness requirement of a user's email. As such, DHO users are
    # created without an email.
    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='add second FULL_CONTROL user')
    # @attr(assertion='works for S3, fails for DHO')
    # @attr('fails_on_aws') #  <Error><Code>AmbiguousGrantByEmailAddress</Code><Message>The e-mail address you provided is associated with more than one account. Please retry your request using a different identification method or after resolving the ambiguity.</Message>
    def test_bucket_acl_grant_email(self):
        bucket_name = get_new_bucket()
        client = get_client()

        alt_user_id = get_alt_user_id()
        alt_display_name = get_alt_display_name()
        alt_email_address = get_alt_email()

        main_user_id = get_main_user_id()
        main_display_name = get_main_display_name()

        grant = {'Grantee': {'EmailAddress': alt_email_address, 'Type': 'AmazonCustomerByEmail'},
                 'Permission': 'FULL_CONTROL'}

        grant = self.add_bucket_user_grant(bucket_name, grant)

        client.put_bucket_acl(Bucket=bucket_name, AccessControlPolicy=grant)

        response = client.get_bucket_acl(Bucket=bucket_name)

        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='FULL_CONTROL',
                    ID=alt_user_id,
                    DisplayName=alt_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=main_user_id,
                    DisplayName=main_display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ]
        )

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='add acl for nonexistent user')
    # @attr(assertion='fail 400')
    def test_bucket_acl_grant_email_notexist(self):
        # behavior not documented by amazon
        bucket_name = get_new_bucket()
        client = get_client()

        alt_user_id = get_alt_user_id()
        alt_display_name = get_alt_display_name()
        alt_email_address = get_alt_email()

        NONEXISTENT_EMAIL = 'doesnotexist@dreamhost.com.invalid'
        grant = {'Grantee': {'EmailAddress': NONEXISTENT_EMAIL, 'Type': 'AmazonCustomerByEmail'},
                 'Permission': 'FULL_CONTROL'}

        grant = self.add_bucket_user_grant(bucket_name, grant)

        e = assert_raises(ClientError, client.put_bucket_acl, Bucket=bucket_name, AccessControlPolicy=grant)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'UnresolvableGrantByEmailAddress')

    # @attr(resource='bucket')
    # @attr(method='ACLs')
    # @attr(operation='revoke all ACLs')
    # @attr(assertion='acls read back as empty')
    def test_bucket_acl_revoke_all(self):
        # revoke all access, including the owner's access
        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_bucket_acl(Bucket=bucket_name)
        old_grants = response['Grants']
        policy = {}
        policy['Owner'] = response['Owner']
        # clear grants
        policy['Grants'] = []

        # remove read/write permission for everyone
        client.put_bucket_acl(Bucket=bucket_name, AccessControlPolicy=policy)

        response = client.get_bucket_acl(Bucket=bucket_name)

        self.assertEqual(len(response['Grants']), 0)

        # set policy back to original so that bucket can be cleaned up
        policy['Grants'] = old_grants
        client.put_bucket_acl(Bucket=bucket_name, AccessControlPolicy=policy)

    # TODO rgw log_bucket.set_as_logging_target() gives 403 Forbidden
    # http://tracker.newdream.net/issues/984
    # @attr(resource='bucket.log')
    # @attr(method='put')
    # @attr(operation='set/enable/disable logging target')
    # @attr(assertion='operations succeed')
    # @attr('fails_on_rgw')
    def test_logging_toggle(self):
        bucket_name = get_new_bucket()
        client = get_client()

        main_display_name = get_main_display_name()
        main_user_id = get_main_user_id()

        status = {'LoggingEnabled': {'TargetBucket': bucket_name, 'TargetGrants': [
            {'Grantee': {'DisplayName': main_display_name, 'ID': main_user_id, 'Type': 'CanonicalUser'},
             'Permission': 'FULL_CONTROL'}], 'TargetPrefix': 'foologgingprefix'}}

        client.put_bucket_logging(Bucket=bucket_name, BucketLoggingStatus=status)
        client.get_bucket_logging(Bucket=bucket_name)
        status = {'LoggingEnabled': {}}
        client.put_bucket_logging(Bucket=bucket_name, BucketLoggingStatus=status)
        # NOTE: this does not actually test whether or not logging works

    def _setup_access(self, bucket_acl, object_acl):
        """
        Simple test fixture: create a bucket with given ACL, with objects:
        - a: owning user, given ACL
        - a2: same object accessed by some other user
        - b: owning user, default ACL in bucket w/given ACL
        - b2: same object accessed by a some other user
        """
        bucket_name = get_new_bucket()
        client = get_client()

        key1 = 'foo'
        key2 = 'bar'
        newkey = 'new'

        client.put_bucket_acl(Bucket=bucket_name, ACL=bucket_acl)
        client.put_object(Bucket=bucket_name, Key=key1, Body='foocontent')
        client.put_object_acl(Bucket=bucket_name, Key=key1, ACL=object_acl)
        client.put_object(Bucket=bucket_name, Key=key2, Body='barcontent')

        return bucket_name, key1, key2, newkey

    def get_bucket_key_names(self, bucket_name):
        objs_list = get_objects_list(bucket_name)
        return frozenset(obj for obj in objs_list)

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: private/private')
        # @attr(assertion='public has no access to bucket or objects')

    def test_access_bucket_private_object_private(self):
        # all the test_access_* tests follow this template
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='private', object_acl='private')

        alt_client = get_alt_client()
        # acled object read fail
        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key1)
        # default object read fail
        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key2)
        # bucket read fail
        self.check_access_denied(alt_client.list_objects, Bucket=bucket_name)

        # acled object write fail
        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='barcontent')
        # NOTE: The above put's causes the connection to go bad, therefore the client can't be used
        # anymore. This can be solved either by:
        # 1) putting an empty string ('') in the 'Body' field of those put_object calls
        # 2) getting a new client hence the creation of alt_client{2,3} for the tests below
        # TODO: Test it from another host and on AWS, Report this to Amazon, if findings are identical

        alt_client2 = get_alt_client()
        # default object write fail
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')
        # bucket write fail
        alt_client3 = get_alt_client()
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: private/private with list-objects-v2')
        # @attr(assertion='public has no access to bucket or objects')
        # @attr('list-objects-v2')

    def test_access_bucket_private_objectv2_private(self):
        # all the test_access_* tests follow this template
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='private', object_acl='private')

        alt_client = get_alt_client()
        # acled object read fail
        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key1)
        # default object read fail
        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key2)
        # bucket read fail
        self.check_access_denied(alt_client.list_objects_v2, Bucket=bucket_name)

        # acled object write fail
        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='barcontent')
        # NOTE: The above put's causes the connection to go bad, therefore the client can't be used
        # anymore. This can be solved either by:
        # 1) putting an empty string ('') in the 'Body' field of those put_object calls
        # 2) getting a new client hence the creation of alt_client{2,3} for the tests below
        # TODO: Test it from another host and on AWS, Report this to Amazon, if findings are identical

        alt_client2 = get_alt_client()
        # default object write fail
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')
        # bucket write fail
        alt_client3 = get_alt_client()
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: private/public-read')
        # @attr(assertion='public can only read readable object')

    def test_access_bucket_private_object_publicread(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='private', object_acl='public-read')
        alt_client = get_alt_client()
        response = alt_client.get_object(Bucket=bucket_name, Key=key1)

        body = self._get_body(response)

        # a should be public-read, b gets default (private)
        self.assertEqual(body, 'foocontent')

        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='foooverwrite')
        alt_client2 = get_alt_client()
        self.check_access_denied(alt_client2.get_object, Bucket=bucket_name, Key=key2)
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')

        alt_client3 = get_alt_client()
        self.check_access_denied(alt_client3.list_objects, Bucket=bucket_name)
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: private/public-read with list-objects-v2')
        # @attr(assertion='public can only read readable object')
        # @attr('list-objects-v2')

    def test_access_bucket_private_objectv2_publicread(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='private', object_acl='public-read')
        alt_client = get_alt_client()
        response = alt_client.get_object(Bucket=bucket_name, Key=key1)

        body = self._get_body(response)

        # a should be public-read, b gets default (private)
        self.assertEqual(body, 'foocontent')

        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='foooverwrite')
        alt_client2 = get_alt_client()
        self.check_access_denied(alt_client2.get_object, Bucket=bucket_name, Key=key2)
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')

        alt_client3 = get_alt_client()
        self.check_access_denied(alt_client3.list_objects_v2, Bucket=bucket_name)
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: private/public-read/write')
        # @attr(assertion='public can only read the readable object')

    def test_access_bucket_private_object_publicreadwrite(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='private', object_acl='public-read-write')
        alt_client = get_alt_client()
        response = alt_client.get_object(Bucket=bucket_name, Key=key1)

        body = self._get_body(response)

        # a should be public-read-only ... because it is in a private bucket
        # b gets default (private)
        self.assertEqual(body, 'foocontent')

        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='foooverwrite')
        alt_client2 = get_alt_client()
        self.check_access_denied(alt_client2.get_object, Bucket=bucket_name, Key=key2)
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')

        alt_client3 = get_alt_client()
        self.check_access_denied(alt_client3.list_objects, Bucket=bucket_name)
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: private/public-read/write with list-objects-v2')
        # @attr(assertion='public can only read the readable object')
        # @attr('list-objects-v2')

    def test_access_bucket_private_objectv2_publicreadwrite(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='private', object_acl='public-read-write')
        alt_client = get_alt_client()
        response = alt_client.get_object(Bucket=bucket_name, Key=key1)

        body = self._get_body(response)

        # a should be public-read-only ... because it is in a private bucket
        # b gets default (private)
        self.assertEqual(body, 'foocontent')

        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='foooverwrite')
        alt_client2 = get_alt_client()
        self.check_access_denied(alt_client2.get_object, Bucket=bucket_name, Key=key2)
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')

        alt_client3 = get_alt_client()
        self.check_access_denied(alt_client3.list_objects_v2, Bucket=bucket_name)
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: public-read/private')
        # @attr(assertion='public can only list the bucket')

    def test_access_bucket_publicread_object_private(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='public-read', object_acl='private')
        alt_client = get_alt_client()

        # a should be private, b gets default (private)
        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key1)
        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='barcontent')

        alt_client2 = get_alt_client()
        self.check_access_denied(alt_client2.get_object, Bucket=bucket_name, Key=key2)
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')

        alt_client3 = get_alt_client()

        objs = get_objects_list(bucket=bucket_name, client=alt_client3)

        self.assertEqual(objs, [u'bar', u'foo'])
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: public-read/public-read')
        # @attr(assertion='public can read readable objects and list bucket')

    def test_access_bucket_publicread_object_publicread(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='public-read', object_acl='public-read')
        alt_client = get_alt_client()

        response = alt_client.get_object(Bucket=bucket_name, Key=key1)

        # a should be public-read, b gets default (private)
        body = self._get_body(response)
        self.assertEqual(body, 'foocontent')

        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='foooverwrite')

        alt_client2 = get_alt_client()
        self.check_access_denied(alt_client2.get_object, Bucket=bucket_name, Key=key2)
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')

        alt_client3 = get_alt_client()

        objs = get_objects_list(bucket=bucket_name, client=alt_client3)

        self.assertEqual(objs, [u'bar', u'foo'])
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: public-read/public-read-write')
        # @attr(assertion='public can read readable objects and list bucket')

    def test_access_bucket_publicread_object_publicreadwrite(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='public-read', object_acl='public-read-write')
        alt_client = get_alt_client()

        response = alt_client.get_object(Bucket=bucket_name, Key=key1)

        body = self._get_body(response)

        # a should be public-read-only ... because it is in a r/o bucket
        # b gets default (private)
        self.assertEqual(body, 'foocontent')

        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1, Body='foooverwrite')

        alt_client2 = get_alt_client()
        self.check_access_denied(alt_client2.get_object, Bucket=bucket_name, Key=key2)
        self.check_access_denied(alt_client2.put_object, Bucket=bucket_name, Key=key2, Body='baroverwrite')

        alt_client3 = get_alt_client()

        objs = get_objects_list(bucket=bucket_name, client=alt_client3)

        self.assertEqual(objs, [u'bar', u'foo'])
        self.check_access_denied(alt_client3.put_object, Bucket=bucket_name, Key=newkey, Body='newcontent')

        # @attr(resource='object')
        # @attr(method='ACLs')
        # @attr(operation='set bucket/object acls: public-read-write/private')
        # @attr(assertion='private objects cannot be read, but can be overwritten')

    def test_access_bucket_publicreadwrite_object_private(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='public-read-write', object_acl='private')
        alt_client = get_alt_client()

        # a should be private, b gets default (private)
        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key1)
        alt_client.put_object(Bucket=bucket_name, Key=key1, Body='barcontent')

        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key2)
        alt_client.put_object(Bucket=bucket_name, Key=key2, Body='baroverwrite')

        objs = get_objects_list(bucket=bucket_name, client=alt_client)
        self.assertEqual(objs, [u'bar', u'foo'])
        alt_client.put_object(Bucket=bucket_name, Key=newkey, Body='newcontent')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='set bucket/object acls: public-read-write/public-read')
    # @attr(assertion='private objects cannot be read, but can be overwritten')
    def test_access_bucket_publicreadwrite_object_publicread(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='public-read-write', object_acl='public-read')
        alt_client = get_alt_client()

        # a should be public-read, b gets default (private)
        response = alt_client.get_object(Bucket=bucket_name, Key=key1)

        body = self._get_body(response)
        self.assertEqual(body, 'foocontent')
        alt_client.put_object(Bucket=bucket_name, Key=key1, Body='barcontent')

        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key2)
        alt_client.put_object(Bucket=bucket_name, Key=key2, Body='baroverwrite')

        objs = get_objects_list(bucket=bucket_name, client=alt_client)
        self.assertEqual(objs, [u'bar', u'foo'])
        alt_client.put_object(Bucket=bucket_name, Key=newkey, Body='newcontent')

    # @attr(resource='object')
    # @attr(method='ACLs')
    # @attr(operation='set bucket/object acls: public-read-write/public-read-write')
    # @attr(assertion='private objects cannot be read, but can be overwritten')
    def test_access_bucket_publicreadwrite_object_publicreadwrite(self):
        bucket_name, key1, key2, newkey = self._setup_access(bucket_acl='public-read-write',
                                                             object_acl='public-read-write')
        alt_client = get_alt_client()
        response = alt_client.get_object(Bucket=bucket_name, Key=key1)
        body = self._get_body(response)

        # a should be public-read-write, b gets default (private)
        self.assertEqual(body, 'foocontent')
        alt_client.put_object(Bucket=bucket_name, Key=key1, Body='foooverwrite')
        self.check_access_denied(alt_client.get_object, Bucket=bucket_name, Key=key2)
        alt_client.put_object(Bucket=bucket_name, Key=key2, Body='baroverwrite')
        objs = get_objects_list(bucket=bucket_name, client=alt_client)
        self.assertEqual(objs, [u'bar', u'foo'])
        alt_client.put_object(Bucket=bucket_name, Key=newkey, Body='newcontent')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='change acl on an object version changes specific version')
    # @attr(assertion='works')
    # @attr('versioning')
    # TODO:sequoias3目前不支持桶和对象的acl
    def test_versioned_object_acl(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'xyz'
        num_versions = 3

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        version_id = version_ids[1]

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        self.assertEqual(response['Owner']['DisplayName'], display_name)
        self.assertEqual(response['Owner']['ID'], user_id)

        grants = response['Grants']
        default_policy = [
            dict(
                Permission='FULL_CONTROL',
                ID=user_id,
                DisplayName=display_name,
                URI=None,
                EmailAddress=None,
                Type='CanonicalUser',
            ),
        ]

        self.check_grants(grants, default_policy)

        client.put_object_acl(ACL='public-read', Bucket=bucket_name, Key=key, VersionId=version_id)

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)
        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

        client.put_object(Bucket=bucket_name, Key=key)

        response = client.get_object_acl(Bucket=bucket_name, Key=key)
        grants = response['Grants']
        self.check_grants(grants, default_policy)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='change acl on an object with no version specified changes latest version')
    # @attr(assertion='works')
    # @attr('versioning')
    # TODO:sequoias3目前不支持桶和对象的acl
    def test_versioned_object_acl_no_version_specified(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'xyz'
        num_versions = 3

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        response = client.get_object(Bucket=bucket_name, Key=key)
        version_id = response['VersionId']

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        self.assertEqual(response['Owner']['DisplayName'], display_name)
        self.assertEqual(response['Owner']['ID'], user_id)

        grants = response['Grants']
        default_policy = [
            dict(
                Permission='FULL_CONTROL',
                ID=user_id,
                DisplayName=display_name,
                URI=None,
                EmailAddress=None,
                Type='CanonicalUser',
            ),
        ]

        self.check_grants(grants, default_policy)

        client.put_object_acl(ACL='public-read', Bucket=bucket_name, Key=key)

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)
        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

        def test_versioned_object_acl_no_version_specified(self):
            bucket_name = get_new_bucket()

        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'xyz'
        num_versions = 3

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        response = client.get_object(Bucket=bucket_name, Key=key)
        version_id = response['VersionId']

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        self.assertEqual(response['Owner']['DisplayName'], display_name)
        self.assertEqual(response['Owner']['ID'], user_id)

        grants = response['Grants']
        default_policy = [
            dict(
                Permission='FULL_CONTROL',
                ID=user_id,
                DisplayName=display_name,
                URI=None,
                EmailAddress=None,
                Type='CanonicalUser',
            ),
        ]

        self.check_grants(grants, default_policy)

        client.put_object_acl(ACL='public-read', Bucket=bucket_name, Key=key)

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)
        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    def test_versioned_object_acl_no_version_specified(self):
        bucket_name = get_new_bucket()

        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'xyz'
        num_versions = 3

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        response = client.get_object(Bucket=bucket_name, Key=key)
        version_id = response['VersionId']

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)

        display_name = get_main_display_name()
        user_id = get_main_user_id()

        self.assertEqual(response['Owner']['DisplayName'], display_name)
        self.assertEqual(response['Owner']['ID'], user_id)

        grants = response['Grants']
        default_policy = [
            dict(
                Permission='FULL_CONTROL',
                ID=user_id,
                DisplayName=display_name,
                URI=None,
                EmailAddress=None,
                Type='CanonicalUser',
            ),
        ]

        self.check_grants(grants, default_policy)

        client.put_object_acl(ACL='public-read', Bucket=bucket_name, Key=key)

        response = client.get_object_acl(Bucket=bucket_name, Key=key, VersionId=version_id)
        grants = response['Grants']
        self.check_grants(
            grants,
            [
                dict(
                    Permission='READ',
                    ID=None,
                    DisplayName=None,
                    URI='http://acs.amazonaws.com/groups/global/AllUsers',
                    EmailAddress=None,
                    Type='Group',
                ),
                dict(
                    Permission='FULL_CONTROL',
                    ID=user_id,
                    DisplayName=display_name,
                    URI=None,
                    EmailAddress=None,
                    Type='CanonicalUser',
                ),
            ],
        )

    def setUp(self):
        setup()

    def tearDown(self):
        teardown()


if __name__ == '__main__':
    unittest.main()
