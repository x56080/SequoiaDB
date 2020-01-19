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


class TestLock(unittest.TestCase):

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object lock with defalut retention')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_put_obj_lock(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Days': 1
                    }
                }}
        response = client.put_object_lock_configuration(
            Bucket=bucket_name,
            ObjectLockConfiguration=conf)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'COMPLIANCE',
                        'Years': 1
                    }
                }}
        response = client.put_object_lock_configuration(
            Bucket=bucket_name,
            ObjectLockConfiguration=conf)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_bucket_versioning(Bucket=bucket_name)
        self.assertEqual(response['Status'], 'Enabled')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object lock with bucket object lock not enabled')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_lock_invalid_bucket(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Days': 1
                    }
                }}
        e = assert_raises(ClientError, client.put_object_lock_configuration, Bucket=bucket_name,
                          ObjectLockConfiguration=conf)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 409)
        self.assertEqual(error_code, 'InvalidBucketState')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object lock with days and years')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_lock_with_days_and_years(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Days': 1,
                        'Years': 1
                    }
                }}
        e = assert_raises(ClientError, client.put_object_lock_configuration, Bucket=bucket_name,
                          ObjectLockConfiguration=conf)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object lock with invalid days')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_lock_invalid_days(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Days': 0
                    }
                }}
        e = assert_raises(ClientError, client.put_object_lock_configuration, Bucket=bucket_name,
                          ObjectLockConfiguration=conf)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidRetentionPeriod')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object lock with invalid years')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_lock_invalid_years(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Years': -1
                    }
                }}
        e = assert_raises(ClientError, client.put_object_lock_configuration, Bucket=bucket_name,
                          ObjectLockConfiguration=conf)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidRetentionPeriod')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object lock with invalid mode')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_lock_invalid_years(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'abc',
                        'Years': 1
                    }
                }}
        e = assert_raises(ClientError, client.put_object_lock_configuration, Bucket=bucket_name,
                          ObjectLockConfiguration=conf)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'governance',
                        'Years': 1
                    }
                }}
        e = assert_raises(ClientError, client.put_object_lock_configuration, Bucket=bucket_name,
                          ObjectLockConfiguration=conf)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object lock with invalid status')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_lock_invalid_status(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Disabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Years': 1
                    }
                }}
        e = assert_raises(ClientError, client.put_object_lock_configuration, Bucket=bucket_name,
                          ObjectLockConfiguration=conf)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test suspend versioning when object lock enabled')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_suspend_versioning(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        e = assert_raises(ClientError, client.put_bucket_versioning, Bucket=bucket_name,
                          VersioningConfiguration={'Status': 'Suspended'})
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 409)
        self.assertEqual(error_code, 'InvalidBucketState')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test get object lock')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_get_obj_lock(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Days': 1
                    }
                }}
        client.put_object_lock_configuration(
            Bucket=bucket_name,
            ObjectLockConfiguration=conf)
        response = client.get_object_lock_configuration(Bucket=bucket_name)
        self.assertEqual(response['ObjectLockConfiguration'], conf)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test get object lock with bucket object lock not enabled')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_get_obj_lock_invalid_bucket(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name)
        e = assert_raises(ClientError, client.get_object_lock_configuration, Bucket=bucket_name)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'ObjectLockConfigurationNotFoundError')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test put object retention')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        version_id = response['VersionId']
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        response = client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id, BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object retention with bucket object lock not enabled')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention_invalid_bucket(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        e = assert_raises(ClientError, client.put_object_retention, Bucket=bucket_name, Key=key, Retention=retention)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidRequest')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object retention with invalid mode')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention_invalid_mode(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        retention = {'Mode': 'governance', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        e = assert_raises(ClientError, client.put_object_retention, Bucket=bucket_name, Key=key, Retention=retention)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

        retention = {'Mode': 'abc', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        e = assert_raises(ClientError, client.put_object_retention, Bucket=bucket_name, Key=key, Retention=retention)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test get object retention')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_get_obj_retention(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        version_id = response['VersionId']
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention)
        response = client.get_object_retention(Bucket=bucket_name, Key=key)
        self.assertEqual(response['Retention'], retention)
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id, BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test get object retention with invalid bucket')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_get_obj_retention_invalid_bucket(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        e = assert_raises(ClientError, client.get_object_retention, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidRequest')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object retention with version id')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention_versionid(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        version_id = response['VersionId']
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, VersionId=version_id, Retention=retention)
        response = client.get_object_retention(Bucket=bucket_name, Key=key, VersionId=version_id)
        self.assertEqual(response['Retention'], retention)
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id, BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object retention to override default retention')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention_override_default_retention(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        conf = {'ObjectLockEnabled': 'Enabled',
                'Rule': {
                    'DefaultRetention': {
                        'Mode': 'GOVERNANCE',
                        'Days': 1
                    }
                }}
        client.put_object_lock_configuration(
            Bucket=bucket_name,
            ObjectLockConfiguration=conf)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        version_id = response['VersionId']
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention)
        response = client.get_object_retention(Bucket=bucket_name, Key=key)
        self.assertEqual(response['Retention'], retention)
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id, BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object retention to increase retention period')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention_increase_period(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        version_id = response['VersionId']
        retention1 = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention1)
        retention2 = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 3, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention2)
        response = client.get_object_retention(Bucket=bucket_name, Key=key)
        self.assertEqual(response['Retention'], retention2)
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id, BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object retention to shorten period')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention_shorten_period(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        version_id = response['VersionId']
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 3, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention)
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        e = assert_raises(ClientError, client.put_object_retention, Bucket=bucket_name, Key=key, Retention=retention)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AccessDenied')
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id, BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put object retention to shorten period with bypass header')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_put_obj_retention_shorten_period_bypass(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        version_id = response['VersionId']
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 3, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention)
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention, BypassGovernanceRetention=True)
        response = client.get_object_retention(Bucket=bucket_name, Key=key)
        self.assertEqual(response['Retention'], retention)
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id, BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='delete')
    # @attr(operation='Test delete object with retention')
    # @attr(assertion='retention period make effects')
    # @attr('object-lock')
    def test_object_lock_delete_object_with_retention(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'

        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention)
        e = assert_raises(ClientError, client.delete_object, Bucket=bucket_name, Key=key,
                          VersionId=response['VersionId'])
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AccessDenied')

        response = client.delete_object(Bucket=bucket_name, Key=key, VersionId=response['VersionId'],
                                        BypassGovernanceRetention=True)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 204)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put legal hold')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_put_legal_hold(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        legal_hold = {'Status': 'ON'}
        response = client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold=legal_hold)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)
        response = client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold={'Status': 'OFF'})
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put legal hold with invalid bucket')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_legal_hold_invalid_bucket(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        legal_hold = {'Status': 'ON'}
        e = assert_raises(ClientError, client.put_object_legal_hold, Bucket=bucket_name, Key=key, LegalHold=legal_hold)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidRequest')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put legal hold with invalid status')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_put_legal_hold_invalid_status(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        legal_hold = {'Status': 'abc'}
        e = assert_raises(ClientError, client.put_object_legal_hold, Bucket=bucket_name, Key=key, LegalHold=legal_hold)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test get legal hold')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_get_legal_hold(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        legal_hold = {'Status': 'ON'}
        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold=legal_hold)
        response = client.get_object_legal_hold(Bucket=bucket_name, Key=key)
        self.assertEqual(response['LegalHold'], legal_hold)
        legal_hold_off = {'Status': 'OFF'}
        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold=legal_hold_off)
        response = client.get_object_legal_hold(Bucket=bucket_name, Key=key)
        self.assertEqual(response['LegalHold'], legal_hold_off)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test get legal hold with invalid bucket')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_get_legal_hold_invalid_bucket(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        e = assert_raises(ClientError, client.get_object_legal_hold, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidRequest')

    # @attr(resource='bucket')
    # @attr(method='delete')
    # @attr(operation='Test delete object with legal hold on')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_delete_object_with_legal_hold_on(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold={'Status': 'ON'})
        e = assert_raises(ClientError, client.delete_object, Bucket=bucket_name, Key=key,
                          VersionId=response['VersionId'])
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AccessDenied')
        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold={'Status': 'OFF'})

    # @attr(resource='bucket')
    # @attr(method='delete')
    # @attr(operation='Test delete object with legal hold off')
    # @attr(assertion='fails')
    # @attr('object-lock')
    def test_object_lock_delete_object_with_legal_hold_off(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        response = client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold={'Status': 'OFF'})
        response = client.delete_object(Bucket=bucket_name, Key=key, VersionId=response['VersionId'])
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 204)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='Test get object metadata')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_get_obj_metadata(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key)
        legal_hold = {'Status': 'ON'}
        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold=legal_hold)
        retention = {'Mode': 'GOVERNANCE', 'RetainUntilDate': datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC)}
        client.put_object_retention(Bucket=bucket_name, Key=key, Retention=retention)
        response = client.head_object(Bucket=bucket_name, Key=key)
        print(response)
        self.assertEqual(response['ObjectLockMode'], retention['Mode'])
        self.assertEqual(response['ObjectLockRetainUntilDate'], retention['RetainUntilDate'])
        self.assertEqual(response['ObjectLockLegalHoldStatus'], legal_hold['Status'])

        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold={'Status': 'OFF'})
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=response['VersionId'],
                             BypassGovernanceRetention=True)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='Test put legal hold and retention when uploading object')
    # @attr(assertion='success')
    # @attr('object-lock')
    def test_object_lock_uploading_obj(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, ObjectLockEnabledForBucket=True)
        key = 'file1'
        client.put_object(Bucket=bucket_name, Body='abc', Key=key, ObjectLockMode='GOVERNANCE',
                          ObjectLockRetainUntilDate=datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC),
                          ObjectLockLegalHoldStatus='ON')

        response = client.head_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ObjectLockMode'], 'GOVERNANCE')
        self.assertEqual(response['ObjectLockRetainUntilDate'], datetime.datetime(2030, 1, 1, tzinfo=pytz.UTC))
        self.assertEqual(response['ObjectLockLegalHoldStatus'], 'ON')
        client.put_object_legal_hold(Bucket=bucket_name, Key=key, LegalHold={'Status': 'OFF'})
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=response['VersionId'],
                             BypassGovernanceRetention=True)