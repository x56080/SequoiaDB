'''
Description   : seqDB-24063:选择符测试
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.13
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestSelector23063( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_23063'
      self.docsNum = 100
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 准备数据      
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": 1 },
         { "_id": 2, "a": 2, "b": [1, 2, 3, 4, 5] },
         { "_id": 3, "a": 3, "b": [{ "b1": 100 }, { "b1": 101 }] }] )
      
   def test_selector( self ):
      # $elemMatch
      # field and value exist, $elemMatch not matcher
      self.cursor = self.cl.find( { "a": 3 }, { "b": { "$elemMatch": { "b1": 100 } } } ).sort( 'a' )
      self.expDocs = [{'_id': 3, 'a': 3, 'b': [{'b1': 100}]}]
      self.checkCursor( self.cursor, self.expDocs )
         
      # field not exist
      self.cursor = self.cl.find( { "a": 3 }, { "notExist": { "$elemMatch": { "b1": 100 } } } ).sort( 'a' )
      self.expDocs = [{'b': [{'b1': 100}, {'b1': 101}], '_id': 3, 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs )
         
      # value not exist
      self.cursor = self.cl.find( { "a": 3 }, { "b": { "$elemMatch": { "b1": "notExist" } } } ).sort( 'a' )
      self.expDocs = [{'_id': 3, 'b': [], 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs )

      # $elemMatch by matcher
      self.cursor = self.cl.find( {}, { "b": { "$elemMatch": { "b1": { "$gt": 100 } } } } ).sort( 'a' )
      self.expDocs = [{'a': 1, '_id': 1}, {'a': 2, '_id': 2, 'b': []}, {'a': 3, '_id': 3, 'b': [{'b1': 101}]}]
      self.checkCursor( self.cursor, self.expDocs )  

      # $slice
      # int
      self.cursor = self.cl.find( { "a": 2 }, { "b": { "$slice": 2 } } ).sort( 'a' )
      self.expDocs = [{'a': 2, '_id': 2, 'b': [1, 2]}]
      self.checkCursor( self.cursor, self.expDocs )         
      # array
      self.cursor = self.cl.find( { "a": 2 }, { "b": { "$slice": [2, 3] } } ).sort( 'a' )
      self.expDocs = [{'a': 2, '_id': 2, 'b': [3, 4, 5]}]
      self.checkCursor( self.cursor, self.expDocs )         
      # _id: 0 | 1
      self.cursor = self.cl.find( { "a": 2 }, { "_id": 1, "b": { "$slice": [2, 3] } } ).sort( 'a' )
      self.expDocs = [{'b': [3, 4, 5], '_id': 2}]
      self.checkCursor( self.cursor, self.expDocs )            

      # { field: 0 | 1  }      
      # b:0
      self.cursor = self.cl.find( { "a": 1 }, { "b": 0 } ).sort( 'a' )
      self.expDocs = [{'_id': 1, 'a': 1}]
      self.checkCursor( self.cursor, self.expDocs )         
      # b:1
      self.cursor = self.cl.find( { "a": 2 }, { "b": { "$slice": 2 } } ).sort( 'a' )
      self.expDocs = [{'a': 2, '_id': 2, 'b': [1, 2]}]
      self.checkCursor( self.cursor, self.expDocs )

      # selector include multi field
      #  _id:0, a:1, b:1
      self.cursor = self.cl.find( { "a": 1 }, { "_id": 0, "a": 1, "b": 1 } ).sort( 'a' )
      self.expDocs = [{'b': 1, 'a': 1}]
      self.checkCursor( self.cursor, self.expDocs )  
         
      # _id:0, a:1, b:0   
      try:
         self.cl.find( { "a": 1 }, { "_id": 0, "a": 1, "b": 0 } )
      except pymongo.errors.OperationFailure as e:
         passs
      except:
         raise
         
      #  field not exist
      self.cursor = self.cl.find( { "a": 1 }, { "notExistFiled": True } ).sort( 'a' )
      self.expDocs = [{'_id': 1}]
      self.checkCursor( self.cursor, self.expDocs )
   
   def tearDown( self ):
      self.cl.drop() 