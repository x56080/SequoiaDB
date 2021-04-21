'''
Description   : seqDB-24062:匹配符测试
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

class TestMatcher23062( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24062'
      self.docsNum = 100
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      # 准备数据   
      self.insert_docs = []
      self.i = 0
      while self.i < self.docsNum:
         self.insert_docs.append( { "_id": self.i, "a": self.i } )
         self.i += 1
      self.cl.insert_many( self.insert_docs )   
      
   def test_mathcer( self ):
      # $eq
      self.cursor = self.cl.find( { "a": { "$eq": 0 } } ).sort( 'a' )
      self.checkCursor( self.cursor, self.insert_docs[:1] )
         
      # $ne
      self.cursor = self.cl.find( { "a": { "$ne": 0 } } ).sort( 'a' )
      self.checkCursor( self.cursor, self.insert_docs[1:] )
         
      # $and / $lt / $gt
      self.cursor = self.cl.find( { "$and": [{ "a": { "$gt": 10 } }, { "a": { "$lt": 20 } }] } ).sort( 'a' )
      self.checkCursor( self.cursor, self.insert_docs[11:20] )

      # $or / $lte / $gte
      self.expDocs = []
      self.expDocs.extend( self.insert_docs[:11] )
      self.expDocs.extend( self.insert_docs[90:] )
      self.cursor = self.cl.find( { "$or": [{ "a": { "$lte": 10 } }, { "a": { "$gte": 90 } }] } ).sort( 'a' )
      self.checkCursor( self.cursor, self.expDocs )

      # exists
      self.tmpDocs = [ 
         { "_id": self.docsNum, "c": self.docsNum }, 
         { "_id": self.docsNum + 1, "c": self.docsNum + 1 } ]
      self.cl.insert_many( self.tmpDocs )
      self.cursor = self.cl.find( { "c": { "$exists": 1 } } ).sort( 'c' )
      self.checkCursor( self.cursor, self.tmpDocs )
      
      self.cl.delete_many( { "_id": { "$gte": self.docsNum } } )
      self.assertEqual( self.cl.count_documents( { "_id": { "$gte": self.docsNum } } ), 0 )

      # $mod
      self.cursor = self.cl.find( { "$and": [{ "a": { "$lt": 5 } }, { "a": { "$mod": [3, 1] } }] } ).sort( 'a' )
      self.checkCursor( self.cursor, [{'_id': 1, 'a': 1}, {'_id': 4, 'a': 4}] )

      # $regex
      self.tmpDocs = [{ "_id": self.docsNum, "c": "abc" }, { "_id": self.docsNum + 1, "c": "test" }]
      self.cl.insert_many( self.tmpDocs )
      self.cursor = self.cl.find( { "c": { "$regex": "^a", "$options": "i" } } ).sort( 'c' )
      self.checkCursor( self.cursor, [{'c': 'abc', '_id': 100}] )

      self.cl.delete_many( { "_id": { "$gte": self.docsNum } } )
      self.assertEqual( self.cl.count_documents( { "_id": { "$gte": self.docsNum } } ), 0 )

      # array
      self.tmpDocs = [{ "_id": self.docsNum, "c": [1, 2] }, { "_id": self.docsNum + 1, "c": [1, 2, 3] }]
      self.cl.insert_many( self.tmpDocs )
      # $all
      self.cursor = self.cl.find( { "c": { "$all": [2, 3] } } )
      self.checkCursor( self.cursor, [{'c': [1, 2, 3], '_id': 101}] )
      
      # $in
      self.cursor = self.cl.find( { "c": { "$in": [3, 4] } } )
      self.checkCursor( self.cursor, [{'c': [1, 2, 3], '_id': 101}] )
         
      # $nin
      self.cursor = self.cl.find( { "c": { "$nin": [3, 4] } } )
      self.checkCursor( self.cursor, [{'_id': 100, 'c': [1, 2]}] ) 
   
   def tearDown( self ):
      self.cl.drop() 