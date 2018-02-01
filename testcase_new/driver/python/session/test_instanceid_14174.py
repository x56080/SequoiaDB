# @decription: instanceid[8/9/10]
# @testlink:   seqDB-14174
# @interface:  set_session_attri(options)
# @author:     liuxiaoxuan 2018-01-25

import unittest
from lib import testlib
from lib import sdbconfig
from session.commlib import *
from pysequoiadb.error import SDBBaseError

class TestSessionInstanceId14174(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      self.data_rg_name = "session14174"
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)

   def test_session_instanceid_14174(self):
      # create data rg
      data_rg = self.db.create_replica_group(self.data_rg_name)

      # create node 1 with config {instanceid : 8}
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name1 = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath1 = sdbconfig.sdb_config.rsrv_node_dir + service_name1
      config1 = {"instanceid ": 8}
      data_rg.create_node(data_hostname, service_name1, data_dbpath1, config1)

      # create node 2 with config {instanceid : 9}
      service_name2 = str(sdbconfig.sdb_config.rsrv_port_begin + 10)
      data_dbpath2 = sdbconfig.sdb_config.rsrv_node_dir + service_name2
      config2 = {"instanceid ": 9}
      data_rg.create_node(data_hostname, service_name2, data_dbpath2, config2)
      
      # create node 3 with config {instanceid : 10}
      service_name3 = str(sdbconfig.sdb_config.rsrv_port_begin + 20)
      data_dbpath3 = sdbconfig.sdb_config.rsrv_node_dir + service_name3
      config3 = {"instanceid ": 10}
      data_rg.create_node(data_hostname, service_name3, data_dbpath3, config3)

      # start group
      self.db.start_replica_group(self.data_rg_name)
      
      check_rg_master(data_rg)
      
      # create cl in new group
      self.cs = self.db.create_collection_space(self.cs_name)
      self.cl = self.cs.create_collection(self.cl_name, {'Group': self.data_rg_name, 'ReplSize' : 0})
      
      # insert datas
      insert_nums = 100
      self.insert_datas(insert_nums);
      
      # set session attr
      opt = {'PreferedInstance' : [8, 9 ,11]};
      self.db.set_session_attri(options = opt)
      
      # query and check explain
      cond = {'a' : 1}
      cursor = self.cl.explain(condition = cond)
      act_node = get_explain_nodename(cursor)
      expect_node = [{'NodeName' : data_hostname + ':' + service_name1},
                     {'NodeName' : data_hostname + ':' + service_name2}]
      self.check_random_explain(expect_node, act_node)
      
      # reset session attr
      opt = {'PreferedInstance' : [9, 10 ,11]};
      self.db.set_session_attri(options = opt)
      
      # query and check explain
      cond = {'a' : 1}
      cursor = self.cl.explain(condition = cond)
      act_node = get_explain_nodename(cursor)
      expect_node = [{'NodeName' : data_hostname + ':' + service_name2},
                     {'NodeName' : data_hostname + ':' + service_name3}]
      self.check_random_explain(expect_node, act_node)
  
      # drop cs
      testlib.drop_cs(self.db, self.cs_name)
  
      # remove group
      self.db.remove_replica_group(self.data_rg_name)
   
   def tearDown(self):
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)
      try:
         self.db.remove_replica_group(self.data_rg_name)
         self.db.disconnect()
      except SDBBaseError as e:
         if -154 != e.code:
            self.fail("tear_down_fail: " + e.detail)
   
   def insert_datas( self , insert_nums):
      flag = 0
      doc = []
      for i in range(0, insert_nums):
         doc.append({"a": i})
      try:
         self.cl.bulk_insert(flag, doc)
      except SDBBaseError as e:   
         self.fail('insert fail: ' + e.detail) 

   def check_random_explain(self, expect_node , act_node ):
      msg = "act_node: " + str(act_node) + ", expect_node: " + str(expect_node)
      for x in act_node:
         self.assertIn(x, expect_node, msg)             