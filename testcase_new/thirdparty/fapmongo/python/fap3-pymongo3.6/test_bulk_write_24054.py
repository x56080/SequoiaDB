'''
Description   : seqDB-24054:bulk_write执行单个操作
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.21
LastEditors   : XiaoNi Huang
'''

#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
from pymongo import InsertOne, UpdateOne, UpdateMany, ReplaceOne, DeleteOne, DeleteMany
import utils
import unittest

class TestBulkWrite24054( utils.TestBase ):
   def setUp( self ):
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24054'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      self.cl.bulk_write( [InsertOne( {'init': 1} )] )
      self.cl.bulk_write( [DeleteMany({})] )
      self.assertEqual( self.cl.count( {} ), 0 )
      
   def test_bulk_write( self ):
      self.bulk_write_InsertOne()    
      self.bulk_write_UpdateOne()     
      self.bulk_write_UpdateMany()     
      self.bulk_write_ReplaceOne()     
      self.bulk_write_DeleteOne()     
      self.bulk_write_DeleteMany()             
      
   def tearDown( self ):
      self.cl.drop()

   def bulk_write_InsertOne( self ): 
      # clear docs
      self.cl.bulk_write( [DeleteMany({})] )   
      # insert_one
      self.requests = [InsertOne( {'_id': 1, 'a': 1} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 1, 0, 0 ,0, {} )
      # insert_one again
      self.requests = [InsertOne( {'_id': 2, 'a': 2} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 1, 0, 0 ,0, {} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'a': 1, '_id': 1}, {'_id': 2, 'a': 2}]
      self.checkCursor( self.cursor, self.expDocs )

   def bulk_write_UpdateOne( self ):
      # ready data
      self.cl.bulk_write( [DeleteMany({})] )
      self.cl.bulk_write( [InsertOne( {'a': 1, '_id': 1, 'b': 1} )] )
      self.cl.bulk_write( [InsertOne( {'a': 2, '_id': 2, 'b': 1} )] )
   
      # test1: UpdateOne, match docs, upsert is default
      self.requests = [UpdateOne( {}, {'$set': {'b': 2}} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 1, 1 ,0, {} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'_id': 1, 'b': 2, 'a': 1}, {'_id': 2, 'b': 1, 'a': 2}]
      self.checkCursor( self.cursor, self.expDocs )
   
      # test2: UpdateOne, not match docs of normal field, upsert = True
      self.requests = [UpdateOne( {'c': 1}, {'$set': {'_id': 3, 'a': 3}}, upsert = True )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 ,0, {0: 3} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'_id': 1, 'b': 2, 'a': 1}, {'_id': 2, 'a': 2, 'b': 1}, 
         {'_id': 3, 'c': 1, 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs )
   
      # test3: UpdateOne, not match docs of _id field, upsert = True
      self.requests = [UpdateOne( {'_id': 4}, {'$set': {'a': 4}}, upsert = True )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 ,0, {0: 4} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'b': 2, 'a': 1, '_id': 1}, {'b': 1, 'a': 2, '_id': 2}, 
   {'c': 1, 'a': 3, '_id': 3}, {'a': 4, '_id': 4}]
      self.checkCursor( self.cursor, self.expDocs )
 
   def bulk_write_UpdateMany( self ):
      # ready data
      self.cl.bulk_write( [DeleteMany({})] )
      self.cl.bulk_write( [InsertOne( {'a': 1, '_id': 1, 'b': 1} )] )
      self.cl.bulk_write( [InsertOne( {'a': 2, '_id': 2, 'b': 1} )] )

      # test1: UpdateOne, match docs, upsert is default
      self.requests = [UpdateMany( {}, {'$inc':{ 'a': 1 }, '$set': {'b': 2}} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 2, 2 ,0, {} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'a': 2, 'b': 2, '_id': 1}, {'a': 3, 'b': 2, '_id': 2}]
      self.checkCursor( self.cursor, self.expDocs )
   
      # test2: UpdateMany, not match docs of normal field, upsert = True
      self.requests = [UpdateMany( {'c': 1}, {'$set': {'_id': 3, 'a': 3}}, upsert = True )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 ,0, {0: 3} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'a': 2, 'b': 2, '_id': 1}, {'a': 3, 'b': 2, '_id': 2}, 
         {'a': 3, 'c': 1, '_id': 3}]
      self.checkCursor( self.cursor, self.expDocs )

      # test3: UpdateMany, not match docs of _id field, upsert = True
      self.requests = [UpdateMany( {'_id': 4}, {'$set': {'a': 4}}, upsert = True )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 ,0, {0: 4} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'b': 2, 'a': 2, '_id': 1}, {'b': 2, 'a': 3, '_id': 2}, 
         {'a': 3, '_id': 3, 'c': 1}, {'a': 4, '_id': 4}]
      self.checkCursor( self.cursor, self.expDocs )
   
   def bulk_write_ReplaceOne( self ):
      # ready data
      self.cl.bulk_write( [DeleteMany({})] )
      self.cl.bulk_write( [InsertOne( {'a': 1, '_id': 1, 'b': 1} )] )
      self.cl.bulk_write( [InsertOne( {'a': 2, '_id': 2, 'b': 1} )] )

      # test1: UpdateOne, match docs, upsert is default
      self.requests = [ReplaceOne( {'b': 1}, {'c': 1} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 1, 1 ,0, {} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'c': 1, '_id': 1}, {'a': 2, 'b': 1, '_id': 2}]
      self.checkCursor( self.cursor, self.expDocs )

      # test2: UpdateMany, not match docs of normal field, upsert = True
      self.requests = [ReplaceOne( {'d': 1}, {'_id':3, 'e':1}, upsert=True )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 ,0, {0: 3} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'_id': 1, 'c': 1}, {'b': 1, '_id': 2, 'a': 2}, {'e': 1, '_id': 3}]
      self.checkCursor( self.cursor, self.expDocs )

   
   def bulk_write_DeleteOne( self ):
      # ready data
      self.cl.bulk_write( [DeleteMany({})] )
      self.cl.bulk_write( [InsertOne( {'a': 1, '_id': 1, 'b': 1} )] )
      self.cl.bulk_write( [InsertOne( {'a': 2, '_id': 2, 'b': 1} )] )
      self.cl.bulk_write( [InsertOne( {'a': 3, '_id': 3, 'b': 1} )] )

      # test1: matcher = {}
      self.requests = [DeleteOne( {} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 , 1, {} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'_id': 2, 'a': 2, 'b': 1}, {'_id': 3, 'a': 3, 'b': 1}]
      self.checkCursor( self.cursor, self.expDocs )

      # test2: matcher = {...}, multi docs
      self.requests = [DeleteOne( {'b': 1} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 , 1, {} )
      # check docs
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'b': 1, 'a': 3, '_id': 3}]
      self.checkCursor( self.cursor, self.expDocs )

   
   def bulk_write_DeleteMany( self ):
      # ready data
      self.cl.bulk_write( [DeleteMany({})] )
      docsNum = 0
      while docsNum < 10:
         self.cl.bulk_write( [InsertOne( {'a': docsNum, '_id': docsNum, 'b': 1} )] )
         docsNum += 1

      # test1: matcher = {...}
      self.requests = [DeleteMany( {'a': {'$gte': 5}} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 , 5, {} )
      self.assertEqual( self.cl.count( {} ) , 5 )

      # test2: matcher = {}
      self.requests = [DeleteMany( {} )]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 0, 0, 0 , 5, {} )
      self.assertEqual( self.cl.count( {} ) , 0 )
      
   def checkResult( self, insertedCount, matchedCount, modifiedCount, deletedCount, upsertedIds ):      
      self.assertEqual( self.result.inserted_count , insertedCount )
      self.assertEqual( self.result.matched_count , matchedCount )
      self.assertEqual( self.result.modified_count , modifiedCount )
      self.assertEqual( self.result.deleted_count , deletedCount )
      self.assertEqual( self.result.upserted_ids , upsertedIds )
