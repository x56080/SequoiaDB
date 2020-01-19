
from botocore.exceptions import ClientError
import unittest
import time
import string
import random
import time
import datetime

from io import StringIO
from lib.testlib import S3TestBase
from .utils import assert_raises, Counter
from .utils import generate_random
from .utils import _get_status_and_error_code
from .file_utils import *

from . import (
    setup,
    teardown,
    get_client,
    get_new_bucket
)


class TestMultiPartUpload(S3TestBase):

    def generate_random(self, size, part_size=5 * 1024 * 1024):
        """
        Generate the specified number random data.
        (actually each MB is a repetition of the first KB)
        """
        chunk = 1024
        allowed = string.ascii_letters
        # range(start,stop,step)
        for x in range(0, size, part_size):
            strpart = ''.join([allowed[random.randint(0, len(allowed) - 1)] for _ in range(chunk)])
            s = ''
            left = size - x
            this_part_size = min(left, part_size)
            for y in range(this_part_size / chunk):
                s = s + strpart
            if this_part_size > len(s):
                s = s + strpart[0:this_part_size - len(s)]
            yield s
            if x == size:
                return

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

    def _create_key_with_random_content(self,keyname, size=7 * 1024 * 1024, bucket_name=None, client=None):
        if bucket_name is None:
            bucket_name = get_new_bucket()

        if client == None:
            client = get_client()
        # TODO:StringIO is not found
        data = StringIO(str(generate_random(size, size).next()))
        client.put_object(Bucket=bucket_name, Key=keyname, Body=data)

        return bucket_name

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='test copy object of a multipart upload')
    # @attr(assertion='successful')
    # @attr('versioning')
    # TODO:SEQUOIADBMAINSTREAM-5275
    def test_object_copy_versioning_multipart_upload(self):
        self.skipTest("sequoias3 暂不支持")
        bucket_name = get_new_bucket()
        client = get_client()
        self.check_configure_versioning_retry(bucket_name, "Enabled", "Enabled")

        key1 = "srcmultipart"
        key1_metadata = {'foo': 'bar'}
        content_type = 'text/bla'
        objlen = 30 * 1024 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key1, size=objlen,
                                                          content_type=content_type, metadata=key1_metadata)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key1, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.get_object(Bucket=bucket_name, Key=key1)
        key1_size = response['ContentLength']
        version_id = response['VersionId']

        # copy object in the same bucket
        copy_source = {'Bucket': bucket_name, 'Key': key1, 'VersionId': version_id}
        key2 = 'dstmultipart'
        client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key=key2)
        response = client.get_object(Bucket=bucket_name, Key=key2)
        version_id2 = response['VersionId']
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(key1_size, response['ContentLength'])
        self.assertEqual(key1_metadata, response['Metadata'])
        self.assertEqual(content_type, response['ContentType'])

        # second copy
        copy_source = {'Bucket': bucket_name, 'Key': key2, 'VersionId': version_id2}
        key3 = 'dstmultipart2'
        client.copy_object(Bucket=bucket_name, CopySource=copy_source, Key=key3)
        response = client.get_object(Bucket=bucket_name, Key=key3)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(key1_size, response['ContentLength'])
        self.assertEqual(key1_metadata, response['Metadata'])
        self.assertEqual(content_type, response['ContentType'])

        # copy to another versioned bucket
        bucket_name2 = get_new_bucket()
        self.check_configure_versioning_retry(bucket_name2, "Enabled", "Enabled")

        copy_source = {'Bucket': bucket_name, 'Key': key1, 'VersionId': version_id}
        key4 = 'dstmultipart3'
        client.copy_object(Bucket=bucket_name2, CopySource=copy_source, Key=key4)
        response = client.get_object(Bucket=bucket_name2, Key=key4)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(key1_size, response['ContentLength'])
        self.assertEqual(key1_metadata, response['Metadata'])
        self.assertEqual(content_type, response['ContentType'])

        # copy to another non versioned bucket
        bucket_name3 = get_new_bucket()
        copy_source = {'Bucket': bucket_name, 'Key': key1, 'VersionId': version_id}
        key5 = 'dstmultipart4'
        client.copy_object(Bucket=bucket_name3, CopySource=copy_source, Key=key5)
        response = client.get_object(Bucket=bucket_name3, Key=key5)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(key1_size, response['ContentLength'])
        self.assertEqual(key1_metadata, response['Metadata'])
        self.assertEqual(content_type, response['ContentType'])

        # copy from a non versioned bucket
        copy_source = {'Bucket': bucket_name3, 'Key': key5}
        key6 = 'dstmultipart5'
        client.copy_object(Bucket=bucket_name3, CopySource=copy_source, Key=key6)
        response = client.get_object(Bucket=bucket_name3, Key=key6)
        body = self._get_body(response)
        self.assertEqual(data, body)
        self.assertEqual(key1_size, response['ContentLength'])
        self.assertEqual(key1_metadata, response['Metadata'])
        self.assertEqual(content_type, response['ContentType'])

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart upload without parts')
    # TODO:SEQUOIADBMAINSTREAM-5276
    def test_multipart_upload_empty(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key1 = "mymultipart"
        objlen = 0
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key1, size=objlen)
        e = assert_raises(ClientError, client.complete_multipart_upload, Bucket=bucket_name, Key=key1,
                          UploadId=upload_id)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'MalformedXML')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart uploads with single small part')
    def test_multipart_upload_small(self):
        bucket_name = get_new_bucket()
        client = get_client()

        key1 = "mymultipart"
        objlen = 1
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key1, size=objlen)
        response = client.complete_multipart_upload(Bucket=bucket_name, Key=key1, UploadId=upload_id,
                                                    MultipartUpload={'Parts': parts})
        response = client.get_object(Bucket=bucket_name, Key=key1)
        self.assertEqual(response['ContentLength'], objlen)

    def _multipart_copy(self, src_bucket_name, src_key, dest_bucket_name, dest_key, size, client=None,
                        part_size=5 * 1024 * 1024,
                        version_id=None):
        if client is None:
            client = get_client()

        response = client.create_multipart_upload(Bucket=dest_bucket_name, Key=dest_key)
        upload_id = response['UploadId']

        if version_id is None:
            copy_source = {'Bucket': src_bucket_name, 'Key': src_key}
        else:
            copy_source = {'Bucket': src_bucket_name, 'Key': src_key, 'VersionId': version_id}

        parts = []

        i = 0
        for start_offset in range(0, size, part_size):
            end_offset = min(start_offset + part_size - 1, size - 1)
            part_num = i + 1
            copy_source_range = 'bytes={start}-{end}'.format(start=start_offset, end=end_offset)
            response = client.upload_part_copy(Bucket=dest_bucket_name, Key=dest_key, CopySource=copy_source,
                                               PartNumber=part_num, UploadId=upload_id,
                                               CopySourceRange=copy_source_range)
            parts.append({'ETag': response['CopyPartResult'][u'ETag'], 'PartNumber': part_num})
            i = i + 1

        return upload_id, parts

    def _check_key_content(self, src_key, src_bucket_name, dest_key, dest_bucket_name, version_id=None):
        client = get_client()

        if version_id is None:
            response = client.get_object(Bucket=src_bucket_name, Key=src_key)
        else:
            response = client.get_object(Bucket=src_bucket_name, Key=src_key, VersionId=version_id)
        src_size = response['ContentLength']

        response = client.get_object(Bucket=dest_bucket_name, Key=dest_key)
        dest_size = response['ContentLength']
        dest_data = self._get_body(response)
        assert (src_size >= dest_size)

        r = 'bytes={s}-{e}'.format(s=0, e=dest_size - 1)
        if version_id is None:
            response = client.get_object(Bucket=src_bucket_name, Key=src_key, Range=r)
        else:
            response = client.get_object(Bucket=src_bucket_name, Key=src_key, Range=r, VersionId=version_id)
        src_data = self._get_body(response)
        self.assertEqual(src_data, dest_data)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart copies with single small part')
    # TODO:sequoias3不支持
    def test_multipart_copy_small(self):
        self.skipTest("sequoias3不支持")
        src_key = 'foo'
        src_bucket_name = self._create_key_with_random_content(src_key)

        dest_bucket_name = get_new_bucket()
        dest_key = "mymultipart"
        size = 1
        client = get_client()

        (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.get_object(Bucket=dest_bucket_name, Key=dest_key)
        self.assertEqual(size, response['ContentLength'])
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart copies with an invalid range')
    # TODO:sequoias3不支持
    def test_multipart_copy_invalid_range(self):
        self.skipTest("sequoias3不支持")
        client = get_client()
        src_key = 'source'
        src_bucket_name = self._create_key_with_random_content(src_key, size=5)

        response = client.create_multipart_upload(Bucket=src_bucket_name, Key='dest')
        upload_id = response['UploadId']

        copy_source = {'Bucket': src_bucket_name, 'Key': src_key}
        copy_source_range = 'bytes={start}-{end}'.format(start=0, end=21)

        e = assert_raises(ClientError, client.upload_part_copy, Bucket=src_bucket_name, Key='dest', UploadId=upload_id,
                          CopySource=copy_source, CopySourceRange=copy_source_range, PartNumber=1)
        status, error_code = _get_status_and_error_code(e.response)
        valid_status = [400, 416]
        if not status in valid_status:
            raise AssertionError("Invalid response " + str(status))
        self.assertEqual(error_code, 'InvalidRange')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart copy with an improperly formatted range')
    # TODO:sequoias3不支持
    def test_multipart_copy_improper_range(self):
        self.skipTest("sequoias3不支持")
        client = get_client()
        src_key = 'source'
        src_bucket_name = self._create_key_with_random_content(src_key, size=5)

        response = client.create_multipart_upload(
            Bucket=src_bucket_name, Key='dest')
        upload_id = response['UploadId']

        copy_source = {'Bucket': src_bucket_name, 'Key': src_key}
        test_ranges = ['{start}-{end}'.format(start=0, end=2),
                       'bytes={start}'.format(start=0),
                       'bytes=hello-world',
                       'bytes=0-bar',
                       'bytes=hello-',
                       'bytes=0-2,3-5']

        for test_range in test_ranges:
            e = assert_raises(ClientError, client.upload_part_copy,
                              Bucket=src_bucket_name, Key='dest',
                              UploadId=upload_id,
                              CopySource=copy_source,
                              CopySourceRange=test_range,
                              PartNumber=1)
            status, error_code = _get_status_and_error_code(e.response)
            self.assertEqual(status, 400)
            self.assertEqual(error_code, 'InvalidArgument')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart copies without x-amz-copy-source-range')
    # TODO:sequoias3不支持
    def test_multipart_copy_without_range(self):
        self.skipTest("sequoias3不支持")
        client = get_client()
        src_key = 'source'
        src_bucket_name = self._create_key_with_random_content(src_key, size=10)
        dest_bucket_name = self.get_new_bucket_name()
        get_new_bucket(name=dest_bucket_name)
        dest_key = "mymultipartcopy"

        response = client.create_multipart_upload(Bucket=dest_bucket_name, Key=dest_key)
        upload_id = response['UploadId']
        parts = []

        copy_source = {'Bucket': src_bucket_name, 'Key': src_key}
        part_num = 1
        copy_source_range = 'bytes={start}-{end}'.format(start=0, end=9)

        response = client.upload_part_copy(Bucket=dest_bucket_name, Key=dest_key, CopySource=copy_source,
                                           PartNumber=part_num, UploadId=upload_id)

        parts.append({'ETag': response['CopyPartResult'][u'ETag'], 'PartNumber': part_num})
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.get_object(Bucket=dest_bucket_name, Key=dest_key)
        self.assertEqual(response['ContentLength'], 10)
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart copies with single small part')
    # TODO:sequoias3不支持
    def test_multipart_copy_special_names(self):
        self.skipTest("sequoias3不支持")
        src_bucket_name = get_new_bucket()

        dest_bucket_name = get_new_bucket()

        dest_key = "mymultipart"
        size = 1
        client = get_client()

        for src_key in (' ', '_', '__', '?versionId'):
            self._create_key_with_random_content(src_key, bucket_name=src_bucket_name)
            (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
            response = client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                                        MultipartUpload={'Parts': parts})
            response = client.get_object(Bucket=dest_bucket_name, Key=dest_key)
            self.assertEqual(size, response['ContentLength'])
            self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

    def _check_content_using_range(self, key, bucket_name, data, step):
        client = get_client()
        response = client.get_object(Bucket=bucket_name, Key=key)
        size = response['ContentLength']

        for ofs in range(0, size, step):
            toread = size - ofs
            if toread > step:
                toread = step
            end = ofs + toread - 1
            r = 'bytes={s}-{e}'.format(s=ofs, e=end)
            response = client.get_object(Bucket=bucket_name, Key=key, Range=r)
            self.assertEqual(response['ContentLength'], toread)
            body = self._get_body(response)
            self.assertEqual(body, data[ofs:end + 1].encode())

    def _get_body(self, response):
        body = response['Body']
        got = body.read()
        return got

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='complete multi-part upload')
    # @attr(assertion='successful')
    # @attr('fails_on_aws')
    def test_multipart_upload(self):
        bucket_name = get_new_bucket()
        key = "mymultipart"
        content_type = 'text/bla'
        objlen = 30 * 1024 * 1024
        metadata = {'foo': 'bar'}
        client = get_client()

        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen,
                                                          content_type=content_type, metadata=metadata)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})
        response = client.get_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ContentType'], content_type)
        self.assertEqual(response['Metadata'], metadata)
        body = self._get_body(response)
        self.assertEqual(len(body), response['ContentLength'])
        self.assertEqual(body, data.encode())

        self._check_content_using_range(key, bucket_name, data, 1000000)
        self._check_content_using_range(key, bucket_name, data, 10000000)

    def check_versioning(self, bucket_name, status):
        client = get_client()

        try:
            response = client.get_bucket_versioning(Bucket=bucket_name)
            self.assertEqual(response['Status'], status)
        except KeyError:
            self.assertEqual(status, None)

        # amazon is eventual consistent, retry a bit if failed

    def check_configure_versioning_retry(self, bucket_name, status, expected_string):
        client = get_client()
        client.put_bucket_versioning(Bucket=bucket_name, VersioningConfiguration={'Status': status})

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

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check multipart copies of versioned objects')
    # @attr('versioning')
    # TODO:sequoias3不支持
    def test_multipart_copy_versioned(self):
        self.skipTest("sequoias3不支持")
        src_bucket_name = get_new_bucket()
        dest_bucket_name = get_new_bucket()

        dest_key = "mymultipart"
        self.check_versioning(src_bucket_name, None)

        src_key = 'foo'
        self.check_configure_versioning_retry(src_bucket_name, "Enabled", "Enabled")

        size = 15 * 1024 * 1024
        self._create_key_with_random_content(src_key, size=size, bucket_name=src_bucket_name)
        self._create_key_with_random_content(src_key, size=size, bucket_name=src_bucket_name)
        self._create_key_with_random_content(src_key, size=size, bucket_name=src_bucket_name)

        version_id = []
        client = get_client()
        response = client.list_object_versions(Bucket=src_bucket_name)
        for ver in response['Versions']:
            version_id.append(ver['VersionId'])

        for vid in version_id:
            (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size,
                                                      version_id=vid)
            response = client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                                        MultipartUpload={'Parts': parts})
            response = client.get_object(Bucket=dest_bucket_name, Key=dest_key)
            self.assertEqual(size, response['ContentLength'])
            self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name, version_id=vid)

    def _check_upload_multipart_resend(self, bucket_name, key, objlen, resend_parts):
        content_type = 'text/bla'
        metadata = {'foo': 'bar'}
        client = get_client()

        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen,
                                                          content_type=content_type, metadata=metadata,
                                                          resend_parts=resend_parts)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.get_object(Bucket=bucket_name, Key=key)
        self.assertEqual(response['ContentType'], content_type)
        self.assertEqual(response['Metadata'], metadata)
        body = self._get_body(response)
        self.assertEqual(len(body), response['ContentLength'])
        self.assertEqual(body, data.encode())

        self._check_content_using_range(key, bucket_name, data, 1000000)
        self._check_content_using_range(key, bucket_name, data, 10000000)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='complete multiple multi-part upload with different sizes')
    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='complete multi-part upload')
    # @attr(assertion='successful')
    def test_multipart_upload_resend_part(self):
        bucket_name = get_new_bucket()
        key = "mymultipart"
        objlen = 10 * 1024 * 1024
        # _multipart_upload(self, bucket_name, key, size, part_size=5 * 1024 * 1024, client=None, content_type=None,
        #                   metadata=None,
        #                   resend_parts=[]):
        self._check_upload_multipart_resend(bucket_name, key, objlen, [0])
        self._check_upload_multipart_resend(bucket_name, key, objlen, [1])
        self._check_upload_multipart_resend(bucket_name, key, objlen, [2])
        self._check_upload_multipart_resend(bucket_name, key, objlen, [1, 2])
        self._check_upload_multipart_resend(bucket_name, key, objlen, [0, 1, 2, 3, 4, 5])

    # @attr(assertion='successful')
    def test_multipart_upload_multiple_sizes(self):
        bucket_name = get_new_bucket()
        key = "mymultipart"
        client = get_client()

        objlen = 5 * 1024 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        objlen = 5 * 1024 * 1024 + 100 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        objlen = 5 * 1024 * 1024 + 600 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        objlen = 10 * 1024 * 1024 + 100 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        objlen = 10 * 1024 * 1024 + 600 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        objlen = 10 * 1024 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen)
        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

    # @attr(assertion='successful')
    # TODO:SEQUOIADBMAINSTREAM-5276
    def test_multipart_copy_multiple_sizes(self):
        self.skipTest("sequoias3 不支持")
        src_key = 'foo'
        src_bucket_name = self._create_key_with_random_content(src_key, 12 * 1024 * 1024)

        dest_bucket_name = get_new_bucket()
        dest_key = "mymultipart"
        client = get_client()

        size = 5 * 1024 * 1024
        (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

        size = 5 * 1024 * 1024 + 100 * 1024
        (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

        size = 5 * 1024 * 1024 + 600 * 1024
        (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

        size = 10 * 1024 * 1024 + 100 * 1024
        (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

        size = 10 * 1024 * 1024 + 600 * 1024
        (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

        size = 10 * 1024 * 1024
        (upload_id, parts) = self._multipart_copy(src_bucket_name, src_key, dest_bucket_name, dest_key, size)
        client.complete_multipart_upload(Bucket=dest_bucket_name, Key=dest_key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})
        self._check_key_content(src_key, src_bucket_name, dest_key, dest_bucket_name)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='check failure on multiple multi-part upload with size too small')
    # @attr(assertion='fails 400')
    def test_multipart_upload_size_too_small(self):
        bucket_name = get_new_bucket()
        key = "mymultipart"
        client = get_client()

        size = 100 * 1024
        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=size,
                                                          part_size=10 * 1024)
        e = assert_raises(ClientError, client.complete_multipart_upload, Bucket=bucket_name, Key=key,
                          UploadId=upload_id,
                          MultipartUpload={'Parts': parts})
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'EntityTooSmall')

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
    # @attr(method='put')
    # @attr(operation='check contents of multi-part upload')
    # @attr(assertion='successful')
    def test_multipart_upload_contents(self):
        bucket_name = get_new_bucket()
        self._do_test_multipart_upload_contents(bucket_name, 'mymultipart', 3)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation=' multi-part upload overwrites existing key')
    # @attr(assertion='successful')
    def test_multipart_upload_overwrite_existing_object(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = 'mymultipart'
        payload = '12345' * 1024 * 1024
        num_parts = 2
        client.put_object(Bucket=bucket_name, Key=key, Body=payload)

        response = client.create_multipart_upload(Bucket=bucket_name, Key=key)
        upload_id = response['UploadId']

        parts = []

        for part_num in range(0, num_parts):
            response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=part_num + 1,
                                          Body=payload)
            parts.append({'ETag': response['ETag'].strip('"'), 'PartNumber': part_num + 1})

        client.complete_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        response = client.get_object(Bucket=bucket_name, Key=key)
        test_string = self._get_body(response)

        self.assertEqual(test_string, (payload * num_parts).encode())

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='abort multi-part upload')
    # @attr(assertion='successful')
    def test_abort_multipart_upload(self):
        bucket_name = get_new_bucket()
        key = "mymultipart"
        objlen = 10 * 1024 * 1024
        client = get_client()

        (upload_id, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=objlen)
        client.abort_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id)

        e = assert_raises(ClientError, client.head_object, Bucket=bucket_name, Key=key)
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='abort non-existent multi-part upload')
    # @attr(assertion='fails 404')
    def test_abort_multipart_upload_not_found(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "mymultipart"
        client.put_object(Bucket=bucket_name, Key=key)

        e = assert_raises(ClientError, client.abort_multipart_upload, Bucket=bucket_name, Key=key, UploadId='56788')
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 404)
        self.assertEqual(error_code, 'NoSuchUpload')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='concurrent multi-part uploads')
    # @attr(assertion='successful')
    def test_list_multipart_upload(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "mymultipart"
        mb = 1024 * 1024

        upload_ids = []
        (upload_id1, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=5 * mb)
        upload_ids.append(upload_id1)
        (upload_id2, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key, size=6 * mb)
        upload_ids.append(upload_id2)

        key2 = "mymultipart2"
        (upload_id3, data, parts) = self._multipart_upload(bucket_name=bucket_name, key=key2, size=5 * mb)
        upload_ids.append(upload_id3)

        response = client.list_multipart_uploads(Bucket=bucket_name)
        uploads = response['Uploads']
        resp_uploadids = []

        for i in range(0, len(uploads)):
            resp_uploadids.append(uploads[i]['UploadId'])

        for i in range(0, len(upload_ids)):
            self.assertEqual(True, (upload_ids[i] in resp_uploadids))

        client.abort_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id1)
        client.abort_multipart_upload(Bucket=bucket_name, Key=key, UploadId=upload_id2)
        client.abort_multipart_upload(Bucket=bucket_name, Key=key2, UploadId=upload_id3)

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='multi-part upload with missing part')
    def test_multipart_upload_missing_part(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "mymultipart"
        size = 1

        response = client.create_multipart_upload(Bucket=bucket_name, Key=key)
        upload_id = response['UploadId']

        parts = []
        response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=1,
                                      Body='test')
        # 'PartNumber should be 1'
        parts.append({'ETag': response['ETag'].strip('"'), 'PartNumber': 9999})

        e = assert_raises(ClientError, client.complete_multipart_upload, Bucket=bucket_name, Key=key,
                          UploadId=upload_id,
                          MultipartUpload={'Parts': parts})
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidPart')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='multi-part upload with incorrect ETag')
    def test_multipart_upload_incorrect_etag(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key = "mymultipart"
        size = 1

        response = client.create_multipart_upload(Bucket=bucket_name, Key=key)
        upload_id = response['UploadId']

        parts = []
        response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key, PartNumber=1,
                                      Body='test')
        # 'ETag' should be "93b885adfe0da089cdf634904fd59f71"
        parts.append({'ETag': "ffffffffffffffffffffffffffffffff", 'PartNumber': 1})

        e = assert_raises(ClientError, client.complete_multipart_upload, Bucket=bucket_name, Key=key,
                          UploadId=upload_id,
                          MultipartUpload={'Parts': parts})
        status, error_code = _get_status_and_error_code(e.response)
        self.assertEqual(status, 400)
        self.assertEqual(error_code, 'InvalidPart')

    # @attr(resource='object')
    # @attr(method='put')
    # @attr(operation='multipart check for two writes of the same part, first write finishes last')
    # @attr(assertion='object contains correct content')
    def test_multipart_resend_first_finishes_last(self):
        bucket_name = get_new_bucket()
        client = get_client()
        key_name = "mymultipart"

        response = client.create_multipart_upload(Bucket=bucket_name, Key=key_name)
        upload_id = response['UploadId']

        # file_size = 8*1024*1024
        file_size = 8

        counter = Counter(0)
        # upload_part might read multiple times from the object
        # first time when it calculates md5, second time when it writes data
        # out. We want to interject only on the last time, but we can't be
        # sure how many times it's going to read, so let's have a test run
        # and count the number of reads

        fp_dry_run = FakeWriteFile(file_size, 'C',
                                   lambda: counter.inc()
                                   )

        parts = []

        response = client.upload_part(UploadId=upload_id, Bucket=bucket_name, Key=key_name, PartNumber=1,
                                      Body=fp_dry_run)

        parts.append({'ETag': response['ETag'].strip('"'), 'PartNumber': 1})
        client.complete_multipart_upload(Bucket=bucket_name, Key=key_name, UploadId=upload_id,
                                         MultipartUpload={'Parts': parts})

        client.delete_object(Bucket=bucket_name, Key=key_name)

        # clear parts
        parts[:] = []

        # ok, now for the actual test
        fp_b = FakeWriteFile(file_size, 'B')

    def setUp(self):
        # print(self.__module__ + " setup: " + str(datetime.now()))
        setup()

    def tearDown(self):
        # print(self.__module__ + " tearDown: " + str(datetime.now()))
        teardown()


if __name__ == '__main__':
    unittest.main()
