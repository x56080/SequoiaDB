# @decription: test list
# @testlink:   seqDB-12504
# @interface:  get_list(self,list_type,kwargs)
# @author:     liuxiaoxuan 2017-9-08

import unittest
import datetime
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from lib import testlib

insert_nums = 100
class TestList12504(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)
      self.insert_datas()

   def test_list_12504(self):

      cl_full_name = self.cl_name_qualified
      condition = [{"Name": cl_full_name} , None]

      # check list with option
      expectResult = [cl_full_name]
      self.get_list_4(expectResult, condition[0])

      # check list without option
      expectResult2 = [cl_full_name]
      self.get_list_4(expectResult, condition[1])

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
			
   def insert_datas(self):
      flag = 0
      doc = []
      for i in range(0,insert_nums):
         doc.append({"a":"test" + str(i)})
      try:
         self.cl.bulk_insert(flag,doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + e.detail) 

   def get_list_4(self,expectResult,cond):
       try:
          list_type_4 = 4
          if(cond == None):
              cursor = self.db.get_list(list_type_4)
          else:
             cursor = self.db.get_list(list_type_4, condition = cond)

          actResult = self.get_act_result(cursor)
          self.check_list_result(expectResult,actResult)
       except SDBBaseError as e:
          self.fail('get list fail: ' + e.detail)

   def get_act_result(self,cursor):
      actResult = []
      while True:
         try:
            rec = cursor.next()
            actResult.append(rec['Name'])
         except SDBEndOfCursor:
            cursor.close()
            break
      return actResult

   def check_list_result(self,expect,act):
      for x in expect:
         self.assertIn(x, act, x + " not in " + str(act))