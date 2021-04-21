'''
Description   : seqDB-24074:find_one_and_update/replace/delete操作
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

class TestFindOneAndXXX23074( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = 'pymongo_24074'
      self.clName = 'pymongo_24074'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      
   def test_find_one_and_xxx( self ):
      self.find_one_and_update()
      self.find_one_and_update_with_upsert()
      self.find_one_and_update_with_return_document()

      self.find_one_and_replace()
      self.find_one_and_delete()

      self.notExist_db_find_one_and_xxx()
      self.notExist_cl_find_one_and_xxx()
   
   def tearDown( self ):
      self.client.drop_database(self.dbName)
      
   def find_one_and_update( self ):
      self.cl.delete_many({})  
      self.cl.insert_many( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 } ] )
      
      # 匹配存在的记录，更新
      self.result = self.cl.find_one_and_update( {'a': {'$gt': 0}}, {'$inc': {'a': 10}} )
      self.assertEqual( self.result, {'a': 1, '_id': 1} )
      
      # 匹配不存在的记录，更新
      self.result = self.cl.find_one_and_update( {'a': 3}, {'$set': {'_id': 3, 'b': 3}} )
      self.assertEqual( self.result, None )
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'a': 11, '_id': 1}, {'a': 2, '_id': 2}] )

   def find_one_and_update_with_upsert( self ):
      self.cl.delete_many({})
      
      # 匹配不存在的记录
      # upsert = True
      self.result = self.cl.find_one_and_update( {'a': 1}, {'$set': {'_id': 1, 'a': 1}}, upsert = True )
      self.assertEqual( self.result, None )
      # upsert = False
      self.result = self.cl.find_one_and_update( {'a': 2}, {'$set': {'_id': 2, 'a': 2}}, upsert = False )
      self.assertEqual( self.result, None ) 
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'_id': 1, 'a': 1}] )

      # 匹配存在的记录，upsert = True
      self.result = self.cl.find_one_and_update( {'a': 1}, {'$set': {'b':1}}, upsert = True )
      self.assertEqual( self.result, None )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'_id': 1, 'a': 1, 'b': 1}] )

   def find_one_and_update_with_return_document( self ):
      # return_document，True更新或插入后的文档，False返回原始文档
      self.cl.delete_many({})
      self.cl.insert_many([{'_id':1,'a':1},{'_id':2,'a':2}])
      # update，return_document = True
      self.result = self.cl.find_one_and_update( {'a': 1}, {'$set': {'b': 1}}, return_document = True )
      self.assertEqual( self.result, {'b': 1, 'a': 1, '_id': 1} )
      # update，return_document = False
      self.result = self.cl.find_one_and_update( {'a': 2}, {'$set': {'b': 2}}, return_document = False )
      self.assertEqual( self.result, {'_id': 2, 'a': 2} )

      # upsert，return_document = True
      self.result = self.cl.find_one_and_update( {'a': 3}, {'$set': {'_id': 3, 'a': 3}}, upsert = True, return_document = True )
      self.assertEqual( self.result, {'_id': 3, 'a': 3} )
      # upsert，return_document = False
      self.result = self.cl.find_one_and_update( {'a': 4}, {'$set': {'_id': 4, 'a': 4}}, upsert = True, return_document = False )
      self.assertEqual( self.result, None )
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'_id': 1, 'a': 1, 'b': 1}, {'_id': 2, 'b': 2, 'a': 2}, {'_id': 3, 'a': 3}, {'_id': 4, 'a': 4}]
      self.checkCursor( self.cursor, self.expDocs )
      
   def find_one_and_replace( self ):
      self.cl.delete_many({})  
      self.cl.insert_one( { "_id": 1, "a": 1 } )
      
      # 匹配存在的记录，relace字段值
      self.result = self.cl.find_one_and_replace({'a':1}, {'a':2})
      self.assertEqual( self.result, {'a': 1, '_id': 1} )
      
      # 匹配存在的记录，relace新的字段和值
      self.result = self.cl.find_one_and_replace({'a':2},{'b':1})
      self.assertEqual( self.result, {'a': 2, '_id': 1} )
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'_id': 1, 'b': 1}] )
      
   def find_one_and_replace_with_upsert( self ):
      self.cl.delete_many({})  
      
      # 匹配不存在的记录，upsert = True
      ''' SEQUOIADBMAINSTREAM-7054
      # 匹配不存在的记录，upsert
      self.result = self.cl.find_one_and_replace( {'a': 3}, {'_id': 3, 'b': 3}, upsert = True, return_document = True )
      self.assertEqual( self.result, {'b': 1, 'a': 1, '_id': 1} )
      '''
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'_id': 1, 'b': 1}] )
      
   def find_one_and_delete( self ):
      self.cl.delete_many({})  
      self.cl.insert_many( [{ "_id": 1, "a": 1 }, { "_id": 2, "a": 2 }, { "_id": 3, "a": 3 } ] )

      # 匹配多条记录删除
      self.result = self.cl.find_one_and_delete({'a': {'$gte': 2}})
      self.assertEqual( self.result, {'a': 2, '_id': 2} )
      
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'a': 1, '_id': 1}, {'a': 3, '_id': 3}] ) 

   def notExist_db_find_one_and_xxx( self ):
      # db不存在，find_one_and_update
      self.client.drop_database(self.dbName)
      self.result = self.cl.find_one_and_update( {'a': 1}, {'$set': {'_id': 1}} )
      self.assertEqual( self.result, None )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [] )

      # db不存在，find_one_and_upsert
      self.client.drop_database(self.dbName)
      self.result = self.cl.find_one_and_update( {'a': 2}, {'$set': {'_id':2, 'b': 2}}, upsert = True, return_document = True )
      self.assertEqual( self.result, {'b': 2, 'a': 2, '_id': 2} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'b': 2, 'a': 2, '_id': 2}] )
      
      ''' SEQUOIADBMAINSTREAM-7054
      # db不存在，find_one_and_replace
      self.client.drop_database(self.dbName)
      self.result = self.cl.find_one_and_replace( {'a': 3}, {'_id': 3, 'b': 3}, upsert = True, return_document = True )
      self.assertEqual( self.result, {'b': 1, 'a': 1, '_id': 1} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'b': 2, 'a': 2, '_id': 2}] )
      '''
      
      # db不存在，find_one_and_delete
      self.client.drop_database(self.dbName)
      self.result = self.cl.find_one_and_delete({})
      self.assertEqual( self.result, None )

   def notExist_cl_find_one_and_xxx( self ):
      self.cl.insert_one( { "_id": 1, "a": 1 } )

      # cl不存在，find_one_and_update
      self.cl.drop()
      self.result = self.cl.find_one_and_update( {'a': 1}, {'$set': {'_id': 1}} )
      self.assertEqual( self.result, None )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [] )

      # cl不存在，find_one_and_upsert
      self.cl.drop()
      self.result = self.cl.find_one_and_update( {'a': 2}, {'$set': {'_id':2, 'b': 2}}, upsert = True, return_document = True )
      self.assertEqual( self.result, {'b': 2, 'a': 2, '_id': 2} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'b': 2, 'a': 2, '_id': 2}] )
      
      ''' SEQUOIADBMAINSTREAM-7054
      # db不存在，find_one_and_replace
      self.cl.drop()
      self.result = self.cl.find_one_and_replace( {'a': 3}, {'_id': 3, 'b': 3}, upsert = True, return_document = True )
      self.assertEqual( self.result, {'b': 1, 'a': 1, '_id': 1} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.checkCursor( self.cursor, [{'b': 2, 'a': 2, '_id': 2}] )
      '''
      
      # db不存在，find_one_and_delete
      self.cl.drop()
      self.result = self.cl.find_one_and_delete({})
      self.assertEqual( self.result, None )
      