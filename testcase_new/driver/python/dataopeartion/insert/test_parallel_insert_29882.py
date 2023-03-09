# @decription: insert record in parallel
# @testlink:   seqDB-29882
# @interface:  insert_with_flag, bulk_insert, insert
# @author:     Cheng Jingjing 2023-01-18
import os
from multiprocessing import Pool, Lock, Manager
from dataopeartion.insert.commlib import *
from lib import testlib
from pysequoiadb import client
from pysequoiadb.error import (SDBBaseError)
from pysequoiadb.collection import (INSERT_FLG_DEFAULT, INSERT_FLG_CONTONDUP, INSERT_FLG_RETURN_OID, INSERT_FLG_REPLACEONDUP, INSERT_FLG_RETURNNUM)
from bson import ObjectId

def writeDataWithBulkInsert(params):
   sdb =  client()
   # each child process inserts data separately
   _, lock = params
   lock.acquire()
   docs = getData(1000)
   flag = INSERT_FLG_RETURNNUM
   sdb.cs_29882.cl_29882.bulk_insert(flag, docs)
   # release this child process
   lock.release()

def getData(recNum):
   records = []
   for i in range(recNum):
      records.append({"a": os.getpid()})
   return records

def writeDataWithInsert(params):
   sdb = client()
   # each child process inserts data separately
   _, lock = params
   lock.acquire()
   for i in range(1000):
      sdb.cs_29882.cl_29882.insert({"a": os.getpid()})
   lock.release()

class TestParallelInsert(testlib.SdbTestBase):
   def setUp(self):
      # create cs and cl
      self.cs_name = "cs_29882"
      self.cl_name = "cl_29882"
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist=True)
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name)

   def test_parallel_insert_29882(self):
      # create 10 processes
      pnum = 10
      params = []
      lock = Manager().Lock()
      for i in range(pnum):
         params.append((i, lock))
      p = Pool(pnum)
      # test bulk insert
      self.cl.truncate()
      p.map(writeDataWithBulkInsert, params)
      actNum = self.cl.get_count()
      expNum = pnum * 1000
      self.assertEqual(actNum, expNum)
      # test insert
      self.cl.truncate()
      p.map(writeDataWithInsert, params)
      actNum = self.cl.get_count()
      expNum = pnum * 1000
      self.assertEqual(actNum, expNum)
      p.close()
      p.join()

   def tearDown(self):
      if self.should_clean_env():
         self.db.drop_collection_space(self.cs_name)
