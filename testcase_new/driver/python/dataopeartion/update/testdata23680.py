# -*- coding: utf-8 -*-
# @decription: seqDB-23680:updateOne/deleteOne/upsertOne接口验证
# @author:     liuli
# @createTime: 2021.03.17

from lib import testlib
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor,SDBTypeError)

class TestUpdateOneAndDeleteOne23680(testlib.SdbTestBase):
   def setUp(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_DeleteOne_23680(self):
      records = [{"a": 1}, {"a": 1}, {"a": 1}, {"a": 1}]
      self.cl.bulk_insert(0, records)

      # 匹配多条数据，删除一条
      DELETE_FLG_DELETE_ONE=2
      self.cl.delete(condition = {"a": 1}, flags = DELETE_FLG_DELETE_ONE)
      try:
         cursor = self.cl.query()
         actRec = []
         expectRec = [{"a": 1}, {"a": 1}, {"a": 1}]
         while True:
            try:
               rec = cursor.next()
               del rec["_id"]
               actRec.append(rec)
            except SDBEndOfCursor:
               break
         self.assertListEqualUnordered(expectRec, actRec)
      except SDBBaseError as e:
         self.fail("delete one error: " + e.detail)

      # 匹配多条记录，更新一条
      update = {'$inc': {"a": 5}}
      UPDATE_FLG_UPDATE_ONE=2
      self.cl.update(update, flags=UPDATE_FLG_UPDATE_ONE)
      try:
         cursor = self.cl.query(order_by = {"a": 1})
         actRec = []
         expectRec = [{"a": 1}, {"a": 1}, {"a": 6}]
         while True:
            try:
               rec = cursor.next()
               del rec["_id"]
               actRec.append(rec)
            except SDBEndOfCursor:
               break
         self.assertListEqualUnordered(expectRec, actRec)
      except SDBBaseError as e:
         self.fail("update one error: " + e.detail)

      # 匹配多条记录，upsert 一条记录
      upsert = {'$inc': {"a": 5}}
      cond = {"a": {"$et": 1}}
      self.cl.upsert(upsert,condition=cond, flags=UPDATE_FLG_UPDATE_ONE)
      try:
         sort = {"a": 1}
         cursor = self.cl.query(order_by = sort)
         actRec = []
         expectRec = [{"a": 6}, {"a": 6}, {"a": 1}]
         while True:
            try:
               rec = cursor.next()
               del rec["_id"]
               actRec.append(rec)
            except SDBEndOfCursor:
               break
         self.assertListEqualUnordered(expectRec, actRec)
      except SDBBaseError as e:
         self.fail("upsert one error: " + e.detail)

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
