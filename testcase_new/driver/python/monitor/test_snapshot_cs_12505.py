# @decription: test snapshot collectionSapces
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
#              reset_snapshot(self,condition)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import SDBBaseError
import time
from lib import testlib

sdb_snap_collection_sapces = 5
class TestSnapshotCSCL12505(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_snapshot_cscl_12505(self):
      condition = [{"Name": self.cs_name}, None]
      expect_result = {"Name": self.cs_name}

      # get snapshot with option
      act_result = self.get_snapshot_collectionspaces(condition[0])
      
      # check snapshot
      self.check_snapshot(expect_result, act_result)
      # get snapshot without option
      act_result = self.get_snapshot_collectionspaces(condition[1])
      # check snapshot
      self.check_snapshot(expect_result, act_result)

      # check reset snapshot with condition
      self.insert_datas()
      self.reset_snapshot(condition[0])
      self.check_reset_snapshot(condition[0])

      # check reset snapshot without condition
      self.insert_datas()
      self.reset_snapshot(condition[1])
      self.check_reset_snapshot(condition[1])
		
   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def get_snapshot_collectionspaces(self, cond):
      rec = {}
      try:
         if cond == None:
            cursor = self.db.get_snapshot(sdb_snap_collection_sapces)
         else:
            cursor = self.db.get_snapshot(sdb_snap_collection_sapces, condition = cond)
         # get result
         rec = self.get_actual_result(cursor)
         # remark totalDataSize before reset snapshot
         self.oldDataSize = rec['TotalDataSize']
      except SDBBaseError as e:
         self.fail('get snapshot fail: ' + e.detail)
      return rec
            
   def get_actual_result(self,cursor):
      rec = {}
      while True:
         try:
            rec = cursor.next()
            if self.cs_name == rec['Name']:
               rec = {'Name': rec['Name'], 'TotalDataSize': rec['TotalDataSize']}
               cursor.close()
               break
         except SDBEndOfCursor:
            cursor.close()
            break
      return rec

   def check_snapshot(self, expect_result, act_result):
      self.assertEqual(expect_result['Name'], act_result['Name'], str(expect_result) + " not equal " + str(act_result))

   def insert_datas(self):
      insert_nums = 100000
      doc = []
      string = ''.join(['a' for i in range(1024)])
      for i in range(0, insert_nums):
         doc.append({ "a": string})
      try:
         self.cl.bulk_insert(0, doc)
      except SDBError as e:
         self.fail('insert fail: ' + e.detail)

   def reset_snapshot(self,cond):
      try:
         if(cond == None):
            self.db.reset_snapshot()
         else:
            self.db.reset_snapshot(cond)
         # sleep 3s , wait for reset
         time.sleep(3)
      except SDBBaseError as e:
         self.fail('reset snapshot fail: ' + e.detail)

   def check_reset_snapshot(self, cond = None):
      try:
         if cond == None:
            rec = self.db.get_snapshot(sdb_snap_collection_sapces).next()
         else:
            rec = self.db.get_snapshot(sdb_snap_collection_sapces, condition = cond).next()
            
         oldDataSize = self.oldDataSize
         newDataSize = rec['TotalDataSize']
         self.oldDataSize = newDataSize
         self.assertNotEqual(oldDataSize, newDataSize, str(oldDataSize) + 'equal to' + str(newDataSize))
      except SDBBaseError as e:
         self.fail('reset snapshot fail: ' + e.detail)
