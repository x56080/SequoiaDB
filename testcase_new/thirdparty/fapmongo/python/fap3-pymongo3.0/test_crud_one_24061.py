'''
Description   : seqDB-24061:增删改查记录(insert_one / find_one / find / update_one / delete_one)
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

class TestCrudOne24061( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24061'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      
   def test_crud_one( self ):
      self.crud_insert_one()  
      self.crud_find_one()
      self.crud_update_one()
      self.crud_delete_one()
   
   def tearDown( self ):
      self.cl.drop() 
      
   def crud_insert_one( self ):      
      # document指定_id
      # insert_one
      self.doc_id = self.cl.insert_one( { "_id": 1, "a": 1, "b": "insert_one" } ).inserted_id
      self.doc_id = self.cl.insert_one( { "_id": 2, "a": 2, "b": "insert_one" } ).inserted_id
      self.assertEqual( self.doc_id, 2 )
      
      # document为{}, 且bypass_document_validation指定为True，默认为False
      self.doc_id = self.cl.insert_one( {} ).inserted_id
      self.assertEqual( len( str( self.doc_id ) ), 24 )
      self.cl.delete_one( {"_id": self.doc_id} )
      self.assertEqual( self.cl.count( {} ), 2 )

   def crud_find_one( self ):
      self.assertEqual( self.cl.find_one(), {'_id': 1, 'b': 'insert_one', 'a': 1} )
      self.assertEqual( self.cl.find_one( {} ), {'_id': 1, 'b': 'insert_one', 'a': 1} )
      self.assertEqual( self.cl.find_one( {'a': 2} ), {'_id': 2, 'b': 'insert_one', 'a': 2} )

   def crud_update_one( self ):
      # filter指定为{}
      self.result = self.cl.update_one( {}, {'$set': {'b': 'update_one'}} )
      self.assertEqual( self.result.matched_count, 1 )
      self.cursor = self.cl.find().sort( 'a' )
      self.expectDocs = [{'a': 1, '_id': 1, 'b': 'update_one'}, {'a': 2, '_id': 2, 'b': 'insert_one'}]
      self.checkCursor( self.cursor, self.expectDocs )
         
      # filter指定为{...}
      self.result = self.cl.update_one( {'a': 2}, {'$set': {'b': 'update_one_2'}} )
      self.assertEqual( self.result.matched_count, 1 )
      self.cursor = self.cl.find().sort( 'a' )
      self.expectDocs = [{'a': 1, '_id': 1, 'b': 'update_one'}, {'a': 2, '_id': 2, 'b': 'update_one_2'}]
      self.checkCursor( self.cursor, self.expectDocs )
   
   def crud_delete_one( self ):
      # 指定fileter为{}
      self.result = self.cl.delete_one({})
      self.assertEqual( self.result.deleted_count, 1 )
      self.assertEqual( self.cl.count( {} ), 1 )
      
      # 指定fileter为{...}
      self.cl.insert_one({'a':3})
      self.result = self.cl.delete_one({'a':{'$gte':0}})
      self.assertEqual( self.result.deleted_count, 1 )
      self.assertEqual( self.cl.count( {} ), 1 )
