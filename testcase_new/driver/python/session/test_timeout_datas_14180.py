# @decription: set time out, crud datas
# @testlink:   seqDB-14180
# @author:     liuxiaoxuan 2018-01-25

import unittest
from lib import testlib
from lib import sdbconfig
from session.commlib import *
from pysequoiadb.error import (SDBBaseError, SDBEndOfCursor)

class TestSessiontimeout14180(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)
      self.data_rg_name1 = 'session14180_1'
      self.data_rg_name2 = 'session14180_2'

   def test_session_timeout_14180(self):

      # create data rg1
      data_rg1 = self.db.create_replica_group(self.data_rg_name1)
      data_rg2 = self.db.create_replica_group(self.data_rg_name2)

      # create node in rg1
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath = sdbconfig.sdb_config.rsrv_node_dir + service_name
      data_rg1.create_node(data_hostname, service_name, data_dbpath)
		
		# create node in rg2
      service_name = str(sdbconfig.sdb_config.rsrv_port_begin + 10)
      data_dbpath = sdbconfig.sdb_config.rsrv_node_dir + service_name
      data_rg2.create_node(data_hostname, service_name, data_dbpath)
		
      # start groups
      self.db.start_replica_group(self.data_rg_name1)
      self.db.start_replica_group(self.data_rg_name2)
      
      check_rg_master(data_rg1) 
      check_rg_master(data_rg2)
	
      # create cs
      self.cs = self.db.create_collection_space(self.cs_name)
   
      # create maincl
      maincl_name = 'testmaincl14180'
      self.maincl = self.cs.create_collection(maincl_name, {'IsMainCL': True, 'ShardingKey': {'a':1}, 'ReplSize' : 0})

      # create hash cl      
      self.cl = self.cs.create_collection(self.cl_name, {'Group': self.data_rg_name1, 'ShardingKey': {'a':1}, 'ReplSize' : 0})

      # insert datas at first
      insert_nums = 30000
      self.insert_datas(insert_nums)
      
      # set session attr timeout 1ms
      newdb1 = testlib.default_db()
      newcl1 = newdb1.get_collection(self.cs_name + '.' + self.cl_name)
      
      opt = {'Timeout' : 1};
      newdb1.set_session_attri(options = opt)

      # insert datas
      insert_nums = 50000
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": i})
      try:
         newcl1.bulk_insert(0, doc)
         self.fail('Need Error -13')
      except SDBBaseError as e:   
         if -13 != e.code:
            self.fail('check create cl timeout fail: ' + e.detail)  
				
      # update datas
      try:
         rule = {'$set' : {'a' : 'newa'}}
         cond = {'a' : {'$gt' : 1}}
         newcl1.update(rule, condition = cond)
         self.fail('Need Error -13')
      except SDBBaseError as e:
         if -13 != e.code:
            self.fail('check update datas timeout fail: ' + e.detail)  
            
      # remove datas
      try:
         newcl1.delete()
         self.fail('Need Error -13')
      except SDBBaseError as e:
         if -13 != e.code:
            self.fail('check remove datas timeout fail: ' + e.detail)   

      # disconnect db
      newdb1.disconnect()            
      
      # split cl      
      try:
         self.cl.split_by_percent(self.data_rg_name1, self.data_rg_name2, 50)
      except SDBBaseError as e:
         self.fail('check split cl fail') 
                   
      # attach cl      
      try:
         opts = {"LowBound":{'a':0},"UpBound":{'a':20000}}
         self.maincl.attach_collection(self.cs_name + "." + self.cl_name, options = opts)
      except SDBBaseError as e:
         self.fail('check attach cl fail') 
           		
   def tearDown(self):
		# drop cs and datagroup
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)
      msg = "tear_down_fail: "
      self.remove_data_group(self.data_rg_name1, msg)
      self.remove_data_group(self.data_rg_name2, msg)

   def insert_datas( self, insert_nums):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": i})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBBaseError as e:   
         self.fail('insert fail: ' + e.detail) 

   def remove_data_group(self, group_name, msg):
      try:
         self.db.remove_replica_group(group_name)
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail(msg + e.detail)	