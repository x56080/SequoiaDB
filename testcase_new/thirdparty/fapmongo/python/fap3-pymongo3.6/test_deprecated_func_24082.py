'''
Description   : seqDB-24082:过期的接口（常用接口基本功能测试）
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.14
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestDeprecatedFunc24082( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24082'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
         
   def test_deprecated_func( self ):
      self.func_insert()
      self.func_count()
   
   def tearDown( self ):
      self.cl.drop() 
      
   def func_insert( self ):
      self.cl.insert( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 }] )
      
   def func_count( self ):
      # test1: count
      self.assertEqual( self.cl.count(), 3 )
      self.assertEqual( self.cl.count( {} ), 3 )
      self.assertEqual( self.cl.count( {'a':{'$gte': 2}} ), 2 )   
      try:
         self.cl.count('')
      except TypeError as e:
         pass
      except:
         raise   