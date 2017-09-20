# -- coding: utf-8 --
"""
 @decription:
 @testlink:   测试用例 seqDB-12495 :: 版本: 1 :: 列出/取消/等待任务
 @author:     laojingtang
"""

from lib import testlib
import unittest

class TestTask12495(testlib.SdbTestBase):
   def setUp(self):
      l=self.get_data_groups()
      self.group1_name=l[0]["GroupName"]
      self.group2_name=l[1]["GroupName"]
      cl_option = {"ShardingKey": {"a": 1}, "ShardingType": "hash", "Group": self.group1_name}
      self.create_cs_cl(cl_option=cl_option)

   def tearDown(self):
      if self.should_clean_env():
         self.drop_cs()

   @unittest.skip("find bug SEQUOIADBMAINSTREAM-2804")
   def test_task(self):
      list=[{"a":i} for i in range(10000)]
      self.cl.split_async_by_percent(self.group1_name,self.group2_name,50.0)
      self.db.list_task()


