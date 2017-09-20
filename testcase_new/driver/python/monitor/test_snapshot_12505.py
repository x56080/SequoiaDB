# @decription: test snapshot
# @testlink:   seqDB-12505
# @interface:  get_snapshot(self,snap_type,kwargs)
#              reset_snapshot(self,condition)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
import time
from lib import testlib

snap_type_5 = 5
class TestSnapshot12505(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_snapshot_12505(self):
      condition = [{"Name": self.cs_name} , None]

      expectResult = {"Name": self.cs_name}

      # check snapshot with option
      self.get_snapshot_5(expectResult, condition[0])
      # check snapshot without option
      self.get_snapshot_5(expectResult, condition[1])

      # check reset snapshot with condition
      self.insert_datas()
      self.reset_snapshot(condition[0])
      self.check_reset_snapshot(condition[0])

      # check reset snapshot without condition
      self.insert_datas()
      self.reset_snapshot(condition[1])
      self.check_reset_snapshot(condition[0])

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)

   def get_snapshot_5(self,expectRec,cond):
      try:
         if(cond == None):
            cursor = self.db.get_snapshot(snap_type_5)
         else:
            cursor = self.db.get_snapshot(snap_type_5, condition = cond)
         # get result
         actRec = self.get_actual_result(cursor)
         # remark totalDataSize before reset snapshot
         self.oldDataSize = actRec['TotalDataSize']
         # check result
         self.check_snapshot(expectRec,actRec)
      except SDBBaseError as e:
         self.fail('get snapshot fail: ' + e.detail)

   def get_actual_result(self,cursor):
      rec = None
      while True:
         try:
            rec = cursor.next()
            if self.cs_name == rec['Name']:
               cursor.close()
               break
         except SDBEndOfCursor:
            cursor.close()
            break
      return rec

   def check_snapshot(self,expectResult,actResult):
      self.assertEqual(expectResult['Name'], actResult['Name'], str(expectResult) + " not in " + str(actResult))

   def insert_datas(self):
      insert_nums = 100000
      flag = 0
      doc = []
      string = ''.join(['a' for i in range(1024)])
      for i in range(0, insert_nums):
         doc.append({ "a": string})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBError as e:
         self.fail('insert fail: ' + e.detail)

   def reset_snapshot(self,cond):
      try:
         if(cond == None):
            self.db.reset_snapshot()
         else:
            self.db.reset_snapshot(condition = cond)
         # sleep 3s , wait for reset
         time.sleep(3)
      except SDBBaseError as e:
         self.fail('reset snapshot fail: ' + e.detail)

   def check_reset_snapshot(self,cond):
      try:
         rec = self.db.get_snapshot(snap_type_5,condition = cond).next()

         oldDataSize = self.oldDataSize
         newDataSize = rec['TotalDataSize']
         self.oldDataSize = newDataSize
         self.assertNotEqual(oldDataSize,newDataSize,str(oldDataSize) + 'not equal to' + str(newDataSize))
      except SDBBaseError as e:
         self.fail('reset snapshot fail: ' + e.detail)
