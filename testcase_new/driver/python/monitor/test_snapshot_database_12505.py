# @decription: test snapshot database
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
# @author:     liuxiaoxuan 2018-02-22

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
from lib import testlib

sdb_snap_database = 6
class TestSnapshotDataBase12505(testlib.SdbTestBase):
   def setUp(self):
      pass

   def test_snapshot_database_12505(self):
      expect_result = ["TotalDataRead", "TotalIndexRead"]
      # snapshot 6
      cursor = self.db.get_snapshot(sdb_snap_database)
      act_result = testlib.get_all_records(cursor)
      # check snapshot
      self.check_snapshot(expect_result, act_result)
		
   def tearDown(self):
      pass

   def check_snapshot(self, expect_result, act_result):
      is_has_totaldataread = False
      is_has_totalindexread = False
      for x in act_result:
         if expect_result[0] in x:
            is_has_totaldataread = True
         if expect_result[1] in x:
            is_has_totalindexread = True
         self.assertTrue(is_has_totaldataread)
         self.assertTrue(is_has_totalindexread)
         