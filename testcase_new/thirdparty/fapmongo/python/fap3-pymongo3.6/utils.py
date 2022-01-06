# -- coding: utf-8 --
'''
Description   : pymongo公共方法
Author        : XiaoNi Huang
CreateTime    : 2021.03.31
LastEditTime  : 2021.04.13
LastEditors   : XiaoNi Huang
'''
#!/usr/bin/python3.5
import pymongo
from pymongo import MongoClient
import config
from copy import copy
from datetime import datetime
import unittest

class TestBase( unittest.TestCase ):  
   def __init__( self, methodName = 'runPymongoTest' ):
      unittest.TestCase.__init__( self, methodName = methodName )

   @classmethod
   def setUpClass( cls ):
      print( cls.__name__ + " setUp: " + str( datetime.now() ) )
      cls.client = cls.commClient()
      cls.COMMDBNAME = config.config.db_prefix + "_db"
      cls.COMMCLNAME = "_cl"

   @classmethod
   def tearDownClass( cls ):
      cls.client.close()
      print( "\n" + cls.__name__ + " tearDown: " + str( datetime.now() ) )
         
   def commClient():
      return MongoClient( config.config.host, config.config.port )
   
   def checkCursor( self, cursor, expectDocs):
      self.docs = []
      for self.doc in cursor:
          self.docs.append( self.doc )
      self.assertEqual( self.docs, expectDocs )