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


class TestPolicy(unittest.TestCase):

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

    def _make_arn_resource(self, path="*"):
        return "arn:aws:s3:::{}".format(path)

    def _get_body(self, response):
        body = response['Body']
        got = body.read()
        return got

    def check_access_denied(self, fn, *args, **kwargs):
        e = assert_raises(ClientError, fn, *args, **kwargs)
        status = self._get_status(e.response)
        self.assertEqual(status, 403)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test copy-source conditional on put obj')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_bucket_policy_put_obj_copy_source(self):
        bucket_name = self._create_objects(keys=['public/foo', 'public/bar', 'private/foo'])
        client = get_client()

        src_resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        policy_document = make_json_policy("s3:GetObject",
                                           src_resource)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        bucket_name2 = get_new_bucket()

        tag_conditional = {"StringLike": {
            "s3:x-amz-copy-source": bucket_name + "/public/*"
        }}

        resource = self._make_arn_resource("{}/{}".format(bucket_name2, "*"))
        policy_document = make_json_policy("s3:PutObject",
                                           resource,
                                           conditions=tag_conditional)

        client.put_bucket_policy(Bucket=bucket_name2, Policy=policy_document)

        alt_client = get_alt_client()
        copy_source = {'Bucket': bucket_name, 'Key': 'public/foo'}

        alt_client.copy_object(Bucket=bucket_name2, CopySource=copy_source, Key='new_foo')

        # This is possible because we are still the owner, see the grants with
        # policy on how to do this right
        response = alt_client.get_object(Bucket=bucket_name2, Key='new_foo')
        body = self._get_body(response)
        self.assertEqual(body, 'public/foo')

        copy_source = {'Bucket': bucket_name, 'Key': 'public/bar'}
        alt_client.copy_object(Bucket=bucket_name2, CopySource=copy_source, Key='new_foo2')

        response = alt_client.get_object(Bucket=bucket_name2, Key='new_foo2')
        body = self._get_body(response)
        self.assertEqual(body, 'public/bar')

        copy_source = {'Bucket': bucket_name, 'Key': 'private/foo'}
        self.check_access_denied(alt_client.copy_object, Bucket=bucket_name2, CopySource=copy_source, Key='new_foo2')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test copy-source conditional on put obj')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_bucket_policy_put_obj_copy_source_meta(self):
        src_bucket_name = self._create_objects(keys=['public/foo', 'public/bar'])
        client = get_client()

        src_resource = self._make_arn_resource("{}/{}".format(src_bucket_name, "*"))
        policy_document = make_json_policy("s3:GetObject",
                                           src_resource)

        client.put_bucket_policy(Bucket=src_bucket_name, Policy=policy_document)

        bucket_name = get_new_bucket()

        tag_conditional = {"StringEquals": {
            "s3:x-amz-metadata-directive": "COPY"
        }}

        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        policy_document = make_json_policy("s3:PutObject",
                                           resource,
                                           conditions=tag_conditional)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        alt_client = get_alt_client()

        lf = (lambda **kwargs: kwargs['params']['headers'].update({"x-amz-metadata-directive": "COPY"}))
        alt_client.meta.events.register('before-call.s3.CopyObject', lf)

        copy_source = {'Bucket': src_bucket_name, 'Key': 'public/foo'}
        alt_client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key='new_foo')

        # This is possible because we are still the owner, see the grants with
        # policy on how to do this right
        response = alt_client.get_object(Bucket=bucket_name, Key='new_foo')
        body = self._get_body(response)
        self.assertEqual(body, 'public/foo')

        # remove the x-amz-metadata-directive header
        def remove_header(**kwargs):
            if "x-amz-metadata-directive" in kwargs['params']['headers']:
                del kwargs['params']['headers']["x-amz-metadata-directive"]

        alt_client.meta.events.register('before-call.s3.CopyObject', remove_header)

        copy_source = {'Bucket': src_bucket_name, 'Key': 'public/bar'}
        self.check_access_denied(alt_client.copy_object, Bucket=bucket_name, CopySource=copy_source, Key='new_foo2',
                                 Metadata={"foo": "bar"})

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test put obj with canned-acl not to be public')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_bucket_policy_put_obj_acl(self):
        bucket_name = get_new_bucket()
        client = get_client()

        # An allow conditional will require atleast the presence of an x-amz-acl
        # attribute a Deny conditional would negate any requests that try to set a
        # public-read/write acl
        conditional = {"StringLike": {
            "s3:x-amz-acl": "public*"
        }}

        p = Policy()
        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        s1 = Statement("s3:PutObject", resource)
        s2 = Statement("s3:PutObject", resource, effect="Deny", condition=conditional)

        policy_document = p.add_statement(s1).add_statement(s2).to_json()
        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        alt_client = get_alt_client()
        key1 = 'private-key'

        # if we want to be really pedantic, we should check that this doesn't raise
        # and mark a failure, however if this does raise nosetests would mark this
        # as an ERROR anyway
        response = alt_client.put_object(Bucket=bucket_name, Key=key1, Body=key1)
        # response = alt_client.put_object_acl(Bucket=bucket_name, Key=key1, ACL='private')
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        key2 = 'public-key'

        lf = (lambda **kwargs: kwargs['params']['headers'].update({"x-amz-acl": "public-read"}))
        alt_client.meta.events.register('before-call.s3.PutObject', lf)

        e = assert_raises(ClientError, alt_client.put_object, Bucket=bucket_name, Key=key2, Body=key2)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 403)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Test put obj with amz-grant back to bucket-owner')
    # @attr(assertion='success')
    # @attr('bucket-policy')
    def test_bucket_policy_put_obj_grant(self):
        bucket_name = get_new_bucket()
        bucket_name2 = get_new_bucket()
        client = get_client()

        # In normal cases a key owner would be the uploader of a key in first case
        # we explicitly require that the bucket owner is granted full control over
        # the object uploaded by any user, the second bucket is where no such
        # policy is enforced meaning that the uploader still retains ownership

        main_user_id = get_main_user_id()
        alt_user_id = get_alt_user_id()

        owner_id_str = "id=" + main_user_id
        s3_conditional = {"StringEquals": {
            "s3:x-amz-grant-full-control": owner_id_str
        }}

        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        policy_document = make_json_policy("s3:PutObject",
                                           resource,
                                           conditions=s3_conditional)

        resource = self._make_arn_resource("{}/{}".format(bucket_name2, "*"))
        policy_document2 = make_json_policy("s3:PutObject", resource)

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
        client.put_bucket_policy(Bucket=bucket_name2, Policy=policy_document2)

        alt_client = get_alt_client()
        key1 = 'key1'

        lf = (lambda **kwargs: kwargs['params']['headers'].update({"x-amz-grant-full-control": owner_id_str}))
        alt_client.meta.events.register('before-call.s3.PutObject', lf)

        response = alt_client.put_object(Bucket=bucket_name, Key=key1, Body=key1)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        def remove_header(**kwargs):
            if "x-amz-grant-full-control" in kwargs['params']['headers']:
                del kwargs['params']['headers']["x-amz-grant-full-control"]

        alt_client.meta.events.register('before-call.s3.PutObject', remove_header)

        key2 = 'key2'
        response = alt_client.put_object(Bucket=bucket_name2, Key=key2, Body=key2)
        self.assertEqual(response['ResponseMetadata']['HTTPStatusCode'], 200)

        acl1_response = client.get_object_acl(Bucket=bucket_name, Key=key1)

        # user 1 is trying to get acl for the object from user2 where ownership
        # wasn't transferred
        self.check_access_denied(client.get_object_acl, Bucket=bucket_name2, Key=key2)

        acl2_response = alt_client.get_object_acl(Bucket=bucket_name2, Key=key2)

        self.assertEqual(acl1_response['Grants'][0]['Grantee']['ID'], main_user_id)
        self.assertEqual(acl2_response['Grants'][0]['Grantee']['ID'], alt_user_id)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='Deny put obj requests without encryption')
    # @attr(assertion='success')
    # @attr('encryption')
    # @attr('bucket-policy')
    # TODO: remove this 'fails_on_rgw' once I get the test passing
    # @attr('fails_on_rgw')
    def test_bucket_policy_put_obj_enc(self):
        bucket_name = get_new_bucket()
        client = get_v2_client()

        deny_incorrect_algo = {
            "StringNotEquals": {
                "s3:x-amz-server-side-encryption": "AES256"
            }
        }

        deny_unencrypted_obj = {
            "Null": {
                "s3:x-amz-server-side-encryption": "true"
            }
        }

        p = Policy()
        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))

        s1 = Statement("s3:PutObject", resource, effect="Deny", condition=deny_incorrect_algo)
        s2 = Statement("s3:PutObject", resource, effect="Deny", condition=deny_unencrypted_obj)
        policy_document = p.add_statement(s1).add_statement(s2).to_json()

        boto3.set_stream_logger(name='botocore')

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)
        key1_str = 'testobj'

        # response = client.get_bucket_policy(Bucket=bucket_name)
        # print response

        self.check_access_denied(client.put_object, Bucket=bucket_name, Key=key1_str, Body=key1_str)

        sse_client_headers = {
            'x-amz-server-side-encryption': 'AES256',
            'x-amz-server-side-encryption-customer-algorithm': 'AES256',
            'x-amz-server-side-encryption-customer-key': 'pO3upElrwuEXSoFwCfnZPdSsmt/xWeFa0N9KgDijwVs=',
            'x-amz-server-side-encryption-customer-key-md5': 'DWygnHRtgiJ77HCm+1rvHw=='
        }

        lf = (lambda **kwargs: kwargs['params']['headers'].update(sse_client_headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        # TODO: why is this a 400 and not passing, it appears boto3 is not parsing the 200 response the rgw sends back properly
        # DEBUGGING: run the boto2 and compare the requests
        # DEBUGGING: try to run this with v2 auth (figure out why get_v2_client isn't working) to make the requests similar to what boto2 is doing
        # DEBUGGING: try to add other options to put_object to see if that makes the response better
        client.put_object(Bucket=bucket_name, Key=key1_str)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='put obj with RequestObjectTag')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    # TODO: remove this fails_on_rgw when I fix it
    # @attr('fails_on_rgw')
    def test_bucket_policy_put_obj_request_obj_tag(self):
        bucket_name = get_new_bucket()
        client = get_client()

        tag_conditional = {"StringEquals": {
            "s3:RequestObjectTag/security": "public"
        }}

        p = Policy()
        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))

        s1 = Statement("s3:PutObject", resource, effect="Allow", condition=tag_conditional)
        policy_document = p.add_statement(s1).to_json()

        client.put_bucket_policy(Bucket=bucket_name, Policy=policy_document)

        alt_client = get_alt_client()
        key1_str = 'testobj'
        self.check_access_denied(alt_client.put_object, Bucket=bucket_name, Key=key1_str, Body=key1_str)

        headers = {"x-amz-tagging": "security=public"}
        lf = (lambda **kwargs: kwargs['params']['headers'].update(headers))
        client.meta.events.register('before-call.s3.PutObject', lf)
        # TODO: why is this a 400 and not passing
        alt_client.put_object(Bucket=bucket_name, Key=key1_str, Body=key1_str)

    # @attr(resource='object')
    # @attr(method='get')
    # @attr(operation='Test ExistingObjectTag conditional on get object acl')
    # @attr(assertion='success')
    # @attr('tagging')
    # @attr('bucket-policy')
    def test_bucket_policy_get_obj_acl_existing_tag(self):
        bucket_name = self._create_objects(keys=['publictag', 'privatetag', 'invalidtag'])
        client = get_client()

        tag_conditional = {"StringEquals": {
            "s3:ExistingObjectTag/security": "public"
        }}

        resource = self._make_arn_resource("{}/{}".format(bucket_name, "*"))
        policy_document = make_json_policy("s3:GetObjectAcl",
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
        response = alt_client.get_object_acl(Bucket=bucket_name, Key='publictag')
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
