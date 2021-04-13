'''
Description   : seqDB-24074:find_one_and_update/replace/delete操作
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

class TestFindOneAndXXX23074( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_23074'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 准备数据      
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": "insert_many" }, 
         { "_id": 2, "a": 2, "b": "insert_many" }, 
         { "_id": 3, "a": 3, "b": "insert_many" } ] )
      
   def test_find_one_and_xxx( self ):
      self.find_one_and_update()
      self.find_one_and_replace()
      self.find_one_and_delete()
   
   def tearDown( self ):
      self.cl.drop() 
      
   def find_one_and_update( self ):
      # 匹配存在的记录，更新
      self.result = self.cl.find_one_and_update( {'a': 1}, {'$inc': {'a': 1}} )
      self.assertEqual( self.result, {'a': 1, 'b': 'insert_many', '_id': 1} )
      
      # 匹配不存在的记录，更新
      self.result = self.cl.find_one_and_update( {'a': 4}, {'$set': {'b': 'find_one_and_update'}} )
      self.assertEqual( self.result, None )
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'a': 2, 'b': 'insert_many', '_id': 1}, {'a': 2, 'b': 'insert_many', '_id': 2}, 
         {'a': 3, 'b': 'insert_many', '_id': 3}]
      self.checkCursor( self.cursor, self.expDocs )
      
   def find_one_and_replace( self ):
      # 匹配存在的记录
      self.result = self.cl.find_one_and_replace({'a':2},{'b':2})
      self.expDocs = {'a': 2, 'b': 'insert_many', '_id': 1}
      self.assertEqual( self.result,self.expDocs )
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'b': 2, '_id': 1}, {'b': 'insert_many', 'a': 2, '_id': 2}, 
         {'b': 'insert_many', 'a': 3, '_id': 3}]
      self.checkCursor( self.cursor, self.expDocs )
      
   def find_one_and_delete( self ):
      # 匹配多条记录删除
      self.result = self.cl.find_one_and_delete({'a': {'$gte': 2}})
      self.assertEqual( self.result, {'_id': 2, 'b': 'insert_many', 'a': 2} )
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'_id': 1, 'b': 2}, {'_id': 3, 'b': 'insert_many', 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs ) 