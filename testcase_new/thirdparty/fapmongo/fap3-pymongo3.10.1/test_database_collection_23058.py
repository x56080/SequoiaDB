'''
Description   : seqDB-24058:创建/删除/自动创建database/collection
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

class TestDBCL23058( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName1 = 'pymongo_db_24058_1'
      self.dbName2 = 'pymongo_db_24058_2'
      self.clName1 = 'pymongo_cl_24058_1'
      self.clName2 = 'pymongo_cl_24058_2'
      # 创建client，获取cl对象
      self.db1 = self.client[ self.dbName1 ]
      self.db1_cl1 = self.db1[ self.clName1 ]
      self.db1_cl2 = self.db1[ self.clName2 ]
      
      self.db2 = self.client[ self.dbName2 ]
      self.db2_cl1 = self.db2[ self.clName1 ]
      # 清理环境
      self.client.drop_database( self.dbName1 )
      self.client.drop_database( self.dbName2 )
      
   def test_db_cl( self ):
      self.auto_create_dbcl()  
      
      self.client_list_databases()
      self.client_list_database_names()
      
      self.db_list_collections()
      self.client_list_database_names()
      
      self.client_get_default_database()
      self.client_get_database()
      
      self.db_drop_collection()
      self.cl_drop()
      
      self.client_drop_database()
   
   def tearDown( self ):
      self.client.drop_database( self.dbName1 )
      self.client.drop_database( self.dbName2 )
    
   def auto_create_dbcl( self ):
      # insert操作，自动创建 db / cl
      self.db1_cl1.insert_one( {'a':1} )
      self.assertEqual( self.db1_cl1.count_documents( {} ), 1 )
      
      self.db2_cl1.insert_one( {'a':1} )
      self.assertEqual( self.db2_cl1.count_documents( {} ), 1 )
      
      # update操作，自动创建 db / cl
      self.db1_cl2.update_one( {'a':1}, { '$set': {'_id': 1 } }, upsert = True )
      self.assertEqual( self.db1_cl2.count_documents( {} ), 1 )
    
   def client_list_databases( self ):
      # list_databases, 列取 db
      self.cursorSize = 0
      for self.db in self.client.list_databases():
         self.cursorSize += 1
      self.assertGreaterEqual( self.cursorSize, 2)
    
   def client_list_database_names( self ):
      # list_databases, 列取 db names
      self.result = self.client.list_database_names()
      self.assertIn( self.dbName1, self.result )
      self.assertIn( self.dbName2, self.result )
    
   def db_list_collections( self ):  
      # list_collections, 列取 cl
      self.cursorSize = 0
      for self.db in self.db1.list_collections():
         self.cursorSize += 1
      self.assertEqual( self.cursorSize, 2 )
    
   def db_list_collection_names( self ):
      # list_databases, 列取 db names
      self.result = self.db1.list_collection_names()
      self.assertEqual( self.result, [self.clName1, self.clName2] )
      
   def client_get_default_database( self ):
      # get_default_database, 获取默认db
      self.assertEqual( self.client.get_default_database( self.dbName1 ).name, self.dbName1 ) 
      
   def client_get_database( self ):
      # get_database 获取 db
      self.assertEqual( self.client.get_database( self.dbName1 ).name, self.dbName1 ) 
      
   def db_drop_collection( self ):
      # drop_collection，删除cl
      self.db1.drop_collection( self.clName1 )
      self.result = self.db1.list_collection_names()
      self.assertEqual( self.result, [self.clName2] )
      
   def cl_drop( self ):
      # drop 删除 cl
      self.db1_cl2.drop()
      self.result = self.db1.list_collection_names()
      self.assertEqual( self.result, [] )
      
   def client_drop_database( self ):
      self.client.drop_database( self.dbName2 )
      self.result = self.client.list_database_names()
      self.assertNotIn( self.dbName2, self.result )
      self.assertIn( self.dbName1, self.result )

