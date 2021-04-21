'''
Description   : seqDB-24066:数据统计
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.21
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestCountDocuments23066( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24066'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 准备数据
      self.cl.insert_many( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }] )
         
   def test_count_documents( self ):
      self.assertEqual( self.cl.count_documents( {} ), 3 )
      self.assertEqual( self.cl.count_documents( {'a':{'$gte': 2}} ), 2 )
      try:
         self.cl.count_documents()
      except TypeError as e:
         pass
      except:
         raise
   
   def tearDown( self ):
      self.cl.drop() 