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


class TestTagging(unittest.TestCase):

    def _multipart_upload(self, bucket_name, key, size, part_size=5 * 1024 * 1024, client=None, content_type=None,
                          metadata=None,
                          resend_parts=[]):
        """
        generate a multi-part upload for a random file of specifed size,
        if requested, generate a list of the parts
        return the upload descriptor
        """
        if client is None:
            client = get_client()

        if content_type is None and metadata is None:
            response = client.create_multipart_upload(Bucket=bucket_name, Key=key)
        else:
            response = client.create_multipart_upload(Bucket=bucket_name, Key=key, Metadata=metadata,
                                                      ContentType=content_type)

        upload_id = response['UploadId']
        s = ''
        parts = []
        num = enumerate(generate_random(size, part_size))
        for i, part in enumerate(generate_random(size, part_size)):
            # part_num is necessary because PartNumber for upload_part and in parts must start at 1 and i starts at 0
            part_num = i + 1
            s += part
            response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=part_num,
                                          Body=part)
            parts.append({'ETag': response['ETag'].strip('"'), 'PartNumber': part_num})
            if i in resend_parts:
                client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=part_num, Body=part)
        return upload_id, s, parts

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

    def _create_objects(self, bucket=None, bucket_name=None, keys=[]):
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

    def _get_body(self, response):
        body = response['Body']
        got = body.read()
        return got

    def _get_post_url(self, bucket_name):
        endpoint = get_config_endpoint()
        return '{endpoint}/{bucket_name}'.format(endpoint=endpoint, bucket_name=bucket_name)

    def _create_key_with_random_content(self, keyname, size=7 * 1024 * 1024, bucket_name=None, client=None):
        if bucket_name is None:
            bucket_name = get_new_bucket()

        if client is None:
            client = get_client()
        # TODO:StringIO is not found
        data = StringIO(str(generate_random(size, size).next()))
        client.put_object(Bucket=bucket_name, Key=keyname, Body=data)

        return bucket_name

    def _create_simple_tagset(self, count):
        tagset = []
        for i in range(count):
            tagset.append({'Key': str(i), 'Value': str(i)})

        return {'TagSet': tagset}

    def _make_random_string(self, size):
        return ''.join(random.choice(string.ascii_letters) for _ in range(size))

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test Get/PutObjTagging output')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_get_obj_tagging(self):
        key = 'testputtags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        input_tagset = self._create_simple_tagset(2)
        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], input_tagset['TagSet'])

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test HEAD obj tagging output')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_get_obj_head_tagging(self):
        key = 'testputtags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()
        count = 2

        input_tagset = self._create_simple_tagset(count)
        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.head_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)
        self.assertEqual(response['ResponseMetadata']['HTTPHeaders']['x-amz-tagging-count'], str(count))

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test Put max allowed tags')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_put_max_tags(self):
        key = 'testputmaxtags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        input_tagset = self._create_simple_tagset(10)
        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], input_tagset['TagSet'])

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test Put max allowed tags')
    # @attr(assertion='fails')
    # @attr('tagging')
    def test_put_excess_tags(self):
        key = 'testputmaxtags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        input_tagset = self._create_simple_tagset(11)
        e = assert_raises(ClientError, client.put_object_tagging, Bucket=bucket_name, Key=key, Tagging=input_tagset)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidTag')

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(len(response['TagSet']), 0)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test Put max allowed k-v size')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_put_max_kvsize_tags(self):
        key = 'testputmaxkeysize'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        tagset = []
        for i in range(10):
            k = self._make_random_string(128)
            v = self._make_random_string(256)
            tagset.append({'Key': k, 'Value': v})

        input_tagset = {'TagSet': tagset}

        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        for kv_pair in response['TagSet']:
            self.assertEqual((kv_pair in input_tagset['TagSet']), True)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test exceed key size')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_put_excess_key_tags(self):
        key = 'testputexcesskeytags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        tagset = []
        for i in range(10):
            k = self._make_random_string(129)
            v = self._make_random_string(256)
            tagset.append({'Key': k, 'Value': v})

        input_tagset = {'TagSet': tagset}

        e = assert_raises(ClientError, client.put_object_tagging, Bucket=bucket_name, Key=key, Tagging=input_tagset)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidTag')

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(len(response['TagSet']), 0)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test exceed val size')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_put_excess_val_tags(self):
        key = 'testputexcesskeytags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        tagset = []
        for i in range(10):
            k = self._make_random_string(128)
            v = self._make_random_string(257)
            tagset.append({'Key': k, 'Value': v})

        input_tagset = {'TagSet': tagset}

        e = assert_raises(ClientError, client.put_object_tagging, Bucket=bucket_name, Key=key, Tagging=input_tagset)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidTag')

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(len(response['TagSet']), 0)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test PUT modifies existing tags')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_put_modify_tags(self):
        key = 'testputmodifytags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        tagset = []
        tagset.append({'Key': 'key', 'Value': 'val'})
        tagset.append({'Key': 'key2', 'Value': 'val2'})

        input_tagset = {'TagSet': tagset}

        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], input_tagset['TagSet'])

        tagset2 = []
        tagset2.append({'Key': 'key3', 'Value': 'val3'})

        input_tagset2 = {'TagSet': tagset2}

        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset2)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], input_tagset2['TagSet'])

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test Delete tags')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_put_delete_tags(self):
        key = 'testputmodifytags'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        input_tagset = self._create_simple_tagset(2)
        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], input_tagset['TagSet'])

        response = client.delete_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 204)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(len(response['TagSet']), 0)

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='anonymous browser based upload via POST request')
    # @attr('tagging')
    # @attr(assertion='succeeds and returns written data')
    def test_post_object_tags_anonymous_request(self):
        bucket_name = get_new_bucket_name()
        client = get_client()
        url = self._get_post_url(bucket_name)
        client.create_bucket(ACL='public-read-write', Bucket=bucket_name)

        key_name = "foo.txt"
        input_tagset = self._create_simple_tagset(2)
        # xml_input_tagset is the same as input_tagset in xml.
        # There is not a simple way to change input_tagset to xml like there is in the boto2 tetss
        xml_input_tagset = "<Tagging><TagSet><Tag><Key>0</Key><Value>0</Value></Tag><Tag><Key>1</Key><Value>1</Value" \
                           "></Tag></TagSet></Tagging> "

        payload = OrderedDict([
            ("key", key_name),
            ("acl", "public-read"),
            ("Content-Type", "text/plain"),
            ("tagging", xml_input_tagset),
            ('file', 'bar'),
        ])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key=key_name)
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

        response = client.get_object_tagging(Bucket=bucket_name, Key=key_name)
        self.assertEqual(response['TagSet'], input_tagset['TagSet'])

    # @attr(resource='object')
    # @attr(method='post')
    # @attr(operation='authenticated browser based upload via POST request')
    # @attr('tagging')
    # @attr(assertion='succeeds and returns written data')
    def test_post_object_tags_authenticated_request(self):
        bucket_name = get_new_bucket()
        client = get_client()

        url = self._get_post_url(bucket_name)
        utc = pytz.utc
        expires = datetime.datetime.now(utc) + datetime.timedelta(seconds=+6000)

        policy_document = {"expiration": expires.strftime("%Y-%m-%dT%H:%M:%SZ"), \
                           "conditions": [
                               {"bucket": bucket_name},
                               ["starts-with", "$key", "foo"],
                               {"acl": "private"},
                               ["starts-with", "$Content-Type", "text/plain"],
                               ["content-length-range", 0, 1024],
                               ["starts-with", "$tagging", ""]
                           ]}

        # xml_input_tagset is the same as `input_tagset = _create_simple_tagset(2)` in xml
        # There is not a simple way to change input_tagset to xml like there is in the boto2 tetss
        xml_input_tagset = "<Tagging><TagSet><Tag><Key>0</Key><Value>0</Value></Tag><Tag><Key>1</Key><Value>1</Value" \
                           "></Tag></TagSet></Tagging> "

        json_policy_document = json.JSONEncoder().encode(policy_document)
        policy = base64.b64encode(json_policy_document)
        aws_secret_access_key = get_main_aws_secret_key()
        aws_access_key_id = get_main_aws_access_key()

        signature = base64.b64encode(hmac.new(aws_secret_access_key, policy, sha).digest())

        payload = OrderedDict([
            ("key", "foo.txt"),
            ("AWSAccessKeyId", aws_access_key_id),
            ("acl", "private"), ("signature", signature), ("policy", policy),
            ("tagging", xml_input_tagset),
            ("Content-Type", "text/plain"),
            ('file', 'bar')])

        r = requests.post(url, files=payload)
        self.assertEqual(r.status_code, 204)
        response = client.get_object(Bucket=bucket_name, Key='foo.txt')
        body = self._get_body(response)
        self.assertEqual(body, 'bar')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test PutObj with tagging headers')
    # @attr(assertion='success')
    # @attr('tagging')
    def test_put_obj_with_tags(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'testtagobj1'
        data = 'A' * 100

        tagset = []
        tagset.append({'Key': 'bar', 'Value': ''})
        tagset.append({'Key': 'foo', 'Value': 'bar'})

        put_obj_tag_headers = {
            'x-amz-tagging': 'foo=bar&bar'
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(put_obj_tag_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)

        client.put_object(Bucket=bucket_name, Key=key, Body=data)
        response = client.get_object(Bucket=bucket_name, Key=key)
        body = self._get_body(response)
        self.assertEqual(body, data)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], tagset)

    def _make_arn_resource(self, path="*"):
        return "arn:aws:s3:::{}".format(path)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test GetObjTagging public read')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_get_tags_acl_public(self):
        key = 'testputtagsacl'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        resource = self._make_arn_resource("{}/{}".format(bucket_name, key))
        policy_document = make_json_policy("s3:GetObjectTagging",
                                           resource)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        input_tagset = self._create_simple_tagset(10)
        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        alt_client = get_alt_client()

        response = alt_client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], input_tagset['TagSet'])

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test PutObjTagging public wrote')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_put_tags_acl_public(self):
        key = 'testputtagsacl'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        resource = self._make_arn_resource("{}/{}".format(bucket_name, key))
        policy_document = make_json_policy("s3:PutObjectTagging",
                                           resource)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        input_tagset = self._create_simple_tagset(10)
        alt_client = get_alt_client()
        response = alt_client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['TagSet'], input_tagset['TagSet'])

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='test deleteobjtagging public')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_delete_tags_obj_public(self):
        key = 'testputtagsacl'
        bucket_name = self._create_key_with_random_content(key)
        client = get_client()

        resource = self._make_arn_resource("{}/{}".format(bucket_name, key))
        policy_document = make_json_policy("s3:DeleteObjectTagging",
                                           resource)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        input_tagset = self._create_simple_tagset(10)
        response = client.put_object_tagging(Bucket=bucket_name, Key=key, Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        alt_client = get_alt_client()

        response = alt_client.delete_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 204)

        response = client.get_object_tagging(Bucket=bucket_name, Key=key)
        self.assertEqual(len(response['TagSet']), 0)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='test whether a correct version-id returned')
    # @attr(assertion='version-id is same as bucket list')
    # @attr('versioning')
    def test_versioning_bucket_atomic_upload_return_version_id(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'bar'

        # for versioning-enabled-bucket, an non-empty version-id should return
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")
        response = client.put_object(Bucket=bucket_name, Key=key)
        version_id = response['VersionId']

        response = client.list_object_versions(Bucket=bucket_name)
        versions = response['Versions']
        for version in versions:
            self.assertEqual(version['VersionId'], version_id)

        # for versioning-default-bucket, no version-id should return.
        bucket_name = get_new_bucket()
        key = 'baz'
        response = client.put_object(Bucket=bucket_name, Key=key)
        self.assertEqual(('VersionId' in response), False)

        # for versioning-suspended-bucket, no version-id should return.
        bucket_name = get_new_bucket()
        key = 'baz'
        self.check_configure_versioning_retry(bucket_name, "Suspended", "Suspended")
        response = client.put_object(Bucket=bucket_name, Key=key)
        self.assertEqual(('VersionId' in response), False)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='test whether a correct version-id returned')
    # @attr(assertion='version-id is same as bucket list')
    # @attr('versioning')
    def test_versioning_bucket_multipart_upload_return_version_id(self):
        content_type = 'text/bla'
        objlen = 30 * 1024 * 1024

        bucket_name = get_new_bucket()
        client = get_client()
        key = 'bar'
        metadata = {'foo': 'baz'}

        # for versioning-enabled-bucket, an non-empty version-id should return
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen, client=client,
                                                          content_type=content_type, metadata=metadata)

        response = client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                                    MultipartUpload={'Parts': parts})
        version_id = response['VersionId']

        response = client.list_object_versions(Bucket=bucket_name)
        versions = response['Versions']
        for version in versions:
            self.assertEqual(version['VersionId'], version_id)

        # for versioning-default-bucket, no version-id should return.
        bucket_name = get_new_bucket()
        key = 'baz'

        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen, client=client,
                                                          content_type=content_type, metadata=metadata)

        response = client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                                    MultipartUpload={'Parts': parts})
        self.assertEqual(('VersionId' in response), False)

        # for versioning-suspended-bucket, no version-id should return
        bucket_name = get_new_bucket()
        key = 'foo'
        self.check_configure_versioning_retry(bucket_name, "Suspended", "Suspended")

        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen, client=client,
                                                          content_type=content_type, metadata=metadata)

        response = client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                                    MultipartUpload={'Parts': parts})
        self.assertEqual(('VersionId' in response), False)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test ExistingObjectTag conditional on get object')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_bucket_policy_get_obj_existing_tag(self):
        bucket_name = self._create_objects(keys=['publictag', 'privatetag', 'invalidtag'])
        client = get_client()

        tag_conditional = {"StringEquals": {
            "s3:ExistingObjectTag/security": "public"
        }}

        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        policy_document = make_json_policy("s3:GetObject",
                                           resource,
                                           conditions=tag_conditional)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
        tagset = []
        tagset.append({'Key': 'security', 'Value': 'public'})
        tagset.append({'Key': 'foo', 'Value': 'bar'})

        input_tagset = {'TagSet': tagset}

        response = client.put_object_tagging(Bucket=bucket_name, Key='publictag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        tagset2 = []
        tagset2.append({'Key': 'security', 'Value': 'private'})

        input_tagset = {'TagSet': tagset2}

        response = client.put_object_tagging(Bucket=bucket_name, Key='privatetag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        tagset3 = []
        tagset3.append({'Key': 'security1', 'Value': 'public'})

        input_tagset = {'TagSet': tagset3}

        response = client.put_object_tagging(Bucket=bucket_name, Key='invalidtag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        alt_client = get_alt_client()
        response = alt_client.get_object(Bucket=bucket_name, Key='publictag')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        e = assert_raises(ClientError, alt_client.get_object, Bucket=bucket_name, Key='privatetag')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

        e = assert_raises(ClientError, alt_client.get_object, Bucket=bucket_name, Key='invalidtag')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test ExistingObjectTag conditional on get object tagging')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_bucket_policy_get_obj_tagging_existing_tag(self):
        bucket_name = self._create_objects(keys=['publictag', 'privatetag', 'invalidtag'])
        client = get_client()

        tag_conditional = {"StringEquals": {
            "s3:ExistingObjectTag/security": "public"
        }}

        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        policy_document = make_json_policy("s3:GetObjectTagging",
                                           resource,
                                           conditions=tag_conditional)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
        tagset = []
        tagset.append({'Key': 'security', 'Value': 'public'})
        tagset.append({'Key': 'foo', 'Value': 'bar'})

        input_tagset = {'TagSet': tagset}

        response = client.put_object_tagging(Bucket=bucket_name, Key='publictag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        tagset2 = []
        tagset2.append({'Key': 'security', 'Value': 'private'})

        input_tagset = {'TagSet': tagset2}

        response = client.put_object_tagging(Bucket=bucket_name, Key='privatetag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        tagset3 = []
        tagset3.append({'Key': 'security1', 'Value': 'public'})

        input_tagset = {'TagSet': tagset3}

        response = client.put_object_tagging(Bucket=bucket_name, Key='invalidtag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        alt_client = get_alt_client()
        response = alt_client.get_object_tagging(Bucket=bucket_name, Key='publictag')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        # A get object itself should fail since we allowed only GetObjectTagging
        e = assert_raises(ClientError, alt_client.get_object, Bucket=bucket_name, Key='publictag')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

        e = assert_raises(ClientError, alt_client.get_object_tagging, Bucket=bucket_name, Key='privatetag')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

        e = assert_raises(ClientError, alt_client.get_object_tagging, Bucket=bucket_name, Key='invalidtag')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test ExistingObjectTag conditional on put object tagging')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_bucket_policy_put_obj_tagging_existing_tag(self):
        bucket_name = self._create_objects(keys=['publictag', 'privatetag', 'invalidtag'])
        client = get_client()

        tag_conditional = {"StringEquals": {
            "s3:ExistingObjectTag/security": "public"
        }}

        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        policy_document = make_json_policy("s3:PutObjectTagging",
                                           resource,
                                           conditions=tag_conditional)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
        tagset = []
        tagset.append({'Key': 'security', 'Value': 'public'})
        tagset.append({'Key': 'foo', 'Value': 'bar'})

        input_tagset = {'TagSet': tagset}

        response = client.put_object_tagging(Bucket=bucket_name, Key='publictag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        tagset2 = []
        tagset2.append({'Key': 'security', 'Value': 'private'})

        input_tagset = {'TagSet': tagset2}

        response = client.put_object_tagging(Bucket=bucket_name, Key='privatetag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        alt_client = get_alt_client()
        # PUT requests with object tagging are a bit wierd, if you forget to put
        # the tag which is supposed to be existing anymore well, well subsequent
        # put requests will fail

        testtagset1 = []
        testtagset1.append({'Key': 'security', 'Value': 'public'})
        testtagset1.append({'Key': 'foo', 'Value': 'bar'})

        input_tagset = {'TagSet': testtagset1}

        response = alt_client.put_object_tagging(Bucket=bucket_name, Key='publictag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        e = assert_raises(ClientError, alt_client.put_object_tagging, Bucket=bucket_name, Key='privatetag',
                          Tagging=input_tagset)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

        testtagset2 = []
        testtagset2.append({'Key': 'security', 'Value': 'private'})

        input_tagset = {'TagSet': testtagset2}

        response = alt_client.put_object_tagging(Bucket=bucket_name, Key='publictag', Tagging=input_tagset)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        # Now try putting the original tags again, this should fail
        input_tagset = {'TagSet': testtagset1}

        e = assert_raises(ClientError, alt_client.put_object_tagging, Bucket=bucket_name, Key='publictag',
                          Tagging=input_tagset)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)
