# @decription: test commit transation
# @testlink:   seqDB-12487
# @interface:  transaction_begin(self)
#              transaction_commit(self)
# @author:     liuxiaoxuan 2017-9-09

import unittest
import datetime
import time
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor, SDBError)
from lib import testlib

insert_nums = 100
class TestTransaction12487(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_transaction_12487(self):
      # begin to do transaction
      self.begin_transaction()

      # insert
      self.insert_datas()
      # check insert result
      expectAllResult = []
      for i in range(0, insert_nums):
         expectAllResult.append({"a": i, "b": "test" + str(i)})
      self.check_result(expectAllResult)

      # update
      rule = {'$set': {'b': 'update'}}
      condition = {'a': {'$gt': 11, '$lt': 20}}
      self.update_datas(rule,condition)

      # remove
      condition = {'a': {'$gte': 20}}
      self.remove_datas(condition)

      # commit transaction
      self.commit_transaction()
      # check commit result
      condition = {'a': {'$gt': 11}}
      expectResult = []
      for i in range(12, 20):
          expectResult.append({"a": i, "b": "update"})
      self.check_result(expectResult,condition)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)   

   def begin_transaction(self):
      try:
         self.db.transaction_begin()
      except SDBBaseError as e:
         self.fail('begin transaction fail: ' + str(e))

   def insert_datas(self):
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": i , "b": "test" + str(i)})
<<<<<<< HEAD
      try:
         flags = 0
         self.cl.bulk_insert(flags, doc)
      except SDBBaseError as e:
         self.fail('insert fail: ' + str(e))

   def update_datas(self,rule,cond):
      try:
         self.cl.update(rule, condition = cond)
      except SDBBaseError as e:
         self.fail('update fail: ' + str(e))

   def remove_datas(self,cond):
      try:
         self.cl.delete(condition = cond)
      except SDBBaseError as e:
         self.fail('remove fail: ' + str(e))
=======
      for j in range(0,10):
         try:
            flags = 0
            self.cl.bulk_insert(flags, doc)
            break
         except SDBBaseError as e:
            if(e.code != -355 or j > 8):
               self.fail('insert fail: ' + str(e))
            time.sleep(1)

   def update_datas(self,rule,cond):
      for j in range(0,10):
         try:
            self.cl.update(rule, condition = cond)
            break
         except SDBBaseError as e:
            if(e.code != -355 or j > 8):
               self.fail('update fail: ' + str(e))
            time.sleep(1)

   def remove_datas(self,cond):
      for j in range(0,10):
         try:
            self.cl.delete(condition = cond)
            break
         except SDBBaseError as e:
            if(e.code != -355 or j > 8):
               self.fail('remove fail: ' + str(e))
            time.sleep(1)
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

   def commit_transaction(self):
      try:
         self.db.transaction_commit()
      except SDBBaseError as e:
         self.fail('commit transaction fail: ' + str(e))

   def check_result(self,expectRec,cond = None):
      try:
         if cond == None:
            cursor = self.cl.query(order_by = {"_id": 1})
         else:
            cursor = self.cl.query(condition = cond, order_by = {"_id": 1})

         actRec = testlib.get_all_records_noid(cursor)
         # check result
         self.assertListEqualUnordered(expectRec, actRec)
      except SDBBaseError as e:
         self.fail('check result fail: ' + str(e))
