'''
Description   : seqDB-24064:更新符测试
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.25
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import bson
import pymongo
from pymongo import MongoClient
import utils
import unittest

class TestUpdater24064( utils.TestBase ):   
   def setUp( self ):  
      # 初始化
      self.dbName = self.COMMDBNAME
      self.clName = 'pymongo_24064'
      self.docsNum = 100
      # 创建client，获取cl对象
      self.db = self.client[ self.dbName ]
      self.cl = self.db[ self.clName ]
      # 清理环境
      self.cl.drop()
      
   def test_mathcer( self ):
      self.updater_inc()
      self.updater_set()
      self.updater_setOnInsert()
      self.updater_set_inc_setOnInsert()
      self.updater_unset()
      self.updater_pop()
      self.updater_pull()
      self.updater_push()
      self.updater_others()
   
   def tearDown( self ):
      self.cl.drop() 
      
   def updater_inc( self ):
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": 1 }, 
         { "_id": 2, "a": 2, "b": 1 }, 
         { "_id": 3, "a": 3, "b": 1 }] )
         
      self.result = self.cl.update_many( { "a": { "$in": [1, 2] } }, { "$inc": { "b": 1, "c": 1 } } )
      self.assertEqual( self.result.matched_count, 2 )
      
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [
         {'c': 1, '_id': 1, 'b': 2, 'a': 1}, 
         {'c': 1, '_id': 2, 'b': 2, 'a': 2}, 
         {'_id': 3, 'b': 1, 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs )
            
      self.cl.delete_many( {} )
      
   def updater_set( self ):
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": 1 }, 
         { "_id": 2, "a": 2, "b": 1 }, 
         { "_id": 3, "a": 3, "b": 1 }] )
      self.result = self.cl.update_many( { "a": { "$in": [1, 2] } }, { "$set": { "b": "test", "c": 1 } } )
      self.assertEqual( self.result.matched_count, 2 )      
      
      self.cursor = self.cl.find().sort( '_id' )
      self.expDocs = [
         {'_id': 1, 'c': 1, 'a': 1, 'b': 'test'}, 
         {'_id': 2, 'c': 1, 'a': 2, 'b': 'test'}, 
         {'_id': 3, 'a': 3, 'b': 1}]
      self.checkCursor( self.cursor, self.expDocs )
      
      self.cl.delete_many( {} )
      
   def updater_setOnInsert( self ):
      # $setOnInsert only support upsert:true, return fail while upsert:false
      # upsert: false
      try:
         self.cl.update_many( { "a": 4 }, { "$setOnInsert": { "b": 4 } }, False )
      except pymongo.errors.OperationFailure as e:
         pass
      except:
         raise

      # upsert:true
      self.result = self.cl.update_many( { "a": 1 }, { "$setOnInsert": { "_id": 1, "a": 1 } }, True )
      self.assertEqual( self.result.matched_count, 0 )    
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [{'_id': 1, 'a': 1}]
      self.checkCursor( self.cursor, self.expDocs )
      
      self.cl.delete_many( {} )
      
   def updater_set_inc_setOnInsert( self ):
      # doc not exist, upsert:true, include _id
      self.result = self.cl.update_many( {}, 
         { "$set": { "_id": 1 }, "$inc": { "a": 1 }, "$setOnInsert": { "b": 1 } }, True )
      self.assertEqual( self.result.matched_count, 0 )    
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [{'b': 1, '_id': 1, 'a': 1}]
      self.checkCursor( self.cursor, self.expDocs )

      # doc not exist, upsert:true, not include _id
      self.result = self.cl.update_many( { "_id": 2 }, 
         { "$set": { "a": 2 }, "$inc": { "b": 2 }, "$setOnInsert": { "c": 2 } }, True )
      self.assertEqual( self.result.matched_count, 0 )    
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [{'_id': 1, 'a': 1, 'b': 1}, {'_id': 2, 'a': 2, 'b': 2, 'c': 2}]
      self.checkCursor( self.cursor, self.expDocs )

      # doc exist, upsert:true
      self.result = self.cl.update_many( { "a": 1 }, 
         { "$set": { "a": 3 }, "$inc": { "b": 3 }, "$setOnInsert": { "notExist": 3 } }, True )
      self.assertEqual( self.result.matched_count, 1 )  
      
      self.cursor = self.cl.find( {}, { "_id": 0, "a": 1, "b": 1, "c": 1 } ).sort( 'a' )
      self.expDocs = [{'c': 2, 'b': 2, 'a': 2}, {'b': 4, 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs )
      
      self.cl.delete_many( {} )

   def updater_unset( self ):
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": 1 }, 
         { "_id": 2, "a": 2, "b": 2 }, 
         { "_id": 3, "a": 3, "b": 3 }] )
      self.result = self.cl.update_many( { "a": { "$lte": 2 } }, { "$unset": { "b": 1, "c": 1 } } )
      self.assertEqual( self.result.matched_count, 2 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [{'_id': 1, 'a': 1}, {'_id': 2, 'a': 2}, {'_id': 3, 'a': 3, 'b': 3}]
      self.checkCursor( self.cursor, self.expDocs )
         
      self.cl.delete_many( {} )

   def updater_pop( self ):
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": [1, 2], "c": [1, 2, 3, 4, 5] },
         { "_id": 2, "a": 2, "b": [1, 2], "c": [1, 2, 3, 4, 5] },
         { "_id": 3, "a": 3, "b": [1, 2], "c": [1, 2, 3, 4, 5] }] )
            
      # $pop: 1
      self.result = self.cl.update_many( { "a": { "$gt": 1 } }, { "$pop": { "b": 1, "c": 2 } } )
      self.assertEqual( self.result.matched_count, 2 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [
         {'b': [1, 2], 'a': 1, '_id': 1, 'c': [1, 2, 3, 4, 5]}, 
         {'b': [1], 'a': 2, '_id': 2, 'c': [1, 2, 3]}, 
         {'b': [1], 'a': 3, '_id': 3, 'c': [1, 2, 3]}]
      self.checkCursor( self.cursor, self.expDocs )

      # $pop: -1
      self.result = self.cl.update_many( { "a": { "$gt": 1 } }, { "$pop": { "b": -1, "c": -2 } } )
      self.assertEqual( self.result.matched_count, 2 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [
         {'a': 1, '_id': 1, 'c': [1, 2, 3, 4, 5], 'b': [1, 2]}, 
         {'a': 2, '_id': 2, 'c': [3], 'b': []}, 
         {'a': 3, '_id': 3, 'c': [3], 'b': []}]
      self.checkCursor( self.cursor, self.expDocs )

      # $pop: 0
      self.result = self.cl.update_many( { "a": 1 }, { "$pop": { "b": 0, "c": 0 } } )
      self.assertEqual( self.result.matched_count, 1 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.checkCursor( self.cursor, self.expDocs )

      # field is empty array
      self.result = self.cl.update_many( { "a": 2 }, { "$pop": { "b": 1 } } )
      self.assertEqual( self.result.matched_count, 1 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.checkCursor( self.cursor, self.expDocs )

      # field is not array
      self.result = self.cl.update_many( { "a": 1 }, { "$pop": { "a": 1 } } )
      self.assertEqual( self.result.matched_count, 1 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.checkCursor( self.cursor, self.expDocs )

      # field is not exist
      self.result = self.cl.update_many( { "a": 1 }, { "$pop": { "notExistField": 1 } } )
      self.assertEqual( self.result.matched_count, 1 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.checkCursor( self.cursor, self.expDocs )
         
      self.cl.delete_many( {} )

   def updater_pull( self ):
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": [], "c": [1, 2, 3, 4, 5] },
         { "_id": 2, "a": 2, "b": 1, "c": [1, 2, 3, 4, 5] },
         { "_id": 3, "a": 3, "b": [1], "c": [1, 3, 4, 5] }] )
            
      # filed exist, cover: is not array, empty array, value not exist
      self.result = self.cl.update_many( {}, { "$pull": { "b": 1, "c": 2 } } )
      self.assertEqual( self.result.matched_count, 3 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [
         {'_id': 1, 'b': [], 'c': [1, 3, 4, 5], 'a': 1}, 
         {'_id': 2, 'b': 1, 'c': [1, 3, 4, 5], 'a': 2}, 
         {'_id': 3, 'b': [], 'c': [1, 3, 4, 5], 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs )
      
      # filed exist, cover: is not array, empty array, value not exist
      self.result = self.cl.update_many( {}, { "$pull": { "b": 1, "c": 2 } } )
      self.assertEqual( self.result.matched_count, 3 )
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [
         {'_id': 1, 'b': [], 'c': [1, 3, 4, 5], 'a': 1}, 
         {'_id': 2, 'b': 1, 'c': [1, 3, 4, 5], 'a': 2}, 
         {'_id': 3, 'b': [], 'c': [1, 3, 4, 5], 'a': 3}]
      self.checkCursor( self.cursor, self.expDocs ) 

      # filed not exist
      self.result = self.cl.update_many( { "a": 3 }, { "$pull": { "notExistField": 2 } } )
      self.assertEqual( self.result.matched_count, 1 )
      
      self.cursor = self.cl.find().sort( 'a' )
      self.checkCursor( self.cursor, self.expDocs )
         
      self.cl.delete_many( {} )

   def updater_push( self ):
      self.cl.insert_many( [
         { "_id": 1, "a": 1, "b": [], "c": [1] },
         { "_id": 2, "a": 2, "b": 1, "c": [1] },
         { "_id": 3, "a": 3, "b": [1], "c": [1] }] )
            
      # filed exist, cover: is not array, empty array, value not exist
      self.result = self.cl.update_many( {}, { "$push": { "b": 1, "c": 2 } } )
      self.assertEqual( self.result.matched_count, 3 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [
         {'b': [1], '_id': 1, 'a': 1, 'c': [1, 2]}, 
         {'b': 1, '_id': 2, 'a': 2, 'c': [1, 2]}, 
         {'b': [1, 1], '_id': 3, 'a': 3, 'c': [1, 2]}]
      self.checkCursor( self.cursor, self.expDocs )

      # filed not exist
      self.result = self.cl.update_many( { "a": 1 }, { "$pull": { "notExistField": 3 } } )
      self.assertEqual( self.result.matched_count, 1 ) 
      
      self.cursor = self.cl.find().sort( 'a' )
      self.checkCursor( self.cursor, self.expDocs )
         
      self.cl.delete_many( {} )

   def updater_others( self ):
      # doc not exist, upsert:true, matcher field not same updater field
      self.result = self.cl.update_many( { "a": 1 }, { "$set": { "_id": 1 }, "$inc": { "b": 1 } }, True )
      self.assertEqual( self.result.matched_count, 0 )

      # doc not exist, upsert:false
      self.result = self.cl.update_many( { "a": 2 }, { "$set": { "_id": 2 }, "$inc": { "b": 2 } }, False )
      self.assertEqual( self.result.matched_count, 0 )
      
      self.cursor = self.cl.find().sort( 'a' )
      self.expDocs = [{'_id': 1, 'a': 1, 'b': 1}]
      self.checkCursor( self.cursor, self.expDocs )