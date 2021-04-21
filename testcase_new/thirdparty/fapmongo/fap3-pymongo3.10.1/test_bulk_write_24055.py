'''
Description   : seqDB-24055:bulk_write批量执行多个操作
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

class TestBulkWrite24055( utils.TestBase ):
   def setUp( self ):
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24055'
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      self.cl.bulk_write( [InsertOne( {'init': 1} )] )
      self.cl.bulk_write( [DeleteMany({})] )
      self.assertEqual( self.cl.count_documents( {} ), 0 )
      
   def test_bulk_write( self ):
      self.bulk_write_01()   
      self.bulk_write_02()  
      self.bulk_write_03()  
      self.bulk_write_04()  
      self.bulk_write_05()  
      self.bulk_write_06()              
      
   def tearDown( self ):
      self.cl.drop()

   def bulk_write_01( self ): 
      # ready data
      self.readyData()

      # test: InsertOne / UpdateOne / UpdateMany / ReplaceOne / DeleteMany / DeleteOne
      self.requests = [
         InsertOne( {'a': 11, '_id': 11, 'b': 1} ), 
         UpdateOne( {'a': {'$lt': 2}}, {'$set': {'b': 2}} ),
         UpdateMany( { "$and": [{ "a": { "$gte": 2 } }, { "a": { "$lt": 4 } }] }, {'$set': {'b': 2}} ),
         ReplaceOne( {'c': 1}, {'d': 2} ),
         DeleteOne( { "$and": [{ "a": { "$gte": 4 } }, { "a": { "$lt": 6 } }] } ),
         DeleteMany( { "$and": [{ "a": { "$gte": 6 } }, { "a": { "$lt": 8 } }] } ) ]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 1, 4, 4 , 3, {} )
      # check docs
      self.assertEqual( self.cl.count_documents( {} ), 8 )
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'d': 2, '_id': 0}, 
         {'b': 1, '_id': 1, 'c': 1, 'a': 1}, 
         {'c': 1, '_id': 2, 'b': 2, 'a': 2},
         {'c': 1, '_id': 3, 'b': 2, 'a': 3}, 
         {'b': 1, '_id': 5, 'c': 1, 'a': 5}, 
         {'b': 1, '_id': 8, 'c': 1, 'a': 8}, 
         {'b': 1, '_id': 9, 'c': 1, 'a': 9}, 
         {'_id': 11, 'b': 1, 'a': 11}] 
      self.checkCursor( self.cursor, self.expDocs ) 

   def bulk_write_02( self ): 
      # ready data
      self.readyData()

      # test: InsertOne / UpdateOne[upsert=True] / UpdateMany[upsert=True] / 
      # ReplaceOne[upsert=True] / DeleteMany / DeleteOne
      self.requests = [
         InsertOne( {'a': 11, '_id': 11, 'b': 1} ), 
         UpdateOne( {'UpdateOne': 1}, {'$set': {'_id': 12, 'UpdateOne2': 1}}, upsert = True ),
         UpdateMany( {'UpdateMany': 1}, {'$set': {'_id': 13, 'UpdateMany2': 1}}, upsert = True ),
         ReplaceOne( {'ReplaceOne': 1}, {'_id': 14, 'ReplaceOne2': 2}, upsert = True ),
         DeleteOne( { "$and": [{ "a": { "$gte": 4 } }, { "a": { "$lt": 6 } }] } ),
         DeleteMany( { "$and": [{ "a": { "$gte": 6 } }, { "a": { "$lt": 8 } }] } ) ]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 1, 0, 0 , 3, {1: 12, 2: 13, 3: 14} )
      # check docs
      self.assertEqual( self.cl.count_documents( {} ), 11 )
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [{'_id': 0, 'c': 1, 'b': 1, 'a': 0}, 
         {'_id': 1, 'c': 1, 'b': 1, 'a': 1}, 
         {'_id': 2, 'c': 1, 'b': 1, 'a': 2}, 
         {'_id': 3, 'c': 1, 'b': 1, 'a': 3}, 
         {'_id': 5, 'c': 1, 'b': 1, 'a': 5}, 
         {'_id': 8, 'c': 1, 'b': 1, 'a': 8}, 
         {'_id': 9, 'c': 1, 'b': 1, 'a': 9}, 
         {'_id': 11, 'b': 1, 'a': 11}, 
         {'_id': 12, 'UpdateOne': 1, 'UpdateOne2': 1}, 
         {'_id': 13, 'UpdateMany': 1, 'UpdateMany2': 1}, 
         {'_id': 14, 'ReplaceOne2': 2}]   
      self.checkCursor( self.cursor, self.expDocs ) 

   def bulk_write_03( self ): 
      # ready data
      self.readyData()

      # test: the same multiple operations. UpdateOne, match doc
      self.requests = [
         InsertOne( {'a': 11, '_id': 11, 'b': 1} ), 
         InsertOne( {'a': 12, '_id': 12, 'b': 1} ), 
         InsertOne( {'a': 13, '_id': 13, 'b': 1} ), 
         
         UpdateOne( {'a': 1}, {'$set': {'b': 2}} ),
         UpdateOne( {'a': 2}, {'$set': {'UpdateOne1': 1}} ),
         UpdateOne( {'UpdateOne': 1}, {'$set': {'_id': 14, 'UpdateOne2': 1}}, upsert = True ),
         
         UpdateMany( { "$and": [{ "a": { "$gte": 3 } }, { "a": { "$lt": 5 } }] }, {'$set': {'b': 3}} ),
         UpdateMany( { "$and": [{ "a": { "$gte": 5 } }, { "a": { "$lt": 7 } }] }, {'$set': {'UpdateMany1': 1}} ),
         UpdateMany( {'UpdateMany': 1}, {'$set': {'_id': 15, 'UpdateMany2': 1}}, upsert = True ),
         
         ReplaceOne( {'c': 1}, {'d': 2} ),
         ReplaceOne( {'ReplaceOne': 1}, {'_id': 16, 'ReplaceOne2': 2}, upsert = True ),
         
         DeleteOne( { 'a': 7 } ),
         DeleteMany( { "$and": [{ "a": { "$gte": 8 } }, { "a": { "$lt": 10 } }] } ) ]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 3, 7, 7 , 3, {8: 15, 10: 16, 5: 14} )
      # check docs
      self.assertEqual( self.cl.count_documents( {} ), 13 )
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [
         {'_id': 0, 'd': 2}, 
         {'a': 1, '_id': 1, 'c': 1, 'b': 2}, 
         {'a': 2, 'UpdateOne1': 1, '_id': 2, 'c': 1, 'b': 1},
         {'a': 3, '_id': 3, 'c': 1, 'b': 3}, 
         {'a': 4, '_id': 4, 'c': 1, 'b': 3}, 
         {'a': 5, 'b': 1, '_id': 5, 'c': 1, 'UpdateMany1': 1}, 
         {'a': 6, 'b': 1, '_id': 6, 'c': 1, 'UpdateMany1': 1}, 
         {'a': 11, '_id': 11, 'b': 1}, 
         {'a': 12, '_id': 12, 'b': 1}, 
         {'a': 13, '_id': 13, 'b': 1}, 
         {'UpdateOne': 1, '_id': 14, 'UpdateOne2': 1}, 
         {'UpdateMany': 1, '_id': 15, 'UpdateMany2': 1}, 
         {'_id': 16, 'ReplaceOne2': 2}]
      self.checkCursor( self.cursor, self.expDocs ) 

   def bulk_write_04( self ): 
      # ready data
      self.readyData()

      # test: the same multiple operations. UpdateOne, match doc
      self.requests = [
         InsertOne( {'a': 11, '_id': 11, 'b': 1} ), 
         InsertOne( {'a': 12, '_id': 12, 'b': 1} ), 
         InsertOne( {'a': 13, '_id': 13, 'b': 1} ), 
         
         UpdateOne( {'a': 1}, {'$set': {'b': 2}}, upsert = True ),
         UpdateOne( {'a': 2}, {'$set': {'UpdateOne1': 1}} ),
         UpdateOne( {'UpdateOne': 1}, {'$set': {'_id': 14, 'UpdateOne2': 1}}, upsert = True ),
         
         UpdateMany( { "$and": [{ "a": { "$gte": 3 } }, { "a": { "$lt": 5 } }] }, {'$set': {'b': 3}} ),
         UpdateMany( { "$and": [{ "a": { "$gte": 5 } }, { "a": { "$lt": 7 } }] }, {'$set': {'UpdateMany1': 1}} ),
         UpdateMany( {'UpdateMany': 1}, {'$set': {'_id': 15, 'UpdateMany2': 1}}, upsert = True ),
         
         ReplaceOne( {'c': 1}, {'d': 2} ),
         ReplaceOne( {'ReplaceOne': 1}, {'_id': 16, 'ReplaceOne2': 2}, upsert = True ),
         
         DeleteOne( { 'a': 7 } ),
         DeleteMany( { "$and": [{ "a": { "$gte": 8 } }, { "a": { "$lt": 10 } }] } ) ]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 3, 7, 7 , 3, {8: 15, 10: 16, 5: 14} )
      # check docs
      self.assertEqual( self.cl.count_documents( {} ), 13 )
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [
         {'_id': 0, 'd': 2}, 
         {'a': 1, '_id': 1, 'c': 1, 'b': 2}, 
         {'a': 2, 'UpdateOne1': 1, '_id': 2, 'c': 1, 'b': 1},
         {'a': 3, '_id': 3, 'c': 1, 'b': 3}, 
         {'a': 4, '_id': 4, 'c': 1, 'b': 3}, 
         {'a': 5, 'b': 1, '_id': 5, 'c': 1, 'UpdateMany1': 1}, 
         {'a': 6, 'b': 1, '_id': 6, 'c': 1, 'UpdateMany1': 1}, 
         {'a': 11, '_id': 11, 'b': 1}, 
         {'a': 12, '_id': 12, 'b': 1}, 
         {'a': 13, '_id': 13, 'b': 1}, 
         {'UpdateOne': 1, '_id': 14, 'UpdateOne2': 1}, 
         {'UpdateMany': 1, '_id': 15, 'UpdateMany2': 1}, 
         {'_id': 16, 'ReplaceOne2': 2}]
      self.checkCursor( self.cursor, self.expDocs ) 

   def bulk_write_05( self ): 
      # ready data
      self.readyData()

      # test: the same multiple operations. UpdateOne, match doc and upsert = True
      self.requests = [
         InsertOne( {'a': 11, '_id': 11, 'b': 1} ), 
         InsertOne( {'a': 12, '_id': 12, 'b': 1} ), 
         InsertOne( {'a': 13, '_id': 13, 'b': 1} ), 
         
         UpdateOne( {'a': 1}, {'$set': {'b': 2}}, upsert = True ),
         UpdateOne( {'a': 2}, {'$set': {'UpdateOne1': 1}} ),
         UpdateOne( {'UpdateOne': 1}, {'$set': {'_id': 14, 'UpdateOne2': 1}}, upsert = True ),
         
         UpdateMany( { "$and": [{ "a": { "$gte": 3 } }, { "a": { "$lt": 5 } }] }, {'$set': {'b': 3}} ),
         UpdateMany( { "$and": [{ "a": { "$gte": 5 } }, { "a": { "$lt": 7 } }] }, {'$set': {'UpdateMany1': 1}} ),
         UpdateMany( {'UpdateMany': 1}, {'$set': {'_id': 15, 'UpdateMany2': 1}}, upsert = True ),
         
         ReplaceOne( {'c': 1}, {'d': 2} ),
         ReplaceOne( {'ReplaceOne': 1}, {'_id': 16, 'ReplaceOne2': 2}, upsert = True ),
         
         DeleteOne( { 'a': 7 } ),
         DeleteMany( { "$and": [{ "a": { "$gte": 8 } }, { "a": { "$lt": 10 } }] } ) ]
      self.result = self.cl.bulk_write( self.requests )
      self.checkResult( 3, 7, 7 , 3, {8: 15, 10: 16, 5: 14} )
      # check docs
      self.assertEqual( self.cl.count_documents( {} ), 13 )
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [
         {'_id': 0, 'd': 2}, 
         {'a': 1, '_id': 1, 'c': 1, 'b': 2}, 
         {'a': 2, 'UpdateOne1': 1, '_id': 2, 'c': 1, 'b': 1},
         {'a': 3, '_id': 3, 'c': 1, 'b': 3}, 
         {'a': 4, '_id': 4, 'c': 1, 'b': 3}, 
         {'a': 5, 'b': 1, '_id': 5, 'c': 1, 'UpdateMany1': 1}, 
         {'a': 6, 'b': 1, '_id': 6, 'c': 1, 'UpdateMany1': 1}, 
         {'a': 11, '_id': 11, 'b': 1}, 
         {'a': 12, '_id': 12, 'b': 1}, 
         {'a': 13, '_id': 13, 'b': 1}, 
         {'UpdateOne': 1, '_id': 14, 'UpdateOne2': 1}, 
         {'UpdateMany': 1, '_id': 15, 'UpdateMany2': 1}, 
         {'_id': 16, 'ReplaceOne2': 2}]
      self.checkCursor( self.cursor, self.expDocs ) 

   def bulk_write_06( self ): 
      # ready data
      self.readyData()

      # test: the same multiple operations. part operation failed
      self.requests = [
         InsertOne( {'a': 11, '_id': 11, 'b': 1} ), 
         InsertOne( {'a': 12, '_id': 12, 'b': 1} ), 
         InsertOne( {'a': 13, '_id': 13, 'b': 1} ), 
         
         UpdateOne( {'a': 1}, {'$error': {'b': 2}}, upsert = True ),
         UpdateOne( {'a': 2}, {'$set': {'UpdateOne1': 1}} ),
         UpdateOne( {'UpdateOne': 1}, {'$set': {'_id': 14, 'UpdateOne2': 1}}, upsert = True ),
         
         UpdateMany( { "$and": [{ "a": { "$gte": 3 } }, { "a": { "$lt": 5 } }] }, {'$set': {'b': 3}} ),
         UpdateMany( { "$and": [{ "a": { "$gte": 5 } }, { "a": { "$lt": 7 } }] }, {'$set': {'UpdateMany1': 1}} ),
         UpdateMany( {'UpdateMany': 1}, {'$set': {'_id': 15, 'UpdateMany2': 1}}, upsert = True ),
         
         ReplaceOne( {'c': 1}, {'d': 2} ),
         ReplaceOne( {'ReplaceOne': 1}, {'_id': 16, 'ReplaceOne2': 2}, upsert = True ),
         
         DeleteOne( { 'a': 7 } ),
         DeleteMany( { "$and": [{ "a": { "$gte": 8 } }, { "a": { "$lt": 10 } }] } ) ]
      try:
         self.result = self.cl.bulk_write( self.requests )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise
      # check docs
      self.assertEqual( self.cl.count_documents( {} ), 13 )
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [
         {'b': 1, 'a': 0, '_id': 0, 'c': 1}, 
         {'b': 1, 'a': 1, '_id': 1, 'c': 1}, 
         {'b': 1, 'a': 2, '_id': 2, 'c': 1}, 
         {'b': 1, 'a': 3, '_id': 3, 'c': 1}, 
         {'b': 1, 'a': 4, '_id': 4, 'c': 1}, 
         {'b': 1, 'a': 5, '_id': 5, 'c': 1}, 
         {'b': 1, 'a': 6, '_id': 6, 'c': 1}, 
         {'b': 1, 'a': 7, '_id': 7, 'c': 1}, 
         {'b': 1, 'a': 8, '_id': 8, 'c': 1}, 
         {'b': 1, 'a': 9, '_id': 9, 'c': 1}]
      
   def readyData( self ):
      self.cl.bulk_write( [DeleteMany({})] ) 
      self.docsNum = 0
      while self.docsNum < 10:
         self.cl.bulk_write( [InsertOne( {'a': self.docsNum, '_id': self.docsNum, 'b': 1, 'c': 1} )] )
         self.docsNum += 1
      
   def checkResult( self, insertedCount, matchedCount, modifiedCount, deletedCount, upsertedIds ):      
      self.assertEqual( self.result.inserted_count , insertedCount )
      self.assertEqual( self.result.matched_count , matchedCount )
      self.assertEqual( self.result.modified_count , modifiedCount )
      self.assertEqual( self.result.deleted_count , deletedCount )
      self.assertEqual( self.result.upserted_ids , upsertedIds )
