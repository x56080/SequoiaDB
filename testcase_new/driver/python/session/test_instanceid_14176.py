# @decription: instanceid, M/S/A
# @testlink:   seqDB-14176
# @interface:  set_session_attri(options)
#              get_session_attri()
# @author:     liuxiaoxuan 2018-01-25

import unittest
from lib import testlib
from lib import sdbconfig
from session.commlib import *
from pysequoiadb.error import SDBBaseError

class TestSessionInstanceId14176(testlib.SdbTestBase):
   def setUp(self):
      # check standalone
      if testlib.is_standalone():
         self.skipTest('run mode is standalone')
      self.data_rg_name = "session14176"
      testlib.drop_cs(self.db, self.cs_name, ignore_not_exist = True)

   def test_session_instanceid_14176(self):
      # create data rg
      data_rg = self.db.create_replica_group(self.data_rg_name)

      # create node 1 with config {instanceid : 11}
      data_hostname = self.db.get_replica_group_by_name("SYSCatalogGroup").get_master().get_hostname()
      service_name1 = str(sdbconfig.sdb_config.rsrv_port_begin)
      data_dbpath1 = sdbconfig.sdb_config.rsrv_node_dir + service_name1
      config1 = {"instanceid ": 11}
      data_rg.create_node(data_hostname, service_name1, data_dbpath1, config1)

      # create node 2 with config {instanceid : 12}
      service_name2 = str(sdbconfig.sdb_config.rsrv_port_begin + 10)
      data_dbpath2 = sdbconfig.sdb_config.rsrv_node_dir + service_name2
      config2 = {"instanceid ": 12}
      data_rg.create_node(data_hostname, service_name2, data_dbpath2, config2)
      
      # create node 3 with config {instanceid : 13}
      service_name3 = str(sdbconfig.sdb_config.rsrv_port_begin + 20)
      data_dbpath3 = sdbconfig.sdb_config.rsrv_node_dir + service_name3
      config3 = {"instanceid ": 13}
      data_rg.create_node(data_hostname, service_name3, data_dbpath3, config3)

      # start group
      self.db.start_replica_group(self.data_rg_name)
      
      check_rg_master(data_rg)
      
      # get master node
      master_node = data_rg.get_master()
      master_svcname = master_node.get_servicename()
      
      # get slave nodes
      slave_svcname1 = ''
      slave_svcname2 = ''
      if master_svcname == service_name1:
         slave_svcname1 = service_name2
         slave_svcname2 = service_name3
      elif master_svcname == service_name2:
         slave_svcname1 = service_name1
         slave_svcname2 = service_name3
      elif master_svcname == service_name3:
         slave_svcname1 = service_name1
         slave_svcname2 = service_name2

      # create maincl and subcls in new group
      self.cs = self.db.create_collection_space(self.cs_name)
      maincl_name = 'testmaincl14176';
      self.maincl = self.cs.create_collection(maincl_name , {'IsMainCL': True, 'ShardingKey': {'a' : 1 }, 'ReplSize' : 0})
      subcl_name1 = 'testsubcl14176_1';
      self.subcl1 = self.cs.create_collection(subcl_name1 , {'Group' : self.data_rg_name, 'ReplSize' : 0})
      subcl_name2 = 'testsubcl14176_2';
      comm_group = testlib.get_data_groups()
      comm_groupname = comm_group[0]['GroupName']
      print('comm_groupname: ' + comm_groupname)
      self.subcl2 = self.cs.create_collection(subcl_name2, {'Group' : comm_groupname, 'ReplSize' : 0})
      
      # attach cls
      opt1 = {"LowBound": {'a':0}, "UpBound": {'a':10000}}
      self.maincl.attach_collection(self.cs_name + '.' + subcl_name1, options = opt1)
      opt2 = {"LowBound": {'a':10000}, "UpBound": {'a':20000}}
      self.maincl.attach_collection(self.cs_name + '.' + subcl_name2, options = opt2)
      
      # insert datas
      insert_nums = 20000
      self.insert_datas(insert_nums);
      
      # set session attr 'M'
      opt = {'PreferedInstance' : 'M'};
      self.db.set_session_attri(options = opt)
      
      # query and check explain
      cond = {'a' : {'$lt' : 5000}}
      cursor = self.maincl.explain(condition = cond)
      act_node = get_explain_nodename(cursor)
      expect_node = [{'NodeName' : data_hostname + ':' + master_svcname}]           
      self.check_exact_explain(expect_node, act_node)
      
      # check session attr
      act_session_attr = self.db.get_session_attri()
      expect_session_attr = {'PreferedInstance' : 'M', 'PreferedInstanceMode' : 'random', 'Timeout' : -1}
      self.check_session_attr(expect_session_attr, act_session_attr)
      
      # set session attr 'S'
      opt = {'PreferedInstance' : 'S'};
      self.db.set_session_attri(options = opt)
      
      # query and check explain
      cond = {'a' : {'$lt' : 5000}}
      cursor = self.maincl.explain(condition = cond)
      act_node = get_explain_nodename(cursor)
      expect_node = [{'NodeName' : data_hostname + ':' + slave_svcname1},
                     {'NodeName' : data_hostname + ':' + slave_svcname2}]           
      self.check_random_explain(expect_node, act_node)
      
      # check session attr
      act_session_attr = self.db.get_session_attri()
      expect_session_attr = {'PreferedInstance' : 'S', 'PreferedInstanceMode' : 'random', 'Timeout' : -1}
      self.check_session_attr(expect_session_attr, act_session_attr)
      
      # set session attr 'A'
      opt = {'PreferedInstance' : 'A'};
      self.db.set_session_attri(options = opt)
      
      # query and check explain
      cond = {'a' : {'$lt' : 5000}}
      cursor = self.maincl.explain(condition = cond)
      act_node = get_explain_nodename(cursor)
      expect_node = [{'NodeName' : data_hostname + ':' + master_svcname},
                     {'NodeName' : data_hostname + ':' + slave_svcname1},
                     {'NodeName' : data_hostname + ':' + slave_svcname2}]           
      self.check_random_explain(expect_node, act_node)
      
      # check session attr
      act_session_attr = self.db.get_session_attri()
      expect_session_attr = {'PreferedInstance' : 'A', 'PreferedInstanceMode' : 'random', 'Timeout' : -1}
      self.check_session_attr(expect_session_attr, act_session_attr)
      
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
         self.maincl.bulk_insert(flag, doc)
      except SDBBaseError as e:   
         self.fail('insert fail: ' + e.detail) 

   def check_random_explain(self, expect_node , act_node ):
      msg = "act_node: " + str(act_node) + ", expect_node: " + str(expect_node)
      for x in act_node:
         self.assertIn(x, expect_node, msg)  

   def check_exact_explain( self, expect_node , act_node ):
      self.assertListEqualUnordered(expect_node, act_node)    

   def check_session_attr(self, expect_session_attr, act_session_attr):
      # compare act and expect results
      self.assertEqual(expect_session_attr['PreferedInstance'], act_session_attr['PreferedInstance'])
      self.assertEqual(expect_session_attr['PreferedInstanceMode'], act_session_attr['PreferedInstanceMode'])
      self.assertEqual(expect_session_attr['Timeout'], act_session_attr['Timeout'])      