'''
Description   : seqDB-24067:去重
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.01
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestDistinct23067( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_23067'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 准备数据
      self.cl.insert_many(  [
         { "_id": 1, "a": 1, "b": 1 },
         { "_id": 4, "a": 4, "b": 1 },
         { "_id": 3, "a": 3, "b": 2 },
         { "_id": 2, "a": 2, "b": 2 },
         { "_id": 5, "a": 5, "b": 3 }] )      
      
   def test_distinct( self ):
      # 普通字段
      # distinct
      self.assertEqual( self.cl.distinct( "b" ), [1, 2, 3] )
      # distinct by match, exist repeat value
      self.assertEqual( self.cl.distinct( "b", { "b": { "$lt": 3 } } ), [1, 2] )
      # distinct by match, not exist repeat value      
      self.assertEqual( self.cl.distinct( "b", { "a": { "$gt": 3 } } ), [1, 3] )
      # distinct, not match docs
      self.assertEqual( self.cl.distinct( "b", { "b": { "$gt": 1000 } } ), [] )
      # distinct, not exist field
      self.assertEqual( self.cl.distinct( "notExistField" ), [] )
      # cl empty, distinct
      self.cl.delete_many( {} )
      self.assertEqual( self.cl.distinct( "a" ), [] )

      # object
      self.cl.insert_many( [
         { "_id": 6, "a": 6, "b": { "b1": 1, "b2": 1 } },
         { "_id": 7, "a": 7, "b": { "b1": 2, "b2": 2 } },
         { "_id": 8, "a": 8, "b": { "b1": 1, "b2": 3 } } ] )
      self.assertEqual( self.cl.distinct( "b.b1" ), [1, 2] )
   
   def tearDown( self ):
      self.cl.drop() 