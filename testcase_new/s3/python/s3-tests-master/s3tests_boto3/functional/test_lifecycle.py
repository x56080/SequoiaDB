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


class TestLifeCycle(unittest.TestCase):

    @staticmethod
    def _create_objects(bucket=None, bucket_name=None, keys=[]):
        """
        Populate a (specified or new) bucket with objects with
        specified names (and contents identical to their names).
        """
        if bucket_name is None:
            bucket_name = get_new_bucket_name()
        if bucket is None:
            bucket = get_new_bucket_resource(name=bucket_name)

        for key in keys:
            obj = bucket.put_object(Body=key, Key=key)

        return bucket_name

    # amazon is eventually consistent, retry a bit if failed
    def check_configure_versioning_retry(self, bucket_name, status, expected_string):
        client = get_client()

        response = client.put_bucket_versioning(Bucket=bucket_name,
                                                VersioningConfiguration={'MFADelete': 'Disabled', 'Status': status})

        read_status = None

        for i in range(5):
            try:
                response = client.get_bucket_versioning(Bucket=bucket_name)
                read_status = response['Status']
            except KeyError:
                read_status = None

            if expected_string == read_status:
                break

            time.sleep(1)

        self.assertEqual(expected_string, read_status)

    def check_obj_content(self, client, bucket_name, key, version_id, content):
        response = client.get_object(Bucket=bucket_name, Key=key, VersionId=version_id)
        if content is not None:
            body = self._get_body(response)
            self.assertEqual(body, content.encode())
        else:
            self.assertEqual(response['DeleteMarker'], True)

    def check_obj_versions(self, client, bucket_name, key, version_ids, contents):
        # check to see if objects is pointing at correct version

        response = client.list_object_versions(Bucket=bucket_name)
        versions = response['Versions']
        # obj versions in versions come out created last to first not first to last like version_ids & contents
        versions.reverse()
        i = 0

        for version in versions:
            self.assertEqual(version['VersionId'], version_ids[i])
            self.assertEqual(version['Key'], key)
            self.check_obj_content(client, bucket_name, key, version['VersionId'], contents[i])
            i += 1

    def create_multiple_versions(self, client, bucket_name, key, num_versions, version_ids=None, contents=None,
                                 check_versions=True):
        contents = contents or []
        version_ids = version_ids or []

        for i in range(num_versions):
            print('i = ', i)
            body = 'content-{i}'.format(i=i)
            response = client.put_object(Bucket=bucket_name, Key=key, Body=body)
            version_id = response['VersionId']

            contents.append(body)
            version_ids.append(version_id)

        if check_versions:
            self.check_obj_versions(client, bucket_name, key, version_ids, contents)

        return version_ids, contents

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config')
    # @attr('lifecycle')
    # TODO:sequoias3不支持
    def test_lifecycle_set(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Days': 1}, 'Prefix': 'test1/', 'Status': 'Enabled'},
                 {'ID': 'rule2', 'Expiration': {'Days': 2}, 'Prefix': 'test2/', 'Status': 'Disabled'}]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='get lifecycle config')
    # @attr('lifecycle')
    # TODO:sequoias3不支持
    def test_lifecycle_get(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'test1/', 'Expiration': {'Days': 31}, 'Prefix': 'test1/', 'Status': 'Enabled'},
                 {'ID': 'test2/', 'Expiration': {'Days': 120}, 'Prefix': 'test2/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        response = client.get_bucket_lifecycle_configuration(Bucket=bucket_name)
        self.assertEqual(response['Rules'], rules)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='get lifecycle config no id')
    # @attr('lifecycle')
    # TODO:sequoias3不支持
    def test_lifecycle_get_no_id(self):
        bucket_name = get_new_bucket()
        client = get_client()

        rules = [{'Expiration': {'Days': 31}, 'Prefix': 'test1/', 'Status': 'Enabled'},
                 {'Expiration': {'Days': 120}, 'Prefix': 'test2/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        response = client.get_bucket_lifecycle_configuration(Bucket=bucket_name)
        current_lc = response['Rules']

        Rule = namedtuple('Rule', ['prefix', 'status', 'days'])
        rules = {'rule1': Rule('test1/', 'Enabled', 31),
                 'rule2': Rule('test2/', 'Enabled', 120)}

        for lc_rule in current_lc:
            if lc_rule['Prefix'] == rules['rule1'].prefix:
                self.assertEqual(lc_rule['Expiration']['Days'], rules['rule1'].days)
                self.assertEqual(lc_rule['Status'], rules['rule1'].status)
                assert 'ID' in lc_rule
            elif lc_rule['Prefix'] == rules['rule2'].prefix:
                self.assertEqual(lc_rule['Expiration']['Days'], rules['rule2'].days)
                self.assertEqual(lc_rule['Status'], rules['rule2'].status)
                assert 'ID' in lc_rule
            else:
                # neither of the rules we supplied was returned, something wrong
                print
                "rules not right"
                assert False

    # The test harness for lifecycle is configured to treat days as 10 second intervals.
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle expiration')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # @attr('fails_on_aws')
    # TODO:sequoias3不支持
    def test_lifecycle_expiration(self):
        bucket_name = self._create_objects(keys=['expire1/foo', 'expire1/bar', 'keep2/foo',
                                                 'keep2/bar', 'expire3/foo', 'expire3/bar'])
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Days': 1}, 'Prefix': 'expire1/', 'Status': 'Enabled'},
                 {'ID': 'rule2', 'Expiration': {'Days': 4}, 'Prefix': 'expire3/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        response = client.list_objects(Bucket=bucket_name)
        init_objects = response['Contents']

        time.sleep(28)
        response = client.list_objects(Bucket=bucket_name)
        expire1_objects = response['Contents']

        time.sleep(10)
        response = client.list_objects(Bucket=bucket_name)
        keep2_objects = response['Contents']

        time.sleep(20)
        response = client.list_objects(Bucket=bucket_name)
        expire3_objects = response['Contents']

        self.assertEqual(len(init_objects), 6)
        self.assertEqual(len(expire1_objects), 6)
        self.assertEqual(len(keep2_objects), 6)
        self.assertEqual(len(expire3_objects), 6)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle expiration with list-objects-v2')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # @attr('fails_on_aws')
    # @attr('list-objects-v2')
    # TODO:sequoias3不支持lifecycle
    def test_lifecyclev2_expiration(self):
        bucket_name = self._create_objects(keys=['expire1/foo', 'expire1/bar', 'keep2/foo',
                                                 'keep2/bar', 'expire3/foo', 'expire3/bar'])
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Days': 1}, 'Prefix': 'expire1/', 'Status': 'Enabled'},
                 {'ID': 'rule2', 'Expiration': {'Days': 4}, 'Prefix': 'expire3/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        response = client.list_objects_v2(Bucket=bucket_name)
        init_objects = response['Contents']

        time.sleep(28)
        response = client.list_objects_v2(Bucket=bucket_name)
        expire1_objects = response['Contents']

        time.sleep(10)
        response = client.list_objects_v2(Bucket=bucket_name)
        keep2_objects = response['Contents']

        time.sleep(20)
        response = client.list_objects_v2(Bucket=bucket_name)
        expire3_objects = response['Contents']

        self.assertEqual(len(init_objects), 6)
        self.assertEqual(len(expire1_objects), 4)
        self.assertEqual(len(keep2_objects), 4)
        self.assertEqual(len(expire3_objects), 2)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle expiration on versining enabled bucket')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # @attr('fails_on_aws')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_expiration_versioning_enabled(self):
        bucket_name = get_new_bucket()
        client = get_client()
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        self.create_multiple_versions(client, bucket_name, "test1/a", 1)
        client.delete_object(Bucket=bucket_name, Key="test1/a")

        rules = [{'ID': 'rule1', 'Expiration': {'Days': 1}, 'Prefix': 'test1/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        time.sleep(30)

        response = client.list_object_versions(Bucket=bucket_name)
        versions = response['Versions']
        delete_markers = response['DeleteMarkers']
        self.assertEqual(len(versions), 1)
        self.assertEqual(len(delete_markers), 1)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='id too long in lifecycle rule')
    # @attr('lifecycle')
    # @attr(assertion='fails 400')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_id_too_long(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 256 * 'a', 'Expiration': {'Days': 2}, 'Prefix': 'test1/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}

        e = assert_raises(ClientError, client.put_bucket_lifecycle_configuration, Bucket=bucket_name,
                          LifecycleConfiguration=lifecycle)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidArgument')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='same id')
    # @attr('lifecycle')
    # @attr(assertion='fails 400')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_same_id(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Days': 1}, 'Prefix': 'test1/', 'Status': 'Enabled'},
                 {'ID': 'rule1', 'Expiration': {'Days': 2}, 'Prefix': 'test2/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}

        e = assert_raises(ClientError, client.put_bucket_lifecycle_configuration, Bucket=bucket_name,
                          LifecycleConfiguration=lifecycle)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidArgument')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='invalid status in lifecycle rule')
    # @attr('lifecycle')
    # @attr(assertion='fails 400')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_invalid_status(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Days': 2}, 'Prefix': 'test1/', 'Status': 'enabled'}]
        lifecycle = {'Rules': rules}

        e = assert_raises(ClientError, client.put_bucket_lifecycle_configuration, Bucket=bucket_name,
                          LifecycleConfiguration=lifecycle)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

        rules = [{'ID': 'rule1', 'Expiration': {'Days': 2}, 'Prefix': 'test1/', 'Status': 'disabled'}]
        lifecycle = {'Rules': rules}

        e = assert_raises(ClientError, client.put_bucket_lifecycle, Bucket=bucket_name,
                          LifecycleConfiguration=lifecycle)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

        rules = [{'ID': 'rule1', 'Expiration': {'Days': 2}, 'Prefix': 'test1/', 'Status': 'invalid'}]
        lifecycle = {'Rules': rules}

        e = assert_raises(ClientError, client.put_bucket_lifecycle_configuration, Bucket=bucket_name,
                          LifecycleConfiguration=lifecycle)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config with expiration date')
    # @attr('lifecycle')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_set_date(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Date': '2017-09-27'}, 'Prefix': 'test1/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}

        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config with not iso8601 date')
    # @attr('lifecycle')
    # @attr(assertion='fails 400')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_set_invalid_date(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Date': '20200101'}, 'Prefix': 'test1/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}

        e = assert_raises(ClientError, client.put_bucket_lifecycle_configuration, Bucket=bucket_name,
                          LifecycleConfiguration=lifecycle)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle expiration with date')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # @attr('fails_on_aws')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_expiration_date(self):
        bucket_name = self._create_objects(keys=['past/foo', 'future/bar'])
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'Date': '2015-01-01'}, 'Prefix': 'past/', 'Status': 'Enabled'},
                 {'ID': 'rule2', 'Expiration': {'Date': '2030-01-01'}, 'Prefix': 'future/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        response = client.list_objects(Bucket=bucket_name)
        init_objects = response['Contents']

        time.sleep(20)
        response = client.list_objects(Bucket=bucket_name)
        expire_objects = response['Contents']

        self.assertEqual(len(init_objects), 2)
        self.assertEqual(len(expire_objects), 1)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle expiration days 0')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_expiration_days0(self):
        bucket_name = self._create_objects(keys=['days0/foo', 'days0/bar'])
        client = get_client()

        rules = [{'ID': 'rule1', 'Expiration': {'Days': 0}, 'Prefix': 'days0/',
                  'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(
            Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        time.sleep(20)

        response = client.list_objects(Bucket=bucket_name)
        expire_objects = response['Contents']

        self.assertEqual(len(expire_objects), 0)

    def setup_lifecycle_expiration(self, bucket_name, rule_id, delta_days,
                                   rule_prefix):
        client = get_client()
        rules = [{'ID': rule_id,
                  'Expiration': {'Days': delta_days}, 'Prefix': rule_prefix,
                  'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(
            Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        key = rule_prefix + '/foo'
        body = 'bar'
        response = client.put_object(Bucket=bucket_name, Key=key, Body="bar")
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)
        return response

    def check_lifecycle_expiration_header(self, response, start_time, rule_id,
                                          delta_days):
        exp_header = response['ResponseMetadata']['HTTPHeaders']['x-amz-expiration']
        m = re.search(r'expiry-date="(.+)", rule-id="(.+)"', exp_header)

        expiration = datetime.datetime.strptime(m.group(1),
                                                '%a %b %d %H:%M:%S %Y')
        self.assertEqual((expiration - start_time).days, delta_days)
        self.assertEqual(m.group(2), rule_id)

        return True

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle expiration header put')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_expiration_header_put(self):
        """
        Check for valid x-amz-expiration header after PUT
        """
        bucket_name = get_new_bucket()
        client = get_client()

        now = datetime.datetime.now(None)
        response = self.setup_lifecycle_expiration(
            bucket_name, 'rule1', 1, 'days1/')
        self.assertEqual(self.check_lifecycle_expiration_header(response, now, 'rule1', 1), True)

    # @attr(resource='bucket')
    # @attr(method='head')
    # @attr(operation='test lifecycle expiration header head')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_expiration_header_head(self):
        """
        Check for valid x-amz-expiration header on HEAD request
        """
        bucket_name = get_new_bucket()
        client = get_client()

        now = datetime.datetime.now(None)
        response = self.setup_lifecycle_expiration(
            bucket_name, 'rule1', 1, 'days1/')

        # stat the object, check header
        response = client.head_object(Bucket=bucket_name, Key='key')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)
        self.assertEqual(self.check_lifecycle_expiration_header(response, now, 'rule1', 1), True)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config with noncurrent version expiration')
    # @attr('lifecycle')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_set_noncurrent(self):
        bucket_name = self._create_objects(keys=['past/foo', 'future/bar'])
        client = get_client()
        rules = [
            {'ID': 'rule1', 'NoncurrentVersionExpiration': {'NoncurrentDays': 2}, 'Prefix': 'past/',
             'Status': 'Enabled'},
            {'ID': 'rule2', 'NoncurrentVersionExpiration': {'NoncurrentDays': 3}, 'Prefix': 'future/',
             'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle non-current version expiration')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # @attr('fails_on_aws')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_noncur_expiration(self):
        bucket_name = get_new_bucket()
        client = get_client()
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        self.create_multiple_versions(client, bucket_name, "test1/a", 3)
        # not checking the object contents on the second run, because the function doesn't support multiple checks
        self.create_multiple_versions(client, bucket_name, "test2/abc", 3, check_versions=False)

        response = client.list_object_versions(Bucket=bucket_name)
        init_versions = response['Versions']

        rules = [
            {'ID': 'rule1', 'NoncurrentVersionExpiration': {'NoncurrentDays': 2}, 'Prefix': 'test1/',
             'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        time.sleep(50)

        response = client.list_object_versions(Bucket=bucket_name)
        expire_versions = response['Versions']
        self.assertEqual(len(init_versions), 6)
        self.assertEqual(len(expire_versions), 4)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config with delete marker expiration')
    # @attr('lifecycle')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_set_deletemarker(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [
            {'ID': 'rule1', 'Expiration': {'ExpiredObjectDeleteMarker': True}, 'Prefix': 'test1/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config with Filter')
    # @attr('lifecycle')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_set_filter(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'ExpiredObjectDeleteMarker': True}, 'Filter': {'Prefix': 'foo'},
                  'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config with empty Filter')
    # @attr('lifecycle')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_set_empty_filter(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [{'ID': 'rule1', 'Expiration': {'ExpiredObjectDeleteMarker': True}, 'Filter': {}, 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle delete marker expiration')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # @attr('fails_on_aws')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_deletemarker_expiration(self):
        bucket_name = get_new_bucket()
        client = get_client()
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        self.create_multiple_versions(client, bucket_name, "test1/a", 1)
        self.create_multiple_versions(client, bucket_name, "test2/abc", 1, check_versions=False)
        client.delete_object(Bucket=bucket_name, Key="test1/a")
        client.delete_object(Bucket=bucket_name, Key="test2/abc")

        response = client.list_object_versions(Bucket=bucket_name)
        init_versions = response['Versions']
        deleted_versions = response['DeleteMarkers']
        total_init_versions = init_versions + deleted_versions

        rules = [{'ID': 'rule1', 'NoncurrentVersionExpiration': {'NoncurrentDays': 1},
                  'Expiration': {'ExpiredObjectDeleteMarker': True}, 'Prefix': 'test1/', 'Status': 'Enabled'}]
        lifecycle = {'Rules': rules}
        client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        time.sleep(50)

        response = client.list_object_versions(Bucket=bucket_name)
        init_versions = response['Versions']
        deleted_versions = response['DeleteMarkers']
        total_expire_versions = init_versions + deleted_versions

        self.assertEqual(len(total_init_versions), 4)
        self.assertEqual(len(total_expire_versions), 2)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='set lifecycle config with multipart expiration')
    # @attr('lifecycle')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_set_multipart(self):
        bucket_name = get_new_bucket()
        client = get_client()
        rules = [
            {'ID': 'rule1', 'Prefix': 'test1/', 'Status': 'Enabled',
             'AbortIncompleteMultipartUpload': {'DaysAfterInitiation': 2}},
            {'ID': 'rule2', 'Prefix': 'test2/', 'Status': 'Disabled',
             'AbortIncompleteMultipartUpload': {'DaysAfterInitiation': 3}}
        ]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='test lifecycle multipart expiration')
    # @attr('lifecycle')
    # @attr('lifecycle_expiration')
    # @attr('fails_on_aws')
    # TODO:sequoias3不支持lifecycle
    def test_lifecycle_multipart_expiration(self):
        bucket_name = get_new_bucket()
        client = get_client()

        key_names = ['test1/a', 'test2/']
        upload_ids = []

        for key in key_names:
            response = client.create_multipart_upload(Bucket=bucket_name, Key=key)
            upload_ids.append(response['UploadId'])

        response = client.list_multipart_uploads(Bucket=bucket_name)
        init_uploads = response['Uploads']

        rules = [
            {'ID': 'rule1', 'Prefix': 'test1/', 'Status': 'Enabled',
             'AbortIncompleteMultipartUpload': {'DaysAfterInitiation': 2}},
        ]
        lifecycle = {'Rules': rules}
        response = client.put_bucket_lifecycle_configuration(Bucket=bucket_name, LifecycleConfiguration=lifecycle)
        time.sleep(50)

        response = client.list_multipart_uploads(Bucket=bucket_name)
        expired_uploads = response['Uploads']
        self.assertEqual(len(init_uploads), 2)
        self.assertEqual(len(expired_uploads), 1)
