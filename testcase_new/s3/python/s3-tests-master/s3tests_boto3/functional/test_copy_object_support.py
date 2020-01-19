
from botocore.exceptions import ClientError
import unittest
import time
import datetime

from lib.testlib import S3TestBase
from .utils import assert_raises
from .utils import _get_status_and_error_code
from .utils import _get_status
from .file_utils import *

from . import (
    setup,
    teardown,
    get_client,
    get_new_bucket,
    get_new_bucket_name,
    get_new_bucket_resource,
    get_main_display_name,
    get_main_user_id,
    get_alt_user_id,
    get_alt_client
)


class TestCopyObject(S3TestBase):

    def _get_body(self, response):
        body = response['Body']
        got = body.read()
        return got

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

    @staticmethod
    def add_bucket_user_grant(bucket_name, grant):
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

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy zero sized object in same bucket')
    # @attr(assertion='works')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_zero_size(self):
        key = 'foo123bar'
        bucket_name = self._create_objects(keys=[key])
        fp_a = FakeWriteFile(0, '')
        client = get_client()
        client.put_object(Bucket=bucket_name, Key=key, Body=fp_a)
        copy_source = {'Bucket': bucket_name, 'Key': key}
        client.copy(copy_source, bucket_name, 'bar321foo')
        response = client.get_object(Bucket=bucket_name, Key='bar321foo')
        self.assertEqual(response['ContentLength'], 0)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object in same bucket')
    # @attr(assertion='works')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_same_bucket(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo123bar', Body='foo')

        copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}

        client.copy(copy_source, bucket_name, 'bar321foo')

        response = client.get_object(Bucket=bucket_name, Key='bar321foo')
        body = self._get_body(response)
        self.assertEqual(b'foo', body)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object with content-type')
    # @attr(assertion='works')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_verify_contenttype(self):
        bucket_name = get_new_bucket()
        client = get_client()

        content_type = 'text/bla'
        client.put_object(Bucket=bucket_name, ContentType=content_type, Key='foo123bar', Body='foo')

        copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}

        client.copy(copy_source, bucket_name, 'bar321foo')

        response = client.get_object(Bucket=bucket_name, Key='bar321foo')
        body = self._get_body(response)
        self.assertEqual(b'foo', body)
        response_content_type = response['ContentType']
        self.assertEqual(response_content_type, content_type)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object to itself')
    # @attr(assertion='fails')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_to_itself(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo123bar', Body='foo')

        copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}

        e = assert_raises(ClientError, client.copy, copy_source, bucket_name, 'foo123bar')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidRequest')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='modify object metadata by copying')
    # @attr(assertion='fails')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_to_itself_with_metadata(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo123bar', Body='foo')
        copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}
        metadata = {'foo': 'bar'}

        client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key='foo123bar', Metadata=metadata,
                           MetadataDirective='REPLACE')
        response = client.get_object(Bucket=bucket_name, Key='foo123bar')
        self.assertEqual(response['Metadata'], metadata)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object from different bucket')
    # @attr(assertion='works')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_diff_bucket(self):
        bucket_name1 = get_new_bucket()
        bucket_name2 = get_new_bucket()

        client = get_client()
        client.put_object(Bucket=bucket_name1, Key='foo123bar', Body='foo')

        copy_source = {'Bucket': bucket_name1, 'Key': 'foo123bar'}

        client.copy(copy_source, bucket_name2, 'bar321foo')

        response = client.get_object(Bucket=bucket_name2, Key='bar321foo')
        body = self._get_body(response)
        self.assertEqual(b'foo', body)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy to an inaccessible bucket')
    # @attr(assertion='fails w/AttributeError')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_not_owned_bucket(self):
        self.skipTest("暂时只有一个s3用户")
        client = get_client()
        alt_client = get_alt_client()
        bucket_name1 = get_new_bucket_name()
        bucket_name2 = get_new_bucket_name()
        client.create_bucket(Bucket=bucket_name1)
        alt_client.create_bucket(Bucket=bucket_name2)

        client.put_object(Bucket=bucket_name1, Key='foo123bar', Body='foo')

        copy_source = {'Bucket': bucket_name1, 'Key': 'foo123bar'}

        e = assert_raises(ClientError, alt_client.copy, copy_source, bucket_name2, 'bar321foo')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy a non-owned object in a non-owned bucket, but with perms')
    # @attr(assertion='works')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_not_owned_object_bucket(self):
        self.skipTest('seuqoias3 不支持AccessControlPolicy')
        client = get_client()
        alt_client = get_alt_client()
        bucket_name = get_new_bucket_name()
        client.create_bucket(Bucket=bucket_name)
        client.put_object(Bucket=bucket_name, Key='foo123bar', Body='foo')

        alt_user_id = get_alt_user_id()

        grant = {'Grantee': {'ID': alt_user_id, 'Type': 'CanonicalUser'}, 'Permission': 'FULL_CONTROL'}
        grants = self.add_obj_user_grant(bucket_name, 'foo123bar', grant)
        client.put_object_acl(Bucket=bucket_name, Key='foo123bar', AccessControlPolicy=grants)

        grant = self.add_bucket_user_grant(bucket_name, grant)
        client.put_bucket_acl(Bucket=bucket_name, AccessControlPolicy=grant)

        alt_client.get_object(Bucket=bucket_name, Key='foo123bar')

        copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}
        alt_client.copy(copy_source, bucket_name, 'bar321foo')

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

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object and change acl')
    # @attr(assertion='works')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_canned_acl(self):
        bucket_name = get_new_bucket()
        client = get_client()
        alt_client = get_alt_client()
        client.put_object(Bucket=bucket_name, Key='foo123bar', Body='foo')

        copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}
        client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key='bar321foo', ACL='public-read')
        # check ACL is applied by doing GET from another user
        alt_client.get_object(Bucket=bucket_name, Key='bar321foo')

        metadata = {'abc': 'def'}
        copy_source = {'Bucket': bucket_name, 'Key': 'bar321foo'}
        client.copy_object(ACL='public-read', Bucket=bucket_name, CopySource=copy_source, Key='foo123bar',
                           Metadata=metadata, MetadataDirective='REPLACE')
        # check ACL is applied by doing GET from another user
        alt_client.get_object(Bucket=bucket_name, Key='foo123bar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object and retain metadata')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_retaining_metadata(self):
        for size in [3, 1024 * 1024]:
            bucket_name = get_new_bucket()
            client = get_client()
            content_type = 'audio/ogg'

            metadata = {'key1': 'value1', 'key2': 'value2'}
            client.put_object(Bucket=bucket_name, Key='foo123bar', Metadata=metadata, ContentType=content_type,
                              Body=bytearray(size))

            copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}
            client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key='bar321foo')

            response = client.get_object(Bucket=bucket_name, Key='bar321foo')
            self.assertEqual(content_type, response['ContentType'])
            self.assertEqual(metadata, response['Metadata'])
            self.assertEqual(size, response['ContentLength'])

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object and replace metadata')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_replacing_metadata(self):
        for size in [3, 1024 * 1024]:
            bucket_name = get_new_bucket()
            client = get_client()
            content_type = 'audio/ogg'

            metadata = {'key1': 'value1', 'key2': 'value2'}
            client.put_object(Bucket=bucket_name, Key='foo123bar', Metadata=metadata, ContentType=content_type,
                              Body=bytearray(size))

            metadata = {'key3': 'value3', 'key2': 'value2'}
            content_type = 'audio/mpeg'

            copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}
            client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key='bar321foo', Metadata=metadata,
                               MetadataDirective='REPLACE', ContentType=content_type)

            response = client.get_object(Bucket=bucket_name, Key='bar321foo')
            self.assertEqual(content_type, response['ContentType'])
            self.assertEqual(metadata, response['Metadata'])
            self.assertEqual(size, response['ContentLength'])

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy from non-existent bucket')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_bucket_not_found(self):
        bucket_name = get_new_bucket()
        client = get_client()

        copy_source = {'Bucket': bucket_name + "-fake", 'Key': 'foo123bar'}
        e = assert_raises(ClientError, client.copy, copy_source, bucket_name, 'bar321foo')
        status = _get_status(e.response)
        self.assertEqual(status, 404)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy from non-existent object')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_key_not_found(self):
        bucket_name = get_new_bucket()
        client = get_client()

        copy_source = {'Bucket': bucket_name, 'Key': 'foo123bar'}
        e = assert_raises(ClientError, client.copy, copy_source, bucket_name, 'bar321foo')
        status = _get_status(e.response)
        self.assertEqual(status, 404)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object to/from versioned bucket')
    # @attr(assertion='works')
    # @attr('versioning')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_versioned_bucket(self):
        self.skipTest('seuqoias3 不支持MFADelete')
        bucket_name = get_new_bucket()
        client = get_client()
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        size = 1 * 1024 * 124
        data = str(bytearray(size))
        key1 = 'foo123bar'
        client.put_object(Bucket=bucket_name, Key=key1, Body=data)

        response = client.get_object(Bucket=bucket_name, Key=key1)
        version_id = response['VersionId']

        # copy object in the same bucket
        copy_source = {'Bucket': bucket_name, 'Key': key1, 'VersionId': version_id}
        key2 = 'bar321foo'
        client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key=key2)
        response = client.get_object(Bucket=bucket_name, Key=key2)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(size, response['ContentLength'])

        # second copy
        version_id2 = response['VersionId']
        copy_source = {'Bucket': bucket_name, 'Key': key2, 'VersionId': version_id2}
        key3 = 'bar321foo2'
        client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key=key3)
        response = client.get_object(Bucket=bucket_name, Key=key3)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(size, response['ContentLength'])

        # copy to another versioned bucket
        bucket_name2 = get_new_bucket()
        self.check_configure_versioning_retry(bucket_name2, "Enabled", "Enabled")
        copy_source = {'Bucket': bucket_name, 'Key': key1, 'VersionId': version_id}
        key4 = 'bar321foo3'
        client.copy_object(Bucket=bucket_name2, CopySource=copy_source, Key=key4)
        response = client.get_object(Bucket=bucket_name2, Key=key4)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(size, response['ContentLength'])

        # copy to another non versioned bucket
        bucket_name3 = get_new_bucket()
        copy_source = {'Bucket': bucket_name, 'Key': key1, 'VersionId': version_id}
        key5 = 'bar321foo4'
        client.copy_object(Bucket=bucket_name3, CopySource=copy_source, Key=key5)
        response = client.get_object(Bucket=bucket_name3, Key=key5)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(size, response['ContentLength'])

        # copy from a non versioned bucket
        copy_source = {'Bucket': bucket_name3, 'Key': key5}
        key6 = 'foo123bar2'
        client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key=key6)
        response = client.get_object(Bucket=bucket_name, Key=key6)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(size, response['ContentLength'])

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='copy object to/from versioned bucket with url-encoded name')
    # @attr(assertion='works')
    # @attr('versioning')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_versioned_url_encoding(self):
        self.skipTest('seuqoias3 不支持MFADelete')
        bucket = get_new_bucket_resource()
        self.check_configure_versioning_retry(bucket.name, "Enabled", "Enabled")
        src_key = 'foo?bar'
        src = bucket.put_object(Key=src_key)
        src.load()  # HEAD request tests that the key exists

        # copy object in the same bucket
        dst_key = 'bar&foo'
        dst = bucket.Object(dst_key)
        dst.copy_from(CopySource={'Bucket': src.bucket_name, 'Key': src.key, 'VersionId': src.version_id})
        dst.load()  # HEAD request tests that the key exists

    # @attr(resource='object')
    # @attr(method='copy')
    # @attr(operation='copy w/ x-amz-copy-source-if-match: the latest ETag')
    # @attr(assertion='succeeds')
    def test_copy_object_ifmatch_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        resp = client.put_object(Bucket=bucket_name, Key='foo', Body='bar')
        client.copy_object(Bucket=bucket_name, CopySource=bucket_name + '/foo', CopySourceIfMatch=resp['ETag'],
                           Key='bar')
        resp = client.get_object(Bucket=bucket_name, Key='bar')
        self.assertEqual(resp['Body'].read(), b'bar')

    # @attr(resource='object')
    # @attr(method='copy')
    # @attr(operation='copy w/ x-amz-copy-source-if-match: bogus ETag')
    # @attr(assertion='fails 412')
    def test_copy_object_ifmatch_failed(self):
        bucket_name = get_new_bucket()
        client = get_client()
        client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        e = assert_raises(ClientError, client.copy_object, Bucket=bucket_name, CopySource=bucket_name + '/foo',
                          CopySourceIfMatch='ABCORZ', Key='bar')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 412)
        self.assertEqual(error_code, 'PreconditionFailed')

    # @attr(resource='object')
    # @attr(method='copy')
    # @attr(operation='copy w/ x-amz-copy-source-if-none-match: the latest ETag')
    # @attr(assertion='fails 412')
    def test_copy_object_ifnonematch_good(self):
        bucket_name = get_new_bucket()
        client = get_client()
        resp = client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        e = assert_raises(ClientError, client.copy_object, Bucket=bucket_name, CopySource=bucket_name + '/foo',
                          CopySourceIfNoneMatch=resp['ETag'], Key='bar')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(error_code, str(304))
        self.assertEqual(status, 304)

    # @attr(resource='object')
    # @attr(method='copy')
    # @attr(operation='copy w/ x-amz-copy-source-if-none-match: bogus ETag')
    # @attr(assertion='succeeds')
    def test_copy_object_ifnonematch_failed(self):
        bucket_name = get_new_bucket()
        client = get_client()
        resp = client.put_object(Bucket=bucket_name, Key='foo', Body='bar')

        client.copy_object(Bucket=bucket_name, CopySource=bucket_name + '/foo', CopySourceIfNoneMatch='ABCORZ',
                           Key='bar')
        resp = client.get_object(Bucket=bucket_name, Key='bar')
        self.assertEqual(resp['Body'].read(), b'bar')

    def setUp(self):
        # print(self.__module__ + " setup: " + str(datetime))
        setup()

    def tearDown(self):
        # print(self.__module__ + " tearDown: " + str(datetime.now()))
        teardown()


if __name__ == '__main__':
    unittest.main()
