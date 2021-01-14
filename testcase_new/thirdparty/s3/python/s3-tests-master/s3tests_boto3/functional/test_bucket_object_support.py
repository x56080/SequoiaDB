import random
import string
import threading
import time
import datetime
import requests
from email.header import decode_header
from io import StringIO
from datetime import timedelta

import pytz
from botocore.exceptions import ClientError, ParamValidationError

from lib.testlib import *
from . import (
    setup,
    teardown,
    get_client,
    get_prefix,
    get_unauthenticated_client,
    get_bad_auth_client,
    get_new_bucket,
    get_new_bucket_name,
    get_new_bucket_resource,
    get_alt_client,
    get_buckets_list,
    get_objects_list,
    get_main_aws_access_key, get_main_api_name)
from .file_utils import *
from .utils import _get_status_and_error_code
from .utils import assert_raises
from .utils import generate_random


class TestBucketObject(S3TestBase):
    @staticmethod
    def _bucket_is_empty(bucket):
        is_empty = True
        for obj in bucket.objects.all():
            is_empty = False
            break
        return is_empty

    @staticmethod
    def _create_key_with_random_content(keyname, size=7 * 1024 * 1024, bucket_name=None, client=None):
        if bucket_name is None:
            bucket_name = get_new_bucket()

        if client is None:
            client = get_client()
        # TODO:StringIO is not found
        data = StringIO(str(generate_random(size, size).next()))
        client.put_object(Bucket=bucket_name, Key=keyname, Body=data)

        return bucket_name

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='empty buckets return no contents')
    def test_bucket_list_empty(self):
        bucket = get_new_bucket_resource()
        is_empty = self._bucket_is_empty(bucket)
        self.assertEqual(is_empty, True)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='distinct buckets have different contents')
    def test_bucket_list_distinct(self):
        bucket1 = get_new_bucket_resource()
        bucket2 = get_new_bucket_resource()
        obj = bucket1.put_object(Body='str', Key='asdf')
        is_empty = self._bucket_is_empty(bucket2)
        self.assertEqual(is_empty, True)

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

    @staticmethod
    def _get_keys(response):
        """
        return lists of strings that are the keys from a client.list_objects() response
        """
        keys = []
        if 'Contents' in response:
            objects_list = response['Contents']
            keys = [obj['Key'] for obj in objects_list]
        return keys

    @staticmethod
    def _get_prefixes(response):
        """
        return lists of strings that are prefixes from a client.list_objects() response
        """
        prefixes = []
        if 'CommonPrefixes' in response:
            prefix_list = response['CommonPrefixes']
            prefixes = [prefix['Prefix'] for prefix in prefix_list]
        return prefixes

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='pagination w/max_keys=2, no marker')
    def test_bucket_list_many(self):
        bucket_name = self._create_objects(keys=['foo', 'bar', 'baz'])
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, MaxKeys=2)
        keys = self._get_keys(response)
        self.assertEqual(len(keys), 2)
        self.assertEqual(keys, ['bar', 'baz'])
        self.assertEqual(response['IsTruncated'], True)

        response = client.list_objects(Bucket=bucket_name, Marker='baz', MaxKeys=2)
        keys = self._get_keys(response)
        self.assertEqual(len(keys), 1)
        self.assertEqual(response['IsTruncated'], False)
        self.assertEqual(keys, ['foo'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='pagination w/max_keys=2, no marker')
    # @attr('list-objects-v2')
    def test_bucket_listv2_many(self):
        bucket_name = self._create_objects(keys=['foo', 'bar', 'baz'])
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, MaxKeys=2)
        keys = self._get_keys(response)
        self.assertEqual(len(keys), 2)
        self.assertEqual(keys, ['bar', 'baz'])
        self.assertEqual(response['IsTruncated'], True)

        response = client.list_objects_v2(Bucket=bucket_name, StartAfter='baz', MaxKeys=2)
        keys = self._get_keys(response)
        self.assertEqual(len(keys), 1)
        self.assertEqual(response['IsTruncated'], False)
        self.assertEqual(keys, ['foo'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='keycount in listobjectsv2')
    # @attr('list-objects-v2')
    def test_basic_key_count(self):
        client = get_client()
        bucket_names = []
        bucket_name = get_new_bucket_name()
        client.create_bucket(Bucket=bucket_name)
        for j in range(5):
            client.put_object(Bucket=bucket_name, Key=str(j))
        response1 = client.list_objects_v2(Bucket=bucket_name)
        self.assertEqual(response1['KeyCount'], 5)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefixes in multi-component object names')
    def test_bucket_list_delimiter_basic(self):
        bucket_name = self._create_objects(keys=['foo/bar', 'foo/bar/xyzzy', 'quux/thud', 'asdf'])
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='/')
        self.assertEqual(response['Delimiter'], '/')
        keys = self._get_keys(response)
        self.assertEqual(keys, ['asdf'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        self.assertEqual(prefixes, ['foo/', 'quux/'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefixes in multi-component object names')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_basic(self):
        bucket_name = self._create_objects(keys=['foo/bar', 'foo/bar/xyzzy', 'quux/thud', 'asdf'])
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='/')
        self.assertEqual(response['Delimiter'], '/')
        keys = self._get_keys(response)
        self.assertEqual(keys, ['asdf'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        self.assertEqual(prefixes, ['foo/', 'quux/'])

    def validate_bucket_list(self, bucket_name, prefix, delimiter, marker, max_keys,
                             is_truncated, check_objs, check_prefixes, next_marker):
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter=delimiter, Marker=marker, MaxKeys=max_keys,
                                       Prefix=prefix)
        self.assertEqual(response['IsTruncated'], is_truncated)
        if 'NextMarker' not in response:
            response['NextMarker'] = None
        self.assertEqual(response['NextMarker'], next_marker)

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)

        self.assertEqual(len(keys), len(check_objs))
        self.assertEqual(len(prefixes), len(check_prefixes))
        self.assertEqual(keys, check_objs)
        self.assertEqual(prefixes, check_prefixes)

        return response['NextMarker']

    def validate_bucket_listv2(self, bucket_name, prefix, delimiter, continuation_token, max_keys,
                               is_truncated, check_objs, check_prefixes, last=False):
        client = get_client()
        params = dict(Bucket=bucket_name, Delimiter=delimiter, MaxKeys=max_keys, Prefix=prefix)
        if continuation_token is not None:
            params['ContinuationToken'] = continuation_token
        else:
            params['StartAfter'] = ''
        response = client.list_objects_v2(**params)
        self.assertEqual(response['IsTruncated'], is_truncated)
        if 'NextContinuationToken' not in response:
            response['NextContinuationToken'] = None
        if last:
            self.assertEqual(response['NextContinuationToken'], None)
        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(len(keys), len(check_objs))
        self.assertEqual(len(prefixes), len(check_prefixes))
        self.assertEqual(keys, check_objs)
        self.assertEqual(prefixes, check_prefixes)

        return response['NextContinuationToken']

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefixes in multi-component object names')
    # TODO:bug:SEQUOIADBMAINSTREAM-5251
    def test_bucket_list_delimiter_prefix(self):
        bucket_name = self._create_objects(keys=['asdf', 'boo/bar', 'boo/baz/xyzzy', 'cquux/thud', 'cquux/bla'])

        delim = '/'
        marker = ''
        prefix = ''

        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 1, True, ['asdf'], [], 'asdf')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 1, True, [], ['boo/'], 'boo/')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 1, False, [], ['cquux/'], None)

        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 2, True, ['asdf'], ['boo/'], 'boo/')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 2, False, [], ['cquux/'], None)

        prefix = 'boo/'

        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 1, True, ['boo/bar'], [], 'boo/bar')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 1, False, [], ['boo/baz/'], None)

        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 2, False, ['boo/bar'], ['boo/baz/'], None)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefixes in multi-component object names')
    # @attr('list-objects-v2')
    # TODO:bug:SEQUOIADBMAINSTREAM-5251
    def test_bucket_listv2_delimiter_prefix(self):
        bucket_name = self._create_objects(keys=['asdf', 'boo/bar', 'boo/baz/xyzzy', 'cquux/thud', 'cquux/bla'])
        delim = '/'
        continuation_token = ''
        prefix = ''
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 1, True, ['asdf'], [])

        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 1, True,
                                                         [], ['boo/'])

        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 1, False, [],
                                                         ['cquux/'], last=True)

        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 2, True, ['asdf'], ['boo/'])
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 2, False, [],
                                                         ['cquux/'], last=True)

        prefix = 'boo/'
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 1, True, ['boo/bar'], [])
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 1, False, [],
                                                         ['boo/baz/'], last=True)

        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 2, False, ['boo/bar'],
                                                         ['boo/baz/'],
                                                         last=True)


    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefix and delimiter handling when object ends with delimiter')
    def test_bucket_list_delimiter_prefix_ends_with_delimiter(self):
        bucket_name = self._create_objects(keys=['asdf/'])
        self.validate_bucket_list(bucket_name, 'asdf/', '/', '', 1000, False, ['asdf/'], [], None)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefix and delimiter handling when object ends with delimiter')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_prefix_ends_with_delimiter(self):
        bucket_name = self._create_objects(keys=['asdf/'])
        self.validate_bucket_listv2(bucket_name=bucket_name, prefix='asdf/', delimiter='/',
                                    continuation_token=None, max_keys=1000, is_truncated=False,
                                    check_objs=['asdf/'], check_prefixes=[], last=True)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='non-slash delimiter characters')
    def test_bucket_list_delimiter_alt(self):
        bucket_name = self._create_objects(keys=['bar', 'baz', 'cab', 'foo'])
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='a')
        self.assertEqual(response['Delimiter'], 'a')

        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        # bar, baz, and cab should be broken up by the 'a' delimiters
        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        self.assertEqual(prefixes, ['ba', 'ca'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='non-slash delimiter characters')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_alt(self):
        bucket_name = self._create_objects(keys=['bar', 'baz', 'cab', 'foo'])
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='a')
        self.assertEqual(response['Delimiter'], 'a')

        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        # bar, baz, and cab should be broken up by the 'a' delimiters
        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        self.assertEqual(prefixes, ['ba', 'ca'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefixes starting with underscore')
    def test_bucket_list_delimiter_prefix_underscore(self):
        bucket_name = self._create_objects(
            keys=['_obj1_', '_under1/bar', '_under1/baz/xyzzy', '_under2/thud', '_under2/bla'])

        delim = '/'
        marker = ''
        prefix = ''
        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 1, True, ['_obj1_'], [], '_obj1_')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 1, True, [], ['_under1/'], '_under1/')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 1, False, [], ['_under2/'], None)

        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 2, True, ['_obj1_'], ['_under1/'],
                                           '_under1/')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 2, False, [], ['_under2/'], None)

        prefix = '_under1/'

        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 1, True, ['_under1/bar'], [], '_under1/bar')
        marker = self.validate_bucket_list(bucket_name, prefix, delim, marker, 1, False, [], ['_under1/baz/'], None)

        marker = self.validate_bucket_list(bucket_name, prefix, delim, '', 2, False, ['_under1/bar'], ['_under1/baz/'],
                                           None)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='prefixes starting with underscore')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_prefix_underscore(self):
        bucket_name = self._create_objects(
            keys=['_obj1_', '_under1/bar', '_under1/baz/xyzzy', '_under2/thud', '_under2/bla'])

        delim = '/'
        continuation_token = ''
        prefix = ''
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 1, True, ['_obj1_'], [])
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 1, True, [],
                                                         ['_under1/'])
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 1, False, [],
                                                         ['_under2/'], last=True)

        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 2, True, ['_obj1_'],
                                                         ['_under1/'])
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 2, False, [],
                                                         ['_under2/'], last=True)

        prefix = '_under1/'

        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 1, True, ['_under1/bar'], [])
        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, continuation_token, 1, False, [],
                                                         ['_under1/baz/'], last=True)

        continuation_token = self.validate_bucket_listv2(bucket_name, prefix, delim, None, 2, False, ['_under1/bar'],
                                                         ['_under1/baz/'], last=True)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='percentage delimiter characters')
    def test_bucket_list_delimiter_percentage(self):
        bucket_name = self._create_objects(keys=['b%ar', 'b%az', 'c%ab', 'foo'])
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='%')
        self.assertEqual(response['Delimiter'], '%')
        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        # bar, baz, and cab should be broken up by the 'a' delimiters
        self.assertEqual(prefixes, ['b%', 'c%'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='percentage delimiter characters')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_percentage(self):
        bucket_name = self._create_objects(keys=['b%ar', 'b%az', 'c%ab', 'foo'])
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='%')
        self.assertEqual(response['Delimiter'], '%')
        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        # bar, baz, and cab should be broken up by the 'a' delimiters
        self.assertEqual(prefixes, ['b%', 'c%'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='whitespace delimiter characters')
    def test_bucket_list_delimiter_whitespace(self):
        bucket_name = self._create_objects(keys=['b ar', 'b az', 'c ab', 'foo'])
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter=' ')
        self.assertEqual(response['Delimiter'], ' ')
        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        # bar, baz, and cab should be broken up by the 'a' delimiters
        self.assertEqual(prefixes, ['b ', 'c '])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='whitespace delimiter characters')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_whitespace(self):
        bucket_name = self._create_objects(keys=['b ar', 'b az', 'c ab', 'foo'])
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter=' ')
        self.assertEqual(response['Delimiter'], ' ')
        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        # bar, baz, and cab should be broken up by the 'a' delimiters
        self.assertEqual(prefixes, ['b ', 'c '])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='dot delimiter characters')
    def test_bucket_list_delimiter_dot(self):
        bucket_name = self._create_objects(keys=['b.ar', 'b.az', 'c.ab', 'foo'])
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='.')
        self.assertEqual(response['Delimiter'], '.')
        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        # bar, baz, and cab should be broken up by the 'a' delimiters
        self.assertEqual(prefixes, ['b.', 'c.'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='dot delimiter characters')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_dot(self):
        bucket_name = self._create_objects(keys=['b.ar', 'b.az', 'c.ab', 'foo'])
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='.')
        self.assertEqual(response['Delimiter'], '.')
        keys = self._get_keys(response)
        # foo contains no 'a' and so is a complete key
        self.assertEqual(keys, ['foo'])

        prefixes = self._get_prefixes(response)
        self.assertEqual(len(prefixes), 2)
        # bar, baz, and cab should be broken up by the 'a' delimiters
        self.assertEqual(prefixes, ['b.', 'c.'])

        # @attr(resource='bucket')
        # @attr(method='get')
        # @attr(operation='list')
        # @attr(assertion='non-printable delimiter can be specified')

    def test_bucket_list_delimiter_unreadable(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter=' x0a')
        self.assertEqual(response['Delimiter'], ' x0a')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='non-printable delimiter can be specified')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_unreadable(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter=' x0a')
        self.assertEqual(response['Delimiter'], ' x0a')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='empty delimiter can be specified')
    def test_bucket_list_delimiter_empty(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='')
        # putting an empty value into Delimiter will not return a value in the response
        self.assertEqual('Delimiter' in response, True)

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='empty delimiter can be specified')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_empty(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='')
        # putting an empty value into Delimiter will not return a value in the response
        self.assertEqual('Delimiter' in response, True)

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='unspecified delimiter defaults to none')
    def test_bucket_list_delimiter_none(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name)
        # putting an empty value into Delimiter will not return a value in the response
        self.assertEqual('Delimiter' in response, False)

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='unspecified delimiter defaults to none')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_none(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name)
        # putting an empty value into Delimiter will not return a value in the response
        self.assertEqual('Delimiter' in response, False)

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr('list-objects-v2')
    def test_bucket_listv2_fetchowner_notempty(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, FetchOwner=True)
        objs_list = response['Contents']
        self.assertEqual('Owner' in objs_list[0], True)

    # @attr('list-objects-v2')
    def test_bucket_listv2_fetchowner_defaultempty(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name)
        objs_list = response['Contents']
        self.assertEqual('Owner' in objs_list[0], False)

    # @attr('list-objects-v2')
    def test_bucket_listv2_fetchowner_empty(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, FetchOwner=False)
        objs_list = response['Contents']
        self.assertEqual('Owner' in objs_list[0], False)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='unused delimiter is not found')
    def test_bucket_list_delimiter_not_exist(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='/')
        # putting an empty value into Delimiter will not return a value in the response
        self.assertEqual(response['Delimiter'], '/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(assertion='unused delimiter is not found')
    # @attr('list-objects-v2')
    def test_bucket_listv2_delimiter_not_exist(self):
        key_names = ['bar', 'baz', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='/')
        # putting an empty value into Delimiter will not return a value in the response
        self.assertEqual(response['Delimiter'], '/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list')
    # @attr(assertion='list with delimiter not skip special keys')
    def test_bucket_list_delimiter_not_skip_special(self):
        key_names = ['0/'] + ['0/%s' % i for i in range(1000, 1999)]
        key_names2 = ['1999', '1999#', '1999+', '2000']
        key_names += key_names2
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='/')
        self.assertEqual(response['Delimiter'], '/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names2)
        self.assertEqual(prefixes, ['0/'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix')
    # @attr(assertion='returns only objects under prefix')
    def test_bucket_list_prefix_basic(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Prefix='foo/', EncodingType='url')

        self.assertEqual(response['Prefix'], 'foo%2F')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['foo%2Fbar', 'foo%2Fbaz'])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix with list-objects-v2')
    # @attr(assertion='returns only objects under prefix')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_basic(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Prefix='foo/')
        self.assertEqual(response['Prefix'], 'foo/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['foo/bar', 'foo/baz'])
        self.assertEqual(prefixes, [])

    # just testing that we can do the delimeter and prefix logic on non-slashes
    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix')
    # @attr(assertion='prefixes w/o delimiters')
    def test_bucket_list_prefix_alt(self):
        key_names = ['bar', 'baz', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Prefix='ba')
        self.assertEqual(response['Prefix'], 'ba')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['bar', 'baz'])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix with list-objects-v2')
    # @attr(assertion='prefixes w/o delimiters')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_alt(self):
        key_names = ['bar', 'baz', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Prefix='ba')
        self.assertEqual(response['Prefix'], 'ba')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['bar', 'baz'])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix')
    # @attr(assertion='empty prefix returns everything')
    def test_bucket_list_prefix_empty(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Prefix='')
        self.assertEqual(response['Prefix'], '')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix with list-objects-v2')
    # @attr(assertion='empty prefix returns everything')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_empty(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Prefix='')
        self.assertEqual(response['Prefix'], '')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix')
    # @attr(assertion='unspecified prefix returns everything')
    def test_bucket_list_prefix_none(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Prefix='')
        self.assertEqual(response['Prefix'], '')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix with list-objects-v2')
    # @attr(assertion='unspecified prefix returns everything')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_none(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name)
        self.assertEqual('Prefix' in response, False)

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix')
    # @attr(assertion='nonexistent prefix returns nothing')
    def test_bucket_list_prefix_not_exist(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Prefix='d')
        self.assertEqual(response['Prefix'], 'd')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix with list-objects-v2')
    # @attr(assertion='nonexistent prefix returns nothing')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_not_exist(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Prefix='d')
        self.assertEqual(response['Prefix'], 'd')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix')
    # @attr(assertion='non-printable prefix can be specified')
    def test_bucket_list_prefix_unreadable(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Prefix=' x0a')
        self.assertEqual(response['Prefix'], '+x0a')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix with list-objects-v2')
    # @attr(assertion='non-printable prefix can be specified')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_unreadable(self):
        key_names = ['foo/bar', 'foo/baz', 'quux']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Prefix=' x0a')
        self.assertEqual(response['Prefix'], ' x0a')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix w/delimiter')
    # @attr(assertion='returns only objects directly under prefix')
    def test_bucket_list_prefix_delimiter_basic(self):
        key_names = ['foo/bar', 'foo/baz/xyzzy', 'quux/thud', 'asdf']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='/', Prefix='foo/', EncodingType='url')
        self.assertEqual(response['Prefix'], 'foo%2F')
        self.assertEqual(response['Delimiter'], '%2F')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['foo%2Fbar'])
        self.assertEqual(prefixes, ['foo%2Fbaz%2F'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list-objects-v2 under prefix w/delimiter')
    # @attr(assertion='returns only objects directly under prefix')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_delimiter_basic(self):
        key_names = ['foo/bar', 'foo/baz/xyzzy', 'quux/thud', 'asdf']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='/', Prefix='foo/')
        self.assertEqual(response['Prefix'], 'foo/')
        self.assertEqual(response['Delimiter'], '/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['foo/bar'])
        self.assertEqual(prefixes, ['foo/baz/'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix w/delimiter')
    # @attr(assertion='non-slash delimiters')
    def test_bucket_list_prefix_delimiter_alt(self):
        key_names = ['bar', 'bazar', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='a', Prefix='ba')
        self.assertEqual(response['Prefix'], 'ba')
        self.assertEqual(response['Delimiter'], 'a')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['bar'])
        self.assertEqual(prefixes, ['baza'])

    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_delimiter_alt(self):
        key_names = ['bar', 'bazar', 'cab', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='a', Prefix='ba')
        self.assertEqual(response['Prefix'], 'ba')
        self.assertEqual(response['Delimiter'], 'a')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['bar'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix w/delimiter')
    # @attr(assertion='finds nothing w/unmatched prefix')
    def test_bucket_list_prefix_delimiter_prefix_not_exist(self):
        key_names = ['b/a/r', 'b/a/c', 'b/a/g', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()
        response = client.list_objects(Bucket=bucket_name, Delimiter='d', Prefix='/')
        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list-objects-v2 under prefix w/delimiter')
    # @attr(assertion='finds nothing w/unmatched prefix')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_delimiter_prefix_not_exist(self):
        key_names = ['b/a/r', 'b/a/c', 'b/a/g', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='d', Prefix='/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix w/delimiter')
    # @attr(assertion='finds nothing w/unmatched prefix')
    def test_bucket_list_prefix_delimiter_prefix_not_exist(self):
        key_names = ['b/a/r', 'b/a/c', 'b/a/g', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='d', Prefix='/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list-objects-v2 under prefix w/delimiter')
    # @attr(assertion='finds nothing w/unmatched prefix')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_delimiter_prefix_not_exist(self):
        key_names = ['b/a/r', 'b/a/c', 'b/a/g', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='d', Prefix='/')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix w/delimiter')
    # @attr(assertion='over-ridden slash ceases to be a delimiter')
    def test_bucket_list_prefix_delimiter_delimiter_not_exist(self):
        key_names = ['b/a/c', 'b/a/g', 'b/a/r', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='z', Prefix='b')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['b/a/c', 'b/a/g', 'b/a/r'])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list-objects-v2 under prefix w/delimiter')
    # @attr(assertion='over-ridden slash ceases to be a delimiter')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_delimiter_delimiter_not_exist(self):
        key_names = ['b/a/c', 'b/a/g', 'b/a/r', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='z', Prefix='b')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, ['b/a/c', 'b/a/g', 'b/a/r'])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list under prefix w/delimiter')
    # @attr(assertion='finds nothing w/unmatched prefix and delimiter')
    def test_bucket_list_prefix_delimiter_prefix_delimiter_not_exist(self):
        key_names = ['b/a/c', 'b/a/g', 'b/a/r', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Delimiter='z', Prefix='y')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list-objects-v2 under prefix w/delimiter')
    # @attr(assertion='finds nothing w/unmatched prefix and delimiter')
    # @attr('list-objects-v2')
    def test_bucket_listv2_prefix_delimiter_prefix_delimiter_not_exist(self):
        key_names = ['b/a/c', 'b/a/g', 'b/a/r', 'g']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, Delimiter='z', Prefix='y')

        keys = self._get_keys(response)
        prefixes = self._get_prefixes(response)
        self.assertEqual(keys, [])
        self.assertEqual(prefixes, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='pagination w/max_keys=1, marker')
    def test_bucket_list_maxkeys_one(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, MaxKeys=1)
        self.assertEqual(response['IsTruncated'], True)

        keys = self._get_keys(response)
        self.assertEqual(keys, key_names[0:1])

        response = client.list_objects(Bucket=bucket_name, Marker=key_names[0])
        self.assertEqual(response['IsTruncated'], False)

        keys = self._get_keys(response)
        self.assertEqual(keys, key_names[1:])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='pagination w/max_keys=1, marker')
    # @attr('list-objects-v2')
    def test_bucket_listv2_maxkeys_one(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, MaxKeys=1)
        self.assertEqual(response['IsTruncated'], True)

        keys = self._get_keys(response)
        self.assertEqual(keys, key_names[0:1])

        response = client.list_objects_v2(Bucket=bucket_name, StartAfter=key_names[0])
        self.assertEqual(response['IsTruncated'], False)

        keys = self._get_keys(response)
        self.assertEqual(keys, key_names[1:])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='pagination w/max_keys=0')
    # TODO:SEQUOIADBMAINSTREAM-5263
    def test_bucket_list_maxkeys_zero(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, MaxKeys=0)

        self.assertEqual(response['IsTruncated'], False)
        keys = self._get_keys(response)
        self.assertEqual(keys, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='pagination w/max_keys=0')
    # @attr('list-objects-v2')
    # TODO:SEQUOIADBMAINSTREAM-5263
    def test_bucket_listv2_maxkeys_zero(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, MaxKeys=0)

        self.assertEqual(response['IsTruncated'], False)
        keys = self._get_keys(response)
        self.assertEqual(keys, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='pagination w/o max_keys')
    def test_bucket_list_maxkeys_none(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name)
        self.assertEqual(response['IsTruncated'], False)
        keys = self._get_keys(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(response['MaxKeys'], 1000)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='pagination w/o max_keys')
    # @attr('list-objects-v2')
    def test_bucket_listv2_maxkeys_none(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name)
        self.assertEqual(response['IsTruncated'], False)
        keys = self._get_keys(response)
        self.assertEqual(keys, key_names)
        self.assertEqual(response['MaxKeys'], 1000)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='bucket list unordered')
    # @attr('fails_on_aws')
    # allow-unordered is a non-standard extension
    def test_bucket_list_unordered(self):
        # boto3.set_stream_logger(name='botocore')
        keys_in = ['ado', 'bot', 'cob', 'dog', 'emu', 'fez', 'gnu', 'hex',
                   'abc/ink', 'abc/jet', 'abc/kin', 'abc/lax', 'abc/mux',
                   'def/nim', 'def/owl', 'def/pie', 'def/qed', 'def/rye',
                   'ghi/sew', 'ghi/tor', 'ghi/uke', 'ghi/via', 'ghi/wit',
                   'xix', 'yak', 'zoo']
        bucket_name = self._create_objects(keys=keys_in)
        client = get_client()

        # adds the unordered query parameter
        def add_unordered(**kwargs):
            kwargs['params']['url'] += "&allow-unordered=true"

        client.meta.events.register('before-call.s3.ListObjects', add_unordered)

        # test simple retrieval
        response = client.list_objects(Bucket=bucket_name, MaxKeys=1000)
        unordered_keys_out = self._get_keys(response)
        self.assertEqual(len(keys_in), len(unordered_keys_out))
        self.assertEqual(keys_in.sort(), unordered_keys_out.sort())

        # test retrieval with prefix
        response = client.list_objects(Bucket=bucket_name,
                                       MaxKeys=1000,
                                       Prefix="abc/")
        unordered_keys_out = self._get_keys(response)
        self.assertEqual(5, len(unordered_keys_out))

        # test incremental retrieval with marker
        response = client.list_objects(Bucket=bucket_name, MaxKeys=6)
        unordered_keys_out = self._get_keys(response)
        self.assertEqual(6, len(unordered_keys_out))

        # now get the next bunch
        response = client.list_objects(Bucket=bucket_name,
                                       MaxKeys=6,
                                       Marker=unordered_keys_out[-1])
        unordered_keys_out2 = self._get_keys(response)
        self.assertEqual(6, len(unordered_keys_out2))

        # make sure there's no overlap between the incremental retrievals
        intersect = set(unordered_keys_out).intersection(unordered_keys_out2)
        self.assertEqual(0, len(intersect))

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='bucket list unordered')
    # @attr('fails_on_aws') # allow-unordered is a non-standard extension
    # @attr('list-objects-v2')
    def test_bucket_listv2_unordered(self):
        # boto3.set_stream_logger(name='botocore')
        keys_in = ['ado', 'bot', 'cob', 'dog', 'emu', 'fez', 'gnu', 'hex',
                   'abc/ink', 'abc/jet', 'abc/kin', 'abc/lax', 'abc/mux',
                   'def/nim', 'def/owl', 'def/pie', 'def/qed', 'def/rye',
                   'ghi/sew', 'ghi/tor', 'ghi/uke', 'ghi/via', 'ghi/wit',
                   'xix', 'yak', 'zoo']
        bucket_name = self._create_objects(keys=keys_in)
        client = get_client()

        # adds the unordered query parameter
        def add_unordered(**kwargs):
            kwargs['params']['url'] += "&allow-unordered=true"

        client.meta.events.register('before-call.s3.ListObjects', add_unordered)

        # test simple retrieval
        response = client.list_objects_v2(Bucket=bucket_name, MaxKeys=1000)
        unordered_keys_out = self._get_keys(response)
        self.assertEqual(len(keys_in), len(unordered_keys_out))
        self.assertEqual(keys_in.sort(), unordered_keys_out.sort())

        # test retrieval with prefix
        response = client.list_objects_v2(Bucket=bucket_name,
                                          MaxKeys=1000,
                                          Prefix="abc/")
        unordered_keys_out = self._get_keys(response)
        self.assertEqual(5, len(unordered_keys_out))

        # test incremental retrieval with marker
        response = client.list_objects_v2(Bucket=bucket_name, MaxKeys=6)
        unordered_keys_out = self._get_keys(response)
        self.assertEqual(6, len(unordered_keys_out))

        # now get the next bunch
        response = client.list_objects_v2(Bucket=bucket_name,
                                          MaxKeys=6,
                                          StartAfter=unordered_keys_out[-1])
        unordered_keys_out2 = self._get_keys(response)
        self.assertEqual(6, len(unordered_keys_out2))

        # make sure there's no overlap between the incremental retrievals
        intersect = set(unordered_keys_out).intersection(unordered_keys_out2)
        self.assertEqual(0, len(intersect))

        # verify that unordered used with delimiter results in error
        # e = assert_raises(ClientError,
        #                   client.list_objects, Bucket=bucket_name, Delimiter="/")
        # status, error_code = _get_status_and_error_code(e.response)
        # self.assertEqual(status, 400)
        # self.assertEqual(error_code, 'InvalidArgument')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='invalid max_keys')
    def test_bucket_list_maxkeys_invalid(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()
        assert_raises(ParamValidationError, client.list_objects, Bucket=bucket_name, MaxKeys="blah")

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='no pagination, no marker')
    def test_bucket_list_marker_none(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name)
        self.assertEqual('Marker' in response, False)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='no pagination, empty marker')
    def test_bucket_list_marker_empty(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Marker='')
        self.assertEqual(response['Marker'], '')
        self.assertEqual(response['IsTruncated'], False)
        keys = self._get_keys(response)
        self.assertEqual(keys, key_names)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='no pagination, empty continuationtoken')
    # @attr('list-objects-v2')
    def test_bucket_listv2_continuationtoken_empty(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        e = assert_raises(ClientError, client.list_objects_v2, Bucket=bucket_name, ContinuationToken='')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidArgument')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list keys with list-objects-v2')
    # @attr(assertion='no pagination, non-empty continuationtoken')
    # @attr('list-objects-v2')
    def test_bucket_listv2_continuationtoken(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response1 = client.list_objects_v2(Bucket=bucket_name, MaxKeys=1)
        next_continuation_token = response1['NextContinuationToken']

        response2 = client.list_objects_v2(Bucket=bucket_name, ContinuationToken=next_continuation_token)
        self.assertEqual(response2['ContinuationToken'], next_continuation_token)
        self.assertEqual(response2['IsTruncated'], False)
        key_names2 = ['baz', 'foo', 'quxx']
        keys = self._get_keys(response2)
        self.assertEqual(keys, key_names2)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list keys with list-objects-v2')
    # @attr(assertion='no pagination, non-empty continuationtoken and startafter')
    # @attr('list-objects-v2')
    def test_bucket_listv2_both_continuationtoken_startafter(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response1 = client.list_objects_v2(Bucket=bucket_name, StartAfter='bar', MaxKeys=1)
        next_continuation_token = response1['NextContinuationToken']

        response2 = client.list_objects_v2(Bucket=bucket_name, StartAfter='bar',
                                           ContinuationToken=next_continuation_token)
        self.assertEqual(response2['ContinuationToken'], next_continuation_token)
        self.assertEqual(response2['StartAfter'], 'bar')
        self.assertEqual(response2['IsTruncated'], False)
        key_names2 = ['foo', 'quxx']
        keys = self._get_keys(response2)
        self.assertEqual(keys, key_names2)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='non-printing marker')
    def test_bucket_list_marker_unreadable(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Marker=' x0a')
        self.assertEqual(response['Marker'], ' x0a')
        self.assertEqual(response['IsTruncated'], False)
        keys = self._get_keys(response)
        self.assertEqual(keys, key_names)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='non-printing startafter')
    # @attr('list-objects-v2')
    def test_bucket_listv2_startafter_unreadable(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, StartAfter=' x0a')
        self.assertEqual(response['StartAfter'], ' x0a')
        self.assertEqual(response['IsTruncated'], False)
        keys = self._get_keys(response)
        self.assertEqual(keys, key_names)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='marker not-in-list')
    def test_bucket_list_marker_not_in_list(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Marker='blah')
        self.assertEqual(response['Marker'], 'blah')
        keys = self._get_keys(response)
        self.assertEqual(keys, ['foo', 'quxx'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='startafter not-in-list')
    # @attr('list-objects-v2')
    def test_bucket_listv2_startafter_not_in_list(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, StartAfter='blah')
        self.assertEqual(response['StartAfter'], 'blah')
        keys = self._get_keys(response)
        self.assertEqual(keys, ['foo', 'quxx'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys')
    # @attr(assertion='marker after list')
    def test_bucket_list_marker_after_list(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects(Bucket=bucket_name, Marker='zzz')
        self.assertEqual(response['Marker'], 'zzz')
        keys = self._get_keys(response)
        self.assertEqual(response['IsTruncated'], False)
        self.assertEqual(keys, [])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all keys with list-objects-v2')
    # @attr(assertion='startafter after list')
    # @attr('list-objects-v2')
    def test_bucket_listv2_startafter_after_list(self):
        key_names = ['bar', 'baz', 'foo', 'quxx']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        response = client.list_objects_v2(Bucket=bucket_name, StartAfter='zzz')
        self.assertEqual(response['StartAfter'], 'zzz')
        keys = self._get_keys(response)
        self.assertEqual(response['IsTruncated'], False)
        self.assertEqual(keys, [])

    def _compare_dates(self, datetime1, datetime2):
        """
        changes ms from datetime1 to 0, compares it to datetime2
        """
        # both times are in datetime format but datetime1 has
        # microseconds and datetime2 does not
        datetime1 = datetime1.replace(microsecond=0)
        self.assertEqual(datetime1, datetime2)

    # @attr(resource='object')
    # @attr(method='head')
    # @attr(operation='compare w/bucket list')
    # @attr(assertion='return same metadata')
    def test_bucket_list_return_data(self):
        key_names = ['bar', 'baz', 'foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        data = {}
        for key_name in key_names:
            obj_response = client.head_object(Bucket=bucket_name, Key=key_name)
            acl_response = client.get_object_acl(Bucket=bucket_name, Key=key_name)
            data.update({
                key_name: {
                    'DisplayName': acl_response['Owner']['DisplayName'],
                    'ID': acl_response['Owner']['ID'],
                    'ETag': obj_response['ETag'],
                    'LastModified': obj_response['LastModified'],
                    'ContentLength': obj_response['ContentLength'],
                }
            })

        response = client.list_objects(Bucket=bucket_name)
        objs_list = response['Contents']
        for obj in objs_list:
            key_name = obj['Key']
            key_data = data[key_name]
            act_etag = obj['ETag']
            exp_etag = key_data['ETag']
            self.assertEqual(act_etag in  exp_etag, True)
            self.assertEqual(obj['Size'], key_data['ContentLength'])
            self.assertEqual(obj['Owner']['DisplayName'], key_data['DisplayName'])
            self.assertEqual(obj['Owner']['ID'], key_data['ID'])
            self._compare_dates(datetime1=obj['LastModified'], datetime2=key_data['LastModified'])

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

            if (expected_string == read_status):
                break

            time.sleep(1)

        self.assertEqual(expected_string, read_status)

    # @attr(resource='object')
    # @attr(method='head')
    # @attr(operation='compare w/bucket list when bucket versioning is configured')
    # @attr(assertion='return same metadata')
    # @attr('versioning')
    # TODO: sequoias3目前不支持MFADelete
    def test_bucket_list_return_data_versioning(self):
        self.skipTest('sequoias3 不支持')
        bucket_name = get_new_bucket()
        self.check_configure_versioning_retry(bucket_name=bucket_name, status="Enabled", expected_string="Enabled")
        key_names = ['bar', 'baz', 'foo']
        bucket_name = self._create_objects(bucket_name=bucket_name, keys=key_names)

        client = get_client()
        data = {}

        for key_name in key_names:
            obj_response = client.head_object(Bucket=bucket_name, Key=key_name)
            acl_response = client.get_object_acl(Bucket=bucket_name, Key=key_name)
            data.update({
                key_name: {
                    'ID': acl_response['Owner']['ID'],
                    'DisplayName': acl_response['Owner']['DisplayName'],
                    'ETag': obj_response['ETag'],
                    'LastModified': obj_response['LastModified'],
                    'ContentLength': obj_response['ContentLength'],
                    'VersionId': obj_response['VersionId']
                }
            })

        response = client.list_object_versions(Bucket=bucket_name)
        objs_list = response['Versions']

        for obj in objs_list:
            key_name = obj['Key']
            key_data = data[key_name]
            self.assertEqual(obj['Owner']['DisplayName'], key_data['DisplayName'])
            self.assertEqual(obj['ETag'], key_data['ETag'])
            self.assertEqual(obj['Size'], key_data['ContentLength'])
            self.assertEqual(obj['Owner']['ID'], key_data['ID'])
            self.assertEqual(obj['VersionId'], key_data['VersionId'])
            self._compare_dates(obj['LastModified'], key_data['LastModified'])

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all objects (anonymous)')
    # @attr(assertion='succeeds')
    # TODO: sequoias3目前不支持桶的ACL
    def test_bucket_list_objects_anonymous(self):
        self.skipTest('sequoias3 不支持')
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_bucket_acl(Bucket=bucket_name, ACL='public-read')

        unauthenticated_client = get_unauthenticated_client()
        unauthenticated_client.list_objects(Bucket=bucket_name)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all objects (anonymous) with list-objects-v2')
    # @attr(assertion='succeeds')
    # @attr('list-objects-v2')
    # TODO: sequoias3目前不支持桶的ACL
    def test_bucket_listv2_objects_anonymous(self):
        self.skipTest('sequoias3 不支持')
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_bucket_acl(Bucket=bucket_name, ACL='public-read')

        unauthenticated_client = get_unauthenticated_client()
        unauthenticated_client.list_objects_v2(Bucket=bucket_name)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all objects (anonymous)')
    # @attr(assertion='fails')
    def test_bucket_list_objects_anonymous_fail(self):
        bucket_name = get_new_bucket()

        unauthenticated_client = get_unauthenticated_client()

        e = assert_raises(ClientError, unauthenticated_client.list_objects, Bucket=bucket_name)

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AuthorizationHeaderMalformed')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all objects (anonymous) with list-objects-v2')
    # @attr(assertion='fails')
    # @attr('list-objects-v2')
    def test_bucket_listv2_objects_anonymous_fail(self):
        bucket_name = get_new_bucket()

        unauthenticated_client = get_unauthenticated_client()
        e = assert_raises(ClientError, unauthenticated_client.list_objects_v2, Bucket=bucket_name)

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'AuthorizationHeaderMalformed')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='non-existant bucket')
    # @attr(assertion='fails 404')
    def test_bucket_notexist(self):
        bucket_name = get_new_bucket_name()
        client = get_client()

        e = assert_raises(ClientError, client.list_objects, Bucket=bucket_name)

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='non-existant bucket with list-objects-v2')
    # @attr(assertion='fails 404')
    # @attr('list-objects-v2')
    def test_bucketv2_notexist(self):
        bucket_name = get_new_bucket_name()
        client = get_client()

        e = assert_raises(ClientError, client.list_objects_v2, Bucket=bucket_name)

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='bucket')
    # @attr(method='delete')
    # @attr(operation='non-existant bucket')
    # @attr(assertion='fails 404')
    def test_bucket_delete_notexist(self):
        bucket_name = get_new_bucket_name()
        client = get_client()

        e = assert_raises(ClientError, client.delete_bucket, Bucket=bucket_name)

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='bucket')
    # @attr(method='delete')
    # @attr(operation='non-empty bucket')
    # @attr(assertion='fails 409')
    def test_bucket_delete_nonempty(self):
        key_names = ['foo']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()

        e = assert_raises(ClientError, client.delete_bucket, Bucket=bucket_name)

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 409)
        self.assertEqual(error_code, 'BucketNotEmpty')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='non-existant bucket')
    # @attr(assertion='fails 404')
    def test_object_write_to_nonexist_bucket(self):
        key_names = ['foo']
        bucket_name = 'whatchutalkinboutwillis'
        client = get_client()

        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key='foo', Body='foo')

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchBucket')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='read contents that were never written')
    # @attr(assertion='fails 404')
    def test_object_read_notexist(self):
        bucket_name = get_new_bucket()
        client = get_client()
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='bar')
        status, error_code = _get_status_and_error_code(e.response)

        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchKey')

    def _make_objs_dict(self, key_names):
        objs_list = []
        for key in key_names:
            obj_dict = {'Key': key}
            objs_list.append(obj_dict)
            objs_dict = {'Objects': objs_list}
        return objs_dict

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='delete multiple objects')
    # @attr(assertion='deletes multiple objects with a single call')
    # TODO:目前sequoias3不支持删除多个对象
    def test_multi_object_delete(self):
        self.skipTest('sequoias3 不支持')
        key_names = ['key0', 'key1', 'key2']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()
        response = client.list_objects(Bucket=bucket_name)
        self.assertEqual(len(response['Contents']), 3)

        objs_dict = self._make_objs_dict(key_names=key_names)
        response = client.delete_objects(Bucket=bucket_name, Delete=objs_dict)

        self.assertEqual(len(response['Deleted']), 3)
        assert 'Errors' not in response
        response = client.list_objects(Bucket=bucket_name)
        assert 'Contents' not in response

        response = client.delete_objects(Bucket=bucket_name, Delete=objs_dict)
        self.assertEqual(len(response['Deleted']), 3)
        assert 'Errors' not in response
        response = client.list_objects(Bucket=bucket_name)
        assert 'Contents' not in response

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='delete multiple objects with list-objects-v2')
    # @attr(assertion='deletes multiple objects with a single call')
    # @attr('list-objects-v2')
    # TODO:目前sequoias3不支持删除多个对象
    def test_multi_objectv2_delete(self):
        self.skipTest('sequoias3 不支持')
        key_names = ['key0', 'key1', 'key2']
        bucket_name = self._create_objects(keys=key_names)
        client = get_client()
        response = client.list_objects_v2(Bucket=bucket_name)
        self.assertEqual(len(response['Contents']), 3)

        objs_dict = self._make_objs_dict(key_names=key_names)
        response = client.delete_objects(Bucket=bucket_name, Delete=objs_dict)

        self.assertEqual(len(response['Deleted']), 3)
        assert 'Errors' not in response
        response = client.list_objects_v2(Bucket=bucket_name)
        assert 'Contents' not in response

        response = client.delete_objects(Bucket=bucket_name, Delete=objs_dict)
        self.assertEqual(len(response['Deleted']), 3)
        assert 'Errors' not in response
        response = client.list_objects_v2(Bucket=bucket_name)
        assert 'Contents' not in response

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write zero-byte key')
    # @attr(assertion='correct content length')
    def test_object_head_zero_bytes(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='')
        response = client.head_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['ContentLength'], 0)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write key')
    # @attr(assertion='correct etag')
    def test_object_write_check_etag(self):
        bucket_name = get_new_bucket()
        client = get_client()
        response = client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)
        self.assertEqual(response['ETag'], '"37b51d194a7513e45b56f6524f2d51f2"')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write key')
    # @attr(assertion='correct cache control header')
    def test_object_write_cache_control(self):
        bucket_name = get_new_bucket()
        client = get_client()
        cache_control = 'public, max-age=14400'
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar', CacheControl=cache_control)

        response = client.head_object(Bucket=bucket_name, Key='foo')
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['cache-control'], cache_control)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='write key')
    # @attr(assertion='correct expires header')
    def test_object_write_expires(self):
        bucket_name = get_new_bucket()
        client = get_client()

        utc = pytz.utc
        expires = datetime.now(utc) + timedelta(seconds=+6000)
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar', Expires=expires)

        response = client.head_object(Bucket=bucket_name, Key='foo')
        self._compare_dates(expires, response['Expires'])

    def _get_body(self, response):
        body = response['Body']
        got = body.read()
        return got

    # @attr(resource='object')   
    # @attr(method='all')   
    # @attr(operation='complete object life cycle')
    # @attr(assertion='read back what we wrote and rewrote')
    def test_object_write_read_update_read_delete(self):
        bucket_name = get_new_bucket()
        client = get_client()

        # Write
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        # Read
        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')
        # Update
        client.put_object(Bucket=bucket_name, Key='foo', Body='soup')
        # Read
        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'soup')
        # Delete
        client.delete_object(Bucket=bucket_name, Key='foo')

    def _set_get_metadata(self, metadata, bucket_name=None):
        """
        create a new bucket new or use an existing
        name to create an object that bucket,
        set the meta1 property to a specified, value,
        and then re-read and return that property
        """
        if bucket_name is None:
            bucket_name = get_new_bucket()

        client = get_client()
        metadata_dict = {'meta1': metadata}
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar', Metadata=metadata_dict)

        response = client.get_object(Bucket=bucket_name, Key='foo')
        return response['Metadata']['meta1']

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata write/re-read')
    # @attr(assertion='reread what we wrote')
    def test_object_set_get_metadata_none_to_good(self):
        got = self._set_get_metadata('mymeta')
        self.assertEqual(got, 'mymeta')

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata write/re-read')
    # @attr(assertion='write empty value, returns empty value')
    def test_object_set_get_metadata_none_to_empty(self):
        got = self._set_get_metadata('')
        self.assertEqual(got, '')

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata write/re-write')
    # @attr(assertion='empty value replaces old')
    def test_object_set_get_metadata_overwrite_to_empty(self):
        bucket_name = get_new_bucket()
        got = self._set_get_metadata('oldmeta', bucket_name)
        self.assertEqual(got, 'oldmeta')
        got = self._set_get_metadata('', bucket_name)
        self.assertEqual(got, '')

    # # @attr(resource='object.metadata')
    # # @attr(method='put')
    # # @attr(operation='metadata write/re-write')
    # # @attr(assertion='UTF-8 values passed through')
    # TODO: 需要弄清楚 client.meta.events.register的用法
    def test_object_set_get_unicode_metadata(self):
        self.skipTest("sequoias3 不支持")
        bucket_name = get_new_bucket()
        client = get_client()

        def set_unicode_metadata(**kwargs):
            kwargs['params']['headers']['x-amz-meta-meta1'] = u"Hello World xe9"

        client.meta.events.register('before-call.s3.PutObject', set_unicode_metadata)
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        got = response['Metadata']['meta1'].decode('utf-8')
        self.assertEqual(got, u"Hello World xe9")

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata write/re-write')
    # @attr(assertion='non-UTF-8 values detected, but preserved')
    # @attr('fails_strict_rfc2616')
    def test_object_set_get_non_utf8_metadata(self):
        self.skipTest('sequoias3 不支持')
        bucket_name = get_new_bucket()
        client = get_client()
        metadata_dict = {'meta1': ' x04mymeta'}
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar', Metadata=metadata_dict)

        response = client.get_object(Bucket=bucket_name, Key='foo')
        got = response['Metadata']['meta1']
        self.assertEqual(got, '=?UTF-8?Q?=04mymeta?=')

    def _set_get_metadata_unreadable(self, metadata, bucket_name=None):
        """
        set and then read back a meta-data value (which presumably
        includes some interesting characters), and return a list
        containing the stored value AND the encoding with which it
        was returned.
        """
        got = self._set_get_metadata(metadata, bucket_name)
        got = decode_header(got)
        return got

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata write')
    # @attr(assertion='non-priting prefixes noted and preserved')
    # @attr('fails_strict_rfc2616')
    def test_object_set_get_metadata_empty_to_unreadable_prefix(self):
        self.skipTest('sequoias3不支持')
        metadata = ' x04w'
        got = self._set_get_metadata_unreadable(metadata)
        self.assertEqual(got, [(b' x04w', 'utf-8')])

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata write')
    # @attr(assertion='non-priting suffixes noted and preserved')
    # @attr('fails_strict_rfc2616')
    def test_object_set_get_metadata_empty_to_unreadable_suffix(self):
        self.skipTest('sequoias3不支持')
        metadata = 'h x04'
        got = self._set_get_metadata_unreadable(metadata)
        self.assertEqual(got, [(metadata, 'utf-8')])

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata write')
    # @attr(assertion='non-priting in-fixes noted and preserved')
    # @attr('fails_strict_rfc2616')
    # 开发说这种不可见字符，修改没有意义，因此注释掉该用例
    def test_object_set_get_metadata_empty_to_unreadable_infix(self):
        self.skipTest('sequoias3不支持')
        metadata = 'h x04w'
        got = self._set_get_metadata_unreadable(metadata)
        self.assertEqual(got, [(metadata, 'utf-8')])

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata re-write')
    # @attr(assertion='non-priting prefixes noted and preserved')
    # @attr('fails_strict_rfc2616')
    def test_object_set_get_metadata_overwrite_to_unreadable_prefix(self):
        self.skipTest('sequoias3不支持')
        metadata = ' x04w'
        got = self._set_get_metadata_unreadable(metadata)
        self.assertEqual(got, [(metadata, 'utf-8')])
        metadata2 = ' x05w'
        got2 = self._set_get_metadata_unreadable(metadata2)
        self.assertEqual(got2, [(metadata2, 'utf-8')])

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata re-write')
    # @attr(assertion='non-priting suffixes noted and preserved')
    # @attr('fails_strict_rfc2616')
    def test_object_set_get_metadata_overwrite_to_unreadable_suffix(self):
        self.skipTest('sequoias3不支持')
        metadata = 'h x04'
        got = self._set_get_metadata_unreadable(metadata)
        self.assertEqual(got, [(metadata, 'utf-8')])
        metadata2 = 'h x05'
        got2 = self._set_get_metadata_unreadable(metadata2)
        self.assertEqual(got2, [(metadata2, 'utf-8')])

    # @attr(resource='object.metadata')
    # @attr(method='put')
    # @attr(operation='metadata re-write')
    # @attr(assertion='non-priting in-fixes noted and preserved')
    # @attr('fails_strict_rfc2616')
    def test_object_set_get_metadata_overwrite_to_unreadable_infix(self):
        self.skipTest('sequoias3不支持')
        metadata = 'h x04w'
        got = self._set_get_metadata_unreadable(metadata)
        self.assertEqual(got, [(metadata, 'utf-8')])
        metadata2 = 'h x05w'
        got2 = self._set_get_metadata_unreadable(metadata2)
        self.assertEqual(got2, [(metadata2, 'utf-8')])

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='data re-write')
    # @attr(assertion='replaces previous metadata')
    def test_object_metadata_replaced_on_put(self):
        bucket_name = get_new_bucket()
        client = get_client()
        metadata_dict = {'meta1': 'bar'}
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar', Metadata=metadata_dict)

        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        got = response['Metadata']
        self.assertEqual(got, {})

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='data write from file (w/100-Continue)')
    # @attr(assertion='succeeds and returns written data')
    def test_object_write_file(self):
        bucket_name = get_new_bucket()
        client = get_client()
        data = b'bar'
        client.put_object(Bucket=bucket_name, Key='foo', Body=data)
        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-Match: the latest ETag')
    # @attr(assertion='succeeds')
    # TODO: SEQUOIADBMAINSTREAM - 5271
    def test_get_object_ifmatch_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        response = client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        etag = response['ETag']
        response = client.get_object(Bucket=bucket_name, Key='foo', IfMatch=etag)
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-Match: bogus ETag')
    # @attr(assertion='fails 412')
    def test_get_object_ifmatch_failed(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='foo', IfMatch='"ABCORZ"')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-None-Match: the latest ETag')
    # @attr(assertion='fails 304')
    # TODO: SEQUOIADBMAINSTREAM - 5271
    def test_get_object_ifnonematch_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        response = client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        etag = response['ETag']

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='foo', IfNoneMatch=etag)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 304)
        self.assertEqual(e.response['Error']['Message'], 'Not Modified')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-None-Match: bogus ETag')
    # @attr(assertion='succeeds')
    def test_get_object_ifnonematch_failed(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo', IfNoneMatch='ABCORZ')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-Modified-Since: before')
    # @attr(assertion='succeeds')
    def test_get_object_ifmodifiedsince_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo', IfModifiedSince='Sat, 29 Oct 1994 19:43:31 GMT')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-Modified-Since: after')
    # @attr(assertion='fails 304')
    def test_get_object_ifmodifiedsince_failed(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object(Bucket=bucket_name, Key='foo')
        last_modified = str(response['LastModified'])

        last_modified = last_modified.split('+')[0]
        mtime = datetime.strptime(last_modified, '%Y-%m-%d %H:%M:%S')
        after = mtime + timedelta(seconds=1)
        after_str = time.strftime("%a, %d %b %Y %H:%M:%S GMT", after.timetuple())

        time.sleep(1)

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='foo', IfModifiedSince=after_str)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 304)
        self.assertEqual(e.response['Error']['Message'], 'Not Modified')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-Unmodified-Since: before')
    # @attr(assertion='fails 412')
    def test_get_object_ifunmodifiedsince_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='foo',
                          IfUnmodifiedSince='Sat, 29 Oct 1994 19:43:31 GMT')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-Unmodified-Since: after')
    # @attr(assertion='succeeds')
    def test_get_object_ifunmodifiedsince_failed(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo', IfUnmodifiedSince='Sat, 29 Oct 2100 19:43:31 GMT')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='data re-write w/ If-Match: the latest ETag')
    # @attr(assertion='replaces previous data and metadata')
    # @attr('fails_on_aws')
    def test_put_object_ifmatch_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

        etag = response['ETag'].replace('"', '')

        # pass in custom header 'If-Match' before PutObject call
        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-Match': etag}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        response = client.put_object(Bucket=bucket_name, Key='foo', Body='zar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'zar')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='get w/ If-Match: bogus ETag')
    # @attr(assertion='fails 412')
    # TODO:ClientError:An error occurred (NotImplemented) when calling the PutObject operation
    def test_put_object_ifmatch_failed(self):
        self.skipTest("sequoias3 不支持")
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

        # pass in custom header 'If-Match' before PutObject call
        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-Match': '"ABCORZ"'}))
        client.meta.events.register('before-call.s3.PutObject', lf)

        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key='foo', Body='zar')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='overwrite existing object w/ If-Match: *')
    # @attr(assertion='replaces previous data and metadata')
    # @attr('fails_on_aws')
    def test_put_object_ifmatch_overwrite_existed_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-Match': '*'}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        response = client.put_object(Bucket=bucket_name, Key='foo', Body='zar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'zar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='overwrite non-existing object w/ If-Match: *')
    # @attr(assertion='fails 412')
    # @attr('fails_on_aws')
    # TODO:ClientError:An error occurred (NotImplemented) when calling the PutObject operation
    def test_put_object_ifmatch_nonexisted_failed(self):
        self.skipTest("sequoias3 不支持")
        bucket_name = get_new_bucket()
        client = get_client()

        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-Match': '*'}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key='foo', Body='bar')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchKey')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='overwrite existing object w/ If-None-Match: outdated ETag')
    # @attr(assertion='replaces previous data and metadata')
    # @attr('fails_on_aws')
    def test_put_object_ifnonmatch_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-None-Match': 'ABCORZ'}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        response = client.put_object(Bucket=bucket_name, Key='foo', Body='zar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'zar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='overwrite existing object w/ If-None-Match: the latest ETag')
    # @attr(assertion='fails 412')
    # @attr('fails_on_aws')
    # TODO:ClientError:An error occurred (NotImplemented) when calling the PutObject operation
    def test_put_object_ifnonmatch_failed(self):
        self.skipTest("sequoias3 不支持")
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

        etag = response['ETag'].replace('"', '')

        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-None-Match': etag}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key='foo', Body='zar')

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='overwrite non-existing object w/ If-None-Match: *')
    # @attr(assertion='succeeds')
    # @attr('fails_on_aws')
    # TODO:ClientError:An error occurred (NotImplemented) when calling the PutObject operation
    def test_put_object_ifnonmatch_nonexisted_good(self):
        bucket_name = get_new_bucket()
        client = get_client()

        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-None-Match': '*'}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='overwrite existing object w/ If-None-Match: *')
    # @attr(assertion='fails 412')
    # @attr('fails_on_aws')
    def test_put_object_ifnonmatch_overwrite_existed_failed(self):
        self.skipTest("sequoias3 不支持")
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, b'bar')

        lf = (lambda **kwargs: kwargs['params']['headers'].update({'If-None-Match': '*'}))
        client.meta.events.register('before-call.s3.PutObject', lf)
        e = assert_raises(ClientError, client.put_object, Bucket=bucket_name, Key='foo', Body='zar')

        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

        response = client.get_object(Bucket=bucket_name, Key='foo')
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    def check_bad_bucket_name(self, bucket_name):
        """
            Attempt to create a bucket with a specified name, and confirm
            that the request fails because of an invalid bucket name.
        """
        client = get_client()
        e = assert_raises(ClientError, client.create_bucket, Bucket=bucket_name)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidBucketName')

    # AWS does not enforce all documented bucket restrictions.
    # http://docs.amazonwebservices.com/AmazonS3/2006-03-01/dev/index.html?BucketRestrictions.html
    # @attr('fails_on_aws')
    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='name begins with underscore')
    # @attr(assertion='fails with subdomain: 400')
    # TODO:java驱动有做校验，python驱动没有做校验，amazons3创建失败，sequoias3成功
    def test_bucket_create_naming_bad_starts_nonalpha(self):
        self.skipTest('sequoias3不支持')
        bucket_name = get_new_bucket_name()
        self.check_bad_bucket_name('_' + bucket_name)

    def check_invalid_bucketname(self, invalid_name):
        """
        Send a create bucket_request with an invalid bucket name
        that will bypass the ParamValidationError that would be raised
        if the invalid bucket name that was passed in normally.
        This function returns the status and error code from the failure
        """
        client = get_client()
        assert_raises(ParamValidationError, client.create_bucket, Bucket=invalid_name)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='empty name')
    # @attr(assertion='fails 405')s
    # TODO: remove this fails_on_rgw when I fix it
    # @attr('fails_on_rgw')
    # TODO: amazons3返回的是ClientError,而sequoias3返回是Exception
    def test_bucket_create_naming_bad_short_empty(self):
        invalid_bucketname = ''
        self.check_invalid_bucketname(invalid_bucketname)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='short (one character) name')
    # @attr(assertion='fails 400')
    def test_bucket_create_naming_bad_short_one(self):
        self.check_bad_bucket_name('a')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='short (two character) name')
    # @attr(assertion='fails 400')
    def test_bucket_create_naming_bad_short_two(self):
        self.check_bad_bucket_name('aa')

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='excessively long names')
    # @attr(assertion='fails with subdomain: 400')
    # TODO: remove this fails_on_rgw when I fix it
    # TODO: amazons3返回的是ClientError,而sequoias3返回是Exception
    # @attr('fails_on_rgw')
    def test_bucket_create_naming_bad_long(self):
        invalid_bucketname = 256 * 'a'
        self.check_invalid_bucketname(invalid_bucketname)

        invalid_bucketname = 280 * 'a'
        self.check_invalid_bucketname(invalid_bucketname)

        invalid_bucketname = 3000 * 'a'
        self.check_invalid_bucketname(invalid_bucketname)

    def check_good_bucket_name(self, name, _prefix=None):
        """
        Attempt to create a bucket with a specified name
        and (specified or default) prefix, returning the
        results of that effort.
        """
        # tests using this with the default prefix must *not* rely on
        # being able to set the initial character, or exceed the max len

        # tests using this with a custom prefix are responsible for doing
        # their own setup/teardown nukes, with their custom prefix; this
        # should be very rare
        if _prefix is None:
            _prefix = get_prefix()
        bucket_name = '{prefix}{name}'.format(
            prefix=_prefix,
            name=name,
        )
        client = get_client()
        response = client.create_bucket(Bucket=bucket_name)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    def _test_bucket_create_naming_good_long(self, length):
        """
        Attempt to create a bucket whose name (including the
        prefix) is of a specified length.
        """
        # tests using this with the default prefix must *not* rely on
        # being able to set the initial character, or exceed the max len

        # tests using this with a custom prefix are responsible for doing
        # their own setup/teardown nukes, with their custom prefix; this
        # should be very rare
        prefix = get_new_bucket_name()
        assert len(prefix) < 63
        num = length - len(prefix)
        name = num * 'a'

        bucket_name = '{prefix}{name}'.format(
            prefix=prefix,
            name=name,
        )
        client = get_client()
        response = client.create_bucket(Bucket=bucket_name)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/60 byte name')
    # @attr(assertion='fails with subdomain')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_good_long_60(self):
        self._test_bucket_create_naming_good_long(60)

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/61 byte name')
    # @attr(assertion='fails with subdomain')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_good_long_61(self):
        self._test_bucket_create_naming_good_long(61)

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/62 byte name')
    # @attr(assertion='fails with subdomain')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_good_long_62(self):
        self._test_bucket_create_naming_good_long(62)

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/63 byte name')
    # @attr(assertion='fails with subdomain')
    def test_bucket_create_naming_good_long_63(self):
        self._test_bucket_create_naming_good_long(63)

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list w/61 byte name')
    # @attr(assertion='fails with subdomain')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_list_long_name(self):
        prefix = get_new_bucket_name()
        length = 61
        num = length - len(prefix)
        name = num * 'a'

        bucket_name = '{prefix}{name}'.format(
            prefix=prefix,
            name=name,
        )
        bucket = get_new_bucket_resource(name=bucket_name)
        is_empty = self._bucket_is_empty(bucket)
        self.assertEqual(is_empty, True)

    # test_bucket_create_naming_dns_* are valid but not recommended
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/underscore in name')
    # @attr(assertion='fails')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_dns_underscore(self):
        self.skipTest("sequoias3对桶名字符没有限制，桶名长度范围[3,63]")
        invalid_bucketname = 'foo_bar'
        status, error_code = self.check_invalid_bucketname(invalid_bucketname)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidBucketName')

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/100 byte name')
    # @attr(assertion='fails with subdomain')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    def test_bucket_create_naming_dns_long(self):
        prefix = get_prefix()
        assert len(prefix) < 50
        num = 63 - len(prefix)
        self.check_good_bucket_name(num * 'a')

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/dash at end of name')
    # @attr(assertion='fails')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_dns_dash_at_end(self):
        self.skipTest("sequoias3对桶名字符没有限制，桶名长度范围[3,63]")
        invalid_bucketname = 'foo-'
        status, error_code = self.check_invalid_bucketname(invalid_bucketname)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidBucketName')

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/.. in name')
    # @attr(assertion='fails')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_dns_dot_dot(self):
        self.skipTest("sequoias3对桶名字符没有限制，桶名长度范围[3,63]")
        invalid_bucketname = 'foo..bar'
        status, error_code = self.check_invalid_bucketname(invalid_bucketname)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidBucketName')

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/.- in name')
    # @attr(assertion='fails')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_dns_dot_dash(self):
        self.skipTest("sequoias3对桶名字符没有限制，桶名长度范围[3,63]")
        invalid_bucketname = 'foo.-bar'
        status, error_code = self.check_invalid_bucketname(invalid_bucketname)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidBucketName')

    # Breaks DNS with SubdomainCallingFormat
    # @attr('fails_with_subdomain')
    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create w/-. in name')
    # @attr(assertion='fails')
    # @attr('fails_on_aws') # <Error><Code>InvalidBucketName</Code><Message>The specified bucket is not valid.</Message>...</Error>
    # Should now pass on AWS even though it has 'fails_on_aws' attr.
    def test_bucket_create_naming_dns_dash_dot(self):
        self.skipTest("sequoias3对桶名字符没有限制，桶名长度范围[3,63]")
        invalid_bucketname = 'foo-.bar'
        status, error_code = self.check_invalid_bucketname(invalid_bucketname)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidBucketName')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='re-create')
    def test_bucket_create_exists(self):
        # aws-s3 default region allows recreation of buckets
        # but all other regions fail with BucketAlreadyOwnedByYou.
        bucket_name = get_new_bucket_name()
        client = get_client()

        client.create_bucket(Bucket=bucket_name)
        try:
            response = client.create_bucket(Bucket=bucket_name)
        except ClientError as e:
            status, error_code = _get_status_and_error_code(e.response)
            self.assertEqual(status, 409)
            self.assertEqual(error_code, 'BucketAlreadyOwnedByYou')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='get location')
    def test_bucket_get_location(self):
        self.skipTest("sequoias3 不支持LocationConstraint")
        location_constraint = get_main_api_name()
        if not location_constraint:
            raise self.skipTest("skip...")
        bucket_name = get_new_bucket_name()
        client = get_client()
        client.create_bucket(Bucket=bucket_name, CreateBucketConfiguration={'LocationConstraint': location_constraint})

        response = client.get_bucket_location(Bucket=bucket_name)
        if location_constraint == "":
            location_constraint = None
        self.assertEqual(response['LocationConstraint'], location_constraint)

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='re-create by non-owner')
    # @attr(assertion='fails 409')
    def test_bucket_create_exists_nonowner(self):
        # Names are shared across a global namespace. As such, no two
        # users can create a bucket with that same name.
        bucket_name = get_new_bucket_name()
        client = get_client()

        alt_client = get_alt_client()

        client.create_bucket(Bucket=bucket_name)
        e = assert_raises(ClientError, alt_client.create_bucket, Bucket=bucket_name)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 409)
        self.assertEqual(error_code, 'BucketAlreadyOwnedByYou')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all buckets')
    # @attr(assertion='returns all expected buckets')
    def test_buckets_create_then_list(self):
        client = get_client()
        bucket_names = []
        for i in range(5):
            bucket_name = get_new_bucket_name()
            bucket_names.append(bucket_name)

        for name in bucket_names:
            client.create_bucket(Bucket=name)

        response = client.list_buckets()
        bucket_dicts = response['Buckets']
        buckets_list = []

        buckets_list = get_buckets_list()

        for name in bucket_names:
            if name not in buckets_list:
                raise RuntimeError("S3 implementation's GET on Service did not return bucket we created: %r",
                                   name)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all buckets (bad auth)')
    # @attr(assertion='fails 403')
    def test_list_buckets_invalid_auth(self):
        bad_auth_client = get_bad_auth_client()
        e = assert_raises(ClientError, bad_auth_client.list_buckets)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'InvalidAccessKeyId')

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='list all buckets (bad auth)')
    # @attr(assertion='fails 403')
    def test_list_buckets_bad_auth(self):
        main_access_key = get_main_aws_access_key()
        bad_auth_client = get_bad_auth_client(aws_access_key_id=main_access_key + "a")
        e = assert_raises(ClientError, bad_auth_client.list_buckets)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
        self.assertEqual(error_code, 'InvalidAccessKeyId')

    def test_bucket_create_naming_good_starts_alpha(self):
        self.check_good_bucket_name('foo', _prefix='a' + get_prefix())

    def test_bucket_create_naming_good_starts_digit(self):
        self.check_good_bucket_name('foo', _prefix='0' + get_prefix())

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create bucket')
    # @attr(assertion='name containing dot works')
    def test_bucket_create_naming_good_contains_period(self):
        self.check_good_bucket_name('aaa.111')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create bucket')
    # @attr(assertion='name containing hyphen works')
    def test_bucket_create_naming_good_contains_hyphen(self):
        self.check_good_bucket_name('aaa-111')

    # @attr(resource='bucket')
    # @attr(method='put')
    # @attr(operation='create bucket with objects and recreate it')
    # @attr(assertion='bucket recreation not overriding index')
    def test_bucket_recreate_not_overriding(self):
        self.skipTest("sequoias3重复创建桶会报错")
        key_names = ['mykey1', 'mykey2']
        bucket_name = self._create_objects(keys=key_names)

        objs_list = get_objects_list(bucket_name)
        self.assertEqual(key_names, objs_list)

        client = get_client()
        client.create_bucket(Bucket=bucket_name)

        objs_list = get_objects_list(bucket_name)
        self.assertEqual(key_names, objs_list)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='create and list objects with special names')
    # @attr(assertion='special names work')
    def test_bucket_create_special_key_names(self):
        key_names = [
            ' ',
            '"',
            '$',
            '%',
            '&',
            '\'',
            '<',
            '>',
            '_',
            '_ ',
            '_ _',
            '__',
        ]

        bucket_name = self._create_objects(keys=key_names)

        objs_list = get_objects_list(bucket_name)
        self.assertEqual(key_names, objs_list)

        client = get_client()

        for name in key_names:
            self.assertEqual((name in objs_list), True)
            response = client.get_object(Bucket=bucket_name, Key=name)
            body = self._get_body(response)
            self.assertEqual(name.encode(), body)

    # @attr(resource='bucket')
    # @attr(method='get')
    # @attr(operation='create and list objects with underscore as prefix, list using prefix')
    # @attr(assertion='listing works correctly')
    def test_bucket_list_special_prefix(self):
        key_names = ['_bla/1', '_bla/2', '_bla/3', '_bla/4', 'abcd']
        bucket_name = self._create_objects(keys=key_names)

        objs_list = get_objects_list(bucket_name)

        self.assertEqual(len(objs_list), 5)

        objs_list = get_objects_list(bucket_name, prefix='_bla/')
        self.assertEqual(len(objs_list), 4)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns correct data, 206')
    def test_ranged_request_response_code(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=4-7')
        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content[4:8].encode())
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-range'], 'bytes 4-7/11')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 206)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns correct data, 206')
    def test_ranged_big_request_response_code(self):
        content = os.urandom(8 * 1024 * 1024)

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=3145728-5242880')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content[3145728:5242881])
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-range'], 'bytes 3145728-5242880/8388608')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 206)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns correct data, 206')
    def test_ranged_request_skip_leading_bytes_response_code(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=4-')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content[4:].encode())
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-range'], 'bytes 4-10/11')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 206)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns correct data, 206')
    def test_ranged_request_return_trailing_bytes_response_code(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=-7')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content[-7:].encode())
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['content-range'], 'bytes 4-10/11')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 206)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 206')
    def test_ranged_request_random_range1(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=-4-7')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range3(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=0-')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range4(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=-1-')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range10(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=1--')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range5(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=4-7-')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range6(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=1--3')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range7(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=--1')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range8(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=--')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range9(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=a-b')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range11(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=ab')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range12(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=1')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns all content, 200')
    def test_ranged_request_random_range13(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)
        response = client.get_object(Bucket=bucket_name, Key='testobj', Range='bytes=')

        fetched_content = self._get_body(response)
        self.assertEqual(fetched_content, content.encode())
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns invalid range, 416')
    def test_ranged_request_invalid_range1(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)

        # test invalid range
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='testobj', Range='bytes=40-50')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 416)
        self.assertEqual(error_code, 'InvalidRange')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns invalid range, 416')
    def test_ranged_request_invalid_range1(self):
        content = 'testcontent'

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)

        # test invalid range
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='testobj', Range='bytes=-0')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 416)
        self.assertEqual(error_code, 'InvalidRange')

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='range')
    # @attr(assertion='returns invalid range, 416')
    def test_ranged_request_empty_object(self):
        content = ''

        bucket_name = get_new_bucket()
        client = get_client()

        client.put_object(Bucket=bucket_name, Key='testobj', Body=content)

        # test invalid range
        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key='testobj', Range='bytes=40-50')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 416)
        self.assertEqual(error_code, 'InvalidRange')

    # @attr(resource='bucket')
    # @attr(method='create')
    # @attr(operation='create versioned bucket')
    # @attr(assertion='can create and suspend bucket versioning')
    # @attr('versioning')
    def test_versioning_bucket_create_suspend(self):
        bucket_name = get_new_bucket()
        self.check_versioning(bucket_name, None)

        self.check_configure_versioning_retry(bucket_name, "Suspended", "Suspended")
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        self.check_configure_versioning_retry(bucket_name, "Suspended", "Suspended")

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
            body = 'content-{i}'.format(i=i)
            response = client.put_object(Bucket=bucket_name, Key=key, Body=body)
            version_id = response['VersionId']

            contents.append(body)
            version_ids.append(version_id)

        if check_versions:
            self.check_obj_versions(client, bucket_name, key, version_ids, contents)

        return version_ids, contents

    def remove_obj_version(self, client, bucket_name, key, version_ids, contents, index):
        self.assertEqual(len(version_ids), len(contents))
        index = index % len(version_ids)
        rm_version_id = version_ids.pop(index)
        rm_content = contents.pop(index)

        self.check_obj_content(client, bucket_name, key, rm_version_id, rm_content)

        client.delete_object(Bucket=bucket_name, Key=key, VersionId=rm_version_id)

        if len(version_ids) != 0:
            self.check_obj_versions(client, bucket_name, key, version_ids, contents)

    def _do_test_create_remove_versions(self, client, bucket_name, key, num_versions, remove_start_idx, idx_inc):
        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        idx = remove_start_idx

        for j in range(num_versions):
            self.remove_obj_version(client, bucket_name, key, version_ids, contents, idx)
            idx += idx_inc

        response = client.list_object_versions(Bucket=bucket_name)
        if 'Versions' in response:
            print(response['Versions'])

    # @attr(resource='object')
    # @attr(method='create')
    # @attr(operation='create and remove versioned object')
    # @attr(assertion='can create access and remove appropriate versions')
    # @attr('versioning')
    def test_versioning_obj_create_read_remove(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_bucket_versioning(Bucket=bucket_name,
                                     VersioningConfiguration={'Status': 'Enabled'})
        key = 'testobj'
        num_versions = 5

        # (self, client, bucket_name, key, num_versions, remove_start_idx, idx_inc)
        self._do_test_create_remove_versions(client, bucket_name, key, num_versions, -1, 0)
        self._do_test_create_remove_versions(client, bucket_name, key, num_versions, -1, 0)
        self._do_test_create_remove_versions(client, bucket_name, key, num_versions, 0, 0)
        self._do_test_create_remove_versions(client, bucket_name, key, num_versions, 1, 0)
        self._do_test_create_remove_versions(client, bucket_name, key, num_versions, 4, -1)
        self._do_test_create_remove_versions(client, bucket_name, key, num_versions, 3, 3)

    # @attr(resource='object')
    # @attr(method='create')
    # @attr(operation='create and remove versioned object and head')
    # @attr(assertion='can create access and remove appropriate versions')
    # @attr('versioning')
    def test_versioning_obj_create_read_remove_head(self):
        self.skipTest("sequoias3不支持MFADelete")
        bucket_name = get_new_bucket()

        client = get_client()
        client.put_bucket_versioning(Bucket=bucket_name,
                                     VersioningConfiguration={'MFADelete': 'Disabled', 'Status': 'Enabled'})
        key = 'testobj'
        num_versions = 5

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        # removes old head object, checks new one
        removed_version_id = version_ids.pop()
        contents.pop()
        num_versions = num_versions - 1

        response = client.delete_object(Bucket=bucket_name, Key=key, VersionId=removed_version_id)
        response = client.get_object(Bucket=bucket_name, Key=key)
        body = self._get_body(response)
        self.assertEqual(body, contents[-1])

        # add a delete marker
        response = client.delete_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['DeleteMarker'], True)

        delete_marker_version_id = response['VersionId']
        version_ids.append(delete_marker_version_id)

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(len(response['Versions']), num_versions)
        self.assertEqual(len(response['DeleteMarkers']), 1)
        self.assertEqual(response['DeleteMarkers'][0]['VersionId'], delete_marker_version_id)

        self.clean_up_bucket(client, bucket_name, key, version_ids)

    # @attr(resource='object')
    # @attr(method='create')
    # @attr(operation='create object, then switch to versioning')
    # @attr(assertion='behaves correctly')
    # @attr('versioning')
    def test_versioning_obj_plain_null_version_removal(self):
        bucket_name = get_new_bucket()
        self.check_versioning(bucket_name, None)

        client = get_client()
        key = 'testobjfoo'
        content = 'fooz'
        client.put_object(Bucket=bucket_name, Key=key, Body=content)

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        client.delete_object(Bucket=bucket_name, Key=key, VersionId='null')

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchKey')

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False)

    # amazon is eventually consistent, retry a bit if failed
    def check_configure_versioning_retry(self, bucket_name, status, expected_string):
        client = get_client()

        response = client.put_bucket_versioning(Bucket=bucket_name,
                                                VersioningConfiguration={'Status': status})

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

    def check_versioning(self, bucket_name, status):
        client = get_client()
        try:
            response = client.get_bucket_versioning(Bucket=bucket_name)
            self.assertEqual(response['Status'], status)
        except KeyError:
            self.assertEqual(status, None)

    # @attr(resource='object')
    # @attr(method='create')
    # @attr(operation='create object, then switch to versioning')
    # @attr(assertion='behaves correctly')
    # @attr('versioning')
    def test_versioning_obj_plain_null_version_overwrite(self):
        bucket_name = get_new_bucket()
        self.check_versioning(bucket_name, None)

        client = get_client()
        key = 'testobjfoo'
        content = 'fooz'
        client.put_object(Bucket=bucket_name, Key=key, Body=content)

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        content2 = 'zzz'
        response = client.put_object(Bucket=bucket_name, Key=key, Body=content2)
        response = client.get_object(Bucket=bucket_name, Key=key)
        body = self._get_body(response)
        self.assertEqual(body, content2.encode())

        version_id = response['VersionId']
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id)
        response = client.get_object(Bucket=bucket_name, Key=key)
        body = self._get_body(response)
        self.assertEqual(body, content.encode())

        client.delete_object(Bucket=bucket_name, Key=key, VersionId='null')

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchKey')

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False)

    # @attr(resource='object')
    # @attr(method='create')
    # @attr(operation='create object, then switch to versioning')
    # @attr(assertion='behaves correctly')
    # @attr('versioning')
    def test_versioning_obj_plain_null_version_overwrite_suspended(self):
        bucket_name = get_new_bucket()
        self.check_versioning(bucket_name, None)

        client = get_client()
        key = 'testobjbar'
        content = 'foooz'
        client.put_object(Bucket=bucket_name, Key=key, Body=content)

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        self.check_configure_versioning_retry(bucket_name, "Suspended", "Suspended")

        content2 = 'zzz'
        response = client.put_object(Bucket=bucket_name, Key=key, Body=content2)
        response = client.get_object(Bucket=bucket_name, Key=key)
        body = self._get_body(response)
        self.assertEqual(body, content2.encode())

        response = client.list_object_versions(Bucket=bucket_name)
        # original object with 'null' version id still counts as a version
        self.assertEqual(len(response['Versions']), 1)

        client.delete_object(Bucket=bucket_name, Key=key, VersionId='null')

        e = assert_raises(ClientError, client.get_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchKey')

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False)

    def delete_suspended_versioning_obj(self, client, bucket_name, key, version_ids, contents):
        client.delete_object(Bucket=bucket_name, Key=key)

        # clear out old null objects in lists since they will get overwritten
        self.assertEqual(len(version_ids), len(contents))
        i = 0
        for version_id in version_ids:
            if version_id == 'null':
                version_ids.pop(i)
                contents.pop(i)
            i += 1

        return version_ids, contents

    def overwrite_suspended_versioning_obj(self, client, bucket_name, key, version_ids, contents, content):
        client.put_object(Bucket=bucket_name, Key=key, Body=content)

        # clear out old null objects in lists since they will get overwritten
        self.assertEqual(len(version_ids), len(contents))
        i = 0
        for version_id in version_ids:
            if version_id == 'null':
                version_ids.pop(i)
                contents.pop(i)
            i += 1

        # add new content with 'null' version id to the end
        contents.append(content)
        version_ids.append('null')

        return version_ids, contents

    # @attr(resource='object')
    # @attr(method='create')
    # @attr(operation='suspend versioned bucket')
    # @attr(assertion='suspended versioning behaves correctly')
    # @attr('versioning')
    def test_versioning_obj_suspend_versions(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'testobj'
        num_versions = 5

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        self.check_configure_versioning_retry(bucket_name, "Suspended", "Suspended")

        self.delete_suspended_versioning_obj(client, bucket_name, key, version_ids, contents)
        self.delete_suspended_versioning_obj(client, bucket_name, key, version_ids, contents)

        self.overwrite_suspended_versioning_obj(client, bucket_name, key, version_ids, contents, 'null content 1')
        self.overwrite_suspended_versioning_obj(client, bucket_name, key, version_ids, contents, 'null content 2')
        self.delete_suspended_versioning_obj(client, bucket_name, key, version_ids, contents)
        self.overwrite_suspended_versioning_obj(client, bucket_name, key, version_ids, contents, 'null content 3')
        self.delete_suspended_versioning_obj(client, bucket_name, key, version_ids, contents)

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, 3, version_ids, contents)
        num_versions += 3

        for idx in range(num_versions):
            self.remove_obj_version(client, bucket_name, key, version_ids, contents, idx)

        self.assertEqual(len(version_ids), 0)
        self.assertEqual(len(version_ids), len(contents))

    # @attr(resource='object')
    # @attr(method='remove')
    # @attr(operation='create and remove versions')
    # @attr(assertion='everything works')
    # @attr('versioning')
    def test_versioning_obj_create_versions_remove_all(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'testobj'
        num_versions = 10

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)
        for idx in range(num_versions):
            self.remove_obj_version(client, bucket_name, key, version_ids, contents, idx)

        self.assertEqual(len(version_ids), 0)
        self.assertEqual(len(version_ids), len(contents))

    # @attr(resource='object')
    # @attr(method='remove')
    # @attr(operation='create and remove versions')
    # @attr(assertion='everything works')
    # @attr('versioning')
    def test_versioning_obj_create_versions_remove_special_names(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        keys = ['_testobj', '_', ':', ' ']
        num_versions = 10

        for key in keys:
            (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)
            for idx in range(num_versions):
                self.remove_obj_version(client, bucket_name, key, version_ids, contents, idx)

            self.assertEqual(len(version_ids), 0)
            self.assertEqual(len(version_ids), len(contents))

    def gen_rand_string(self, size, chars=string.ascii_uppercase + string.digits):
        return ''.join(random.choice(chars) for _ in range(size))

    def _do_test_multipart_upload_contents(self, bucket_name, key, num_parts):
        payload = self.gen_rand_string(5) * 1024 * 1024
        client = get_client()

        response = client.create_multipart_upload(Bucket=bucket_name, Key=key)
        upload_id = response['UploadId']

        parts = []

        for part_num in range(0, num_parts):
            part = payload.encode()
            response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=part_num + 1,
                                          Body=part)
            parts.append({'ETag': response['ETag'].strip('"'), 'PartNumber': part_num + 1})

        last_payload = '123' * 1024 * 1024
        last_part = last_payload.encode()
        response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=num_parts + 1,
                                      Body=last_part)
        parts.append({'ETag': response['ETag'].strip('"'), 'PartNumber': num_parts + 1})

        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.get_object(Bucket=bucket_name, Key=key)
        test_string = self._get_body(response)

        all_payload = payload * num_parts + last_payload

        assert test_string == all_payload.encode()

        return all_payload

    # @attr(resource='object')
    # @attr(method='multipart')
    # @attr(operation='create and test multipart object')
    # @attr(assertion='everything works')
    # @attr('versioning')
    def test_versioning_obj_create_overwrite_multipart(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'testobj'
        num_versions = 3
        contents = []
        version_ids = []

        for i in range(num_versions):
            ret = self._do_test_multipart_upload_contents(bucket_name, key, 3)
            contents.append(ret)

        response = client.list_object_versions(Bucket=bucket_name)
        for version in response['Versions']:
            version_ids.append(version['VersionId'])

        version_ids.reverse()
        self.check_obj_versions(client, bucket_name, key, version_ids, contents)

        for idx in range(num_versions):
            self.remove_obj_version(client, bucket_name, key, version_ids, contents, idx)

        self.assertEqual(len(version_ids), 0)
        self.assertEqual(len(version_ids), len(contents))

    # @attr(resource='object')
    # @attr(method='multipart')
    # @attr(operation='list versioned objects')
    # @attr(assertion='everything works')
    # @attr('versioning')
    def test_versioning_obj_list_marker(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'testobj'
        key2 = 'testobj-1'
        num_versions = 5

        contents = []
        version_ids = []
        contents2 = []
        version_ids2 = []

        # for key #1
        for i in range(num_versions):
            body = 'content-{i}'.format(i=i)
            response = client.put_object(Bucket=bucket_name, Key=key, Body=body)
            version_id = response['VersionId']

            contents.append(body)
            version_ids.append(version_id)

        # for key #2
        for i in range(num_versions):
            body = 'content-{i}'.format(i=i)
            response = client.put_object(Bucket=bucket_name, Key=key2, Body=body)
            version_id = response['VersionId']

            contents2.append(body)
            version_ids2.append(version_id)

        response = client.list_object_versions(Bucket=bucket_name)
        versions = response['Versions']
        # obj versions in versions come out created last to first not first to last like version_ids & contents
        versions.reverse()

        i = 0
        # test the last 5 created objects first
        for i in range(5):
            version = versions[i]
            self.assertEqual(version['VersionId'], version_ids2[i])
            self.assertEqual(version['Key'], key2)
            self.check_obj_content(client, bucket_name, key2, version['VersionId'], contents2[i])
            i += 1

        # then the first 5
        for j in range(5):
            version = versions[i]
            self.assertEqual(version['VersionId'], version_ids[j])
            self.assertEqual(version['Key'], key)
            self.check_obj_content(client, bucket_name, key, version['VersionId'], contents[j])
            i += 1

    # @attr(resource='object')
    # @attr(method='multipart')
    # @attr(operation='create and test versioned object copying')
    # @attr(assertion='everything works')
    # @attr('versioning')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_versioning_copy_obj_version(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'testobj'
        num_versions = 3

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        for i in range(num_versions):
            new_key_name = 'key_{i}'.format(i=i)
            copy_source = {'Bucket': bucket_name, 'Key': key, 'VersionId': version_ids[i]}
            client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key=new_key_name)
            response = client.get_object(Bucket=bucket_name, Key=new_key_name)
            body = self._get_body(response)
            self.assertEqual(body.decode(), contents[i])

        another_bucket_name = get_new_bucket()

        for i in range(num_versions):
            new_key_name = 'key_{i}'.format(i=i)
            copy_source = {'Bucket': bucket_name, 'Key': key, 'VersionId': version_ids[i]}
            client.copy_object(Bucket=another_bucket_name, CopySource=copy_source, Key=new_key_name)
            response = client.get_object(Bucket=another_bucket_name, Key=new_key_name)
            body = self._get_body(response)
            self.assertEqual(body.decode(), contents[i])

        new_key_name = 'new_key'
        copy_source = {'Bucket': bucket_name, 'Key': key}
        client.copy_object(Bucket=another_bucket_name, CopySource=copy_source, Key=new_key_name)

        response = client.get_object(Bucket=another_bucket_name, Key=new_key_name)
        body = self._get_body(response)
        self.assertEqual(body.decode(), contents[-1])

    # @attr(resource='object')
    # @attr(method='delete')
    # @attr(operation='delete multiple versions')
    # @attr(assertion='deletes multiple versions of an object with a single call')
    # @attr('versioning')
    def test_versioning_multi_object_delete(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'key'
        num_versions = 2

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        response = client.list_object_versions(Bucket=bucket_name)
        versions = response['Versions']
        versions.reverse()

        for version in versions:
            client.delete_object(Bucket=bucket_name, Key=key, VersionId=version['VersionId'])

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False)

        # now remove again, should all succeed due to idempotency
        for version in versions:
            client.delete_object(Bucket=bucket_name, Key=key, VersionId=version['VersionId'])

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False)

    # @attr(resource='object')
    # @attr(method='delete')
    # @attr(operation='delete multiple versions')
    # @attr(assertion='deletes multiple versions of an object and delete marker with a single call')
    # @attr('versioning')
    def test_versioning_multi_object_delete_with_marker(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'key'
        num_versions = 2

        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)

        client.delete_object(Bucket=bucket_name, Key=key)
        response = client.list_object_versions(Bucket=bucket_name)
        versions = response['Versions']
        delete_markers = response['DeleteMarkers']

        version_ids.append(delete_markers[0]['VersionId'])
        self.assertEqual(len(version_ids), 3)
        self.assertEqual(len(delete_markers), 1)

        for version in versions:
            client.delete_object(Bucket=bucket_name, Key=key, VersionId=version['VersionId'])

        for delete_marker in delete_markers:
            client.delete_object(Bucket=bucket_name, Key=key, VersionId=delete_marker['VersionId'])

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False)
        self.assertEqual(('DeleteMarkers' in response), False)

        for version in versions:
            client.delete_object(Bucket=bucket_name, Key=key, VersionId=version['VersionId'])

        for delete_marker in delete_markers:
            client.delete_object(Bucket=bucket_name, Key=key, VersionId=delete_marker['VersionId'])

        # now remove again, should all succeed due to idempotency
        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False)
        self.assertEqual(('DeleteMarkers' in response), False)

    # @attr(resource='object')
    # @attr(method='delete')
    # @attr(operation='multi delete create marker')
    # @attr(assertion='returns correct marker version id')
    # @attr('versioning')
    def test_versioning_multi_object_delete_with_marker_create(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'key'

        response = client.delete_object(Bucket=bucket_name, Key=key)
        delete_marker_version_id = response['VersionId']

        response = client.list_object_versions(Bucket=bucket_name)
        delete_markers = response['DeleteMarkers']

        self.assertEqual(len(delete_markers), 1)
        self.assertEqual(delete_marker_version_id, delete_markers[0]['VersionId'])
        self.assertEqual(key, delete_markers[0]['Key'])

    def _do_create_object(self, client, bucket_name, key, i):
        body = 'data {i}'.format(i=i)
        client.put_object(Bucket=bucket_name, Key=key, Body=body)

    def _do_remove_ver(self, client, bucket_name, key, version_id):
        client.delete_object(Bucket=bucket_name, Key=key, VersionId=version_id)

    def _do_create_versioned_obj_concurrent(self, client, bucket_name, key, num):
        t = []
        for i in range(num):
            thr = threading.Thread(target=self._do_create_object, args=(client, bucket_name, key, i))
            thr.start()
            t.append(thr)
        return t

    def _do_clear_versioned_bucket_concurrent(self, client, bucket_name):
        t = []
        response = client.list_object_versions(Bucket=bucket_name)
        for version in response.get('Versions', []):
            thr = threading.Thread(target=self._do_remove_ver,
                                   args=(client, bucket_name, version['Key'], version['VersionId']))
            thr.start()
            t.append(thr)
        return t

    def _do_wait_completion(self, t):
        for thr in t:
            thr.join()

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='concurrent creation of objects, concurrent removal')
    # @attr(assertion='works')
    # @attr('versioning')
    def test_versioned_concurrent_object_create_concurrent_remove(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'myobj'
        num_versions = 5

        for i in range(5):
            t = self._do_create_versioned_obj_concurrent(client, bucket_name, key, num_versions)
            self._do_wait_completion(t)

            response = client.list_object_versions(Bucket=bucket_name)
            versions = response['Versions']

            self.assertEqual(len(versions), num_versions)

            t = self._do_clear_versioned_bucket_concurrent(client, bucket_name)
            self._do_wait_completion(t)

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False, response)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='concurrent creation and removal of objects')
    # @attr(assertion='works')
    # @attr('versioning')
    def test_versioned_concurrent_object_create_and_remove(self):
        bucket_name = get_new_bucket()
        client = get_client()

        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key = 'myobj'
        num_versions = 3

        all_threads = []

        for i in range(3):
            t = self._do_create_versioned_obj_concurrent(client, bucket_name, key, num_versions)
            all_threads.append(t)

            t = self._do_clear_versioned_bucket_concurrent(client, bucket_name)
            all_threads.append(t)

        for t in all_threads:
            self._do_wait_completion(t)

        t = self._do_clear_versioned_bucket_concurrent(client, bucket_name)
        self._do_wait_completion(t)

        response = client.list_object_versions(Bucket=bucket_name)
        self.assertEqual(('Versions' in response), False, response)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='x-amz-expires check not expired')
    # @attr(assertion='succeeds')
    def test_object_raw_get_x_amz_expires_not_expired(self):
        bucket_name = get_new_bucket()
        client = get_alt_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        params = {'Bucket': bucket_name, 'Key': 'foo'}
        url = client.generate_presigned_url(ClientMethod='get_object', Params=params, ExpiresIn=3600,
                                            HttpMethod='GET')
        res = requests.get(url).__dict__
        self.assertEqual(res['status_code'], 200)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='check x-amz-expires value out of positive range')
    # @attr(assertion='succeeds')
    def test_object_raw_get_x_amz_expires_out_positive_range(self):
        bucket_name = get_new_bucket()
        client = get_alt_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        params = {'Bucket': bucket_name, 'Key': 'foo'}
        url = client.generate_presigned_url(ClientMethod='get_object', Params=params, ExpiresIn=-360, HttpMethod='GET')
        res = requests.get(url).__dict__
        self.assertEqual(res['status_code'], 403)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='check version x-amz-expires')
    # @attr('versioning')
    def test_object_raw_get_x_amz_expires_version(self):
        bucket_name = get_new_bucket()
        client = get_alt_client()
        client.put_bucket_versioning(Bucket=bucket_name,VersioningConfiguration={'Status': 'Enabled'})
        key = 'foo'
        num_versions = 5
        (version_ids, contents) = self.create_multiple_versions(client, bucket_name, key, num_versions)
        for i in range(num_versions):
            params = {'Bucket': bucket_name, 'Key': key, 'VersionId': version_ids[i]}
            url = client.generate_presigned_url(ClientMethod='get_object', Params=params, ExpiresIn=6000,
                                                HttpMethod='GET')
            res = requests.get(url).__dict__
            self.assertEqual(res['status_code'], 200)
            self.assertEqual(res['_content'], str.encode(contents[i]))

    def setUp(self):
        # print(self.__module__ + " setup: " + str(datetime.now()))
        setup()

    def tearDown(self):
        # print(self.__module__ + " tearDown: " + str(datetime.now()))
        teardown()


if __name__ == '__main__':
    unittest.main()
