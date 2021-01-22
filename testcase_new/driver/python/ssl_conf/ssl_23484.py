# @decription: test node connect with ssl
# @testlink:   seqDB-23484
# @interface:  node.connect()
# @author:     fanyu 2021-01-21
from lib import testlib
from lib import sdbconfig
from pysequoiadb import client
from pysequoiadb.error import SDBBaseError

username = "admin23484"
password = "admin23484"

class TestSSL23484(testlib.SdbTestBase):
    def setUp(self):
        try:
            if testlib.is_standalone():
                self.skipTest('current environment is standalone')
            self.db.create_user(username, password)
            self.config = sdbconfig.SdbConfig()
        except Exception as e:
            self.db.remove_user(username, password)
            raise e

    def test(self):
       # no use ssl
       self.verify(False)

       # use ssl
       self.verify(True)

    def tearDown(self):
       try:
          self.db.remove_user(username, password)
       except SDBBaseError as e:
          # user or password not exist
          if (-300 != e.code):
             self.fail('teardown fail: ' + e.detail)

    def verify(self, ssl):
       try:
          # create new connect
          newdb = client(self.config.host_name, self.config.service, username, password, ssl)

          # get master node
          replica_group = newdb.list_replica_groups().next()
          master = newdb.get_replica_group_by_name(replica_group["GroupName"]).get_master()

          # create node connect
          master_connect = master.connect()

          # check
          cs_name = "cs23484"
          master_connect.create_collection_space(cs_name)
          master_connect.drop_collection_space(cs_name)
       except Exception as e:
          self.db.remove_user(username, password)
          raise e

