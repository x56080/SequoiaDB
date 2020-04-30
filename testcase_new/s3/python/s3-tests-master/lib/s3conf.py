import json
import os
import os.path


class S3Config(object):

    def __init__(self):
        config_file = os.path.join(os.getcwd(), "s3tests.json")
        fp = open(config_file)
        configs = json.load(fp)
        self.host = configs['host']
        self.port = configs['port']
        self.is_secure = configs['is_secure']
        self.bucket_prefix = configs['bucket_prefix']
        self.display_name = configs['display_name']
        self.user_id = configs['user_id']
        self.email = configs['email']
        self.api_name = configs['api_name']
        self.access_key = configs['access_key']
        self.secret_key = configs['secret_key']
        self.kms_keyid = configs['kms_keyid']
        self.alt_display_name = configs['alt_display_name']
        self.alt_email = configs['alt_email']
        self.alt_user_id = configs['alt_user_id']
        self.alt_access_key = configs['alt_access_key']
        self.alt_secret_key = configs['alt_secret_key']
        self.tenant_display_name = configs['tenant_display_name']
        self.tenant_user_id = configs['tenant_user_id']
        self.tenant_access_key = configs['tenant_access_key']
        self.tenant_secret_key = configs['tenant_secret_key']
        self.tenant_email = configs['tenant_email']
        self.remote_host = configs['remote_host']
        self.remote_user = configs['remote_user']
        self.remote_password = configs['remote_password']
        fp.close()


if __name__ == '__main__':
    s3_config = S3Config()
    print('conf = ', s3_config)
