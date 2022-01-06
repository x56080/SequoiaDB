'''
Description   : seqDB-24060:增删改查数据（insert_many/update_many/delete_many）
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

class TestCrudMany24060( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24060'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      
   def test_crud_many( self ):
      self.crud_insert_many()  
      self.crud_find()
      self.crud_update_many()
      self.crud_delete_many()
   
   def tearDown( self ):
      self.cl.drop() 
      
   def crud_insert_many( self ):      
      # insert_many
      self.docs = [
         { "_id": 1, "a": 1, "b": "insert_many" }, 
         { "_id": 2, "a": 2, "b": "insert_many" }, 
         { "_id": 3, "a": 3, "b": "insert_many" }, 
         { "_id": 4, "a": 4, "b": "insert_many" }]
      self.docs_ids = self.cl.insert_many( self.docs ).inserted_ids
      self.assertEqual( self.docs_ids, [1, 2, 3, 4] )

      # document为{}, 且bypass_document_validation指定为True，默认为False
      self.docs = [ {} ]
      self.docs_ids = self.cl.insert_many( self.docs ).inserted_ids
      self.assertEqual( len( self.docs_ids ), 1 )
      self.assertEqual( len( str( self.docs_ids[0] ) ), 24 )

      self.cl.delete_many( { "_id": self.docs_ids[0] } )
      self.assertEqual( self.cl.count_documents( {"_id": self.docs_ids[0]} ), 0 ) 
      
   def crud_find( self ):
      # 不指定filter
      self.cursor = self.cl.find().sort( 'a' )
      self.expectDocs = [
         { "_id": 1, "a": 1, "b": "insert_many" }, 
         { "_id": 2, "a": 2, "b": "insert_many" }, 
         { "_id": 3, "a": 3, "b": "insert_many" },
         { "_id": 4, "a": 4, "b": "insert_many" }]
      self.checkCursor( self.cursor, self.expectDocs )
      
      # 指定filter为{}
      self.cursor = self.cl.find( {} ).sort( 'a' )
      self.expectDocs = [
         { "_id": 1, "a": 1, "b": "insert_many" }, 
         { "_id": 2, "a": 2, "b": "insert_many" }, 
         { "_id": 3, "a": 3, "b": "insert_many" },
         { "_id": 4, "a": 4, "b": "insert_many" }]
      self.checkCursor( self.cursor, self.expectDocs )
      
      # 指定filter为{...}
      self.cursor = self.cl.find( {'a': {'$gt': 2}} ).sort( 'a' )
      self.expectDocs = [
         { "_id": 3, "a": 3, "b": "insert_many" },
         { "_id": 4, "a": 4, "b": "insert_many" }]
      self.checkCursor( self.cursor, self.expectDocs )
      
   def crud_update_many( self ): 
      # update_many
      self.result = self.cl.update_many( {}, {'$set': {'b': 'update_many'}} )
      self.assertEqual( self.result.matched_count, 4 )
      self.cursor = self.cl.find().sort( 'a' )
      self.expectDocs = [
         {'a': 1, '_id': 1, 'b': 'update_many'}, 
         {'a': 2, '_id': 2, 'b': 'update_many'}, 
         {'a': 3, '_id': 3, 'b': 'update_many'}, 
         {'a': 4, '_id': 4, 'b': 'update_many'}]
      self.checkCursor( self.cursor, self.expectDocs )
         
      # filter指定为{...}
      self.result = self.cl.update_many( {'a': {'$gte':2}}, {'$set': {'b': 'update_many_2'}} )
      self.assertEqual( self.result.matched_count, 3 )
      self.cursor = self.cl.find().sort( 'a' )
      self.expectDocs = [
         {'a': 1, '_id': 1, 'b': 'update_many'}, 
         {'a': 2, '_id': 2, 'b': 'update_many_2'}, 
         {'a': 3, '_id': 3, 'b': 'update_many_2'}, 
         {'a': 4, '_id': 4, 'b': 'update_many_2'}]
      self.checkCursor( self.cursor, self.expectDocs )
            
   def crud_delete_many( self ):     
      # 指定fileter为{...}
      self.result = self.cl.delete_many( { 'a': {'$gte': 3 } } )
      self.assertEqual( self.result.deleted_count, 2 )
      self.assertEqual( self.cl.count_documents( {} ), 2 )
      
      # 指定fileter为{}
      self.result = self.cl.delete_many( {} )
      self.assertEqual( self.result.deleted_count, 2 )
      self.assertEqual( self.cl.count_documents( {} ), 0 )