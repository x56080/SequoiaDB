"""

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = spilt.py

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

"""
#modify: Ting YU 2015-12-11
import pysequoiadb
from pysequoiadb import client
from pysequoiadb import collectionspace
from pysequoiadb import collection
from pysequoiadb import cursor
from pysequoiadb import const
from pysequoiadb.error import (SDBTypeError, SDBBaseError, SDBEndOfCursor)
import sys,getopt
import traceback

hostname=None
service=None

# parase the args
def parse_option():

   if len( sys.argv ) < 2:
      usage()
      sys.exit(1)
   
   try:  
      opts, args = getopt.getopt( sys.argv[1:], 'hH:p:', ['help'] )
   except getopt.GetoptError, err:
      print str( err )
      usage()
      sys.exit(1)
   
   global hostname, service
   for op, value in opts:
      if op == '-H':
         hostname = value
      elif op == '-p':
         service = int( value )
      elif op in ('-h', '--help'):
         usage()
         sys.exit()
      else:
         print 'arguments error'
         usage()
         sys.exit(1)
         
def usage():
   print 'Command options:'
   print '-h,--help  help'
   print '-H   arg   hostname'
   print '-p   arg   coord_port'

def isStandalone( db ):   
   print '---begin to get cluster mode'   
   is_standalone = False
   try:
      db.list_replica_groups()
   except SDBBaseError, e:
      if ( -159 == e.code ):
         is_standalone = True
      else:
         raise e   
         
   return is_standalone 
   
def getDataRGNum():
   print '---begin to get data group number'
   dataRGNum = 0
   rc = db.get_list( 7, condition={'Role': 0} )   
   while True:
      try:
         rc.next()
         dataRGNum = dataRGNum + 1
      except SDBEndOfCursor:
         break
      except ( Exception ), e:
         raise e
   return dataRGNum
   
def createSplitCL( csName, clName, splitType ):
   print '---begin to drop cs in ready'
   try:
      db.drop_collection_space( csName )
   except SDBBaseError, e:
      if ( -34 != e.code ):         
         raise e
                   
   print '---begin to create cs cl: %s.%s' % ( csName, clName )
   cs = db.create_collection_space( csName )
   option = { 'ReplSize':0, 'ShardingKey':{'a':1}, 'ShardingType':splitType }
   cl = cs.create_collection( clName, option )
   
   return cl

# get source group   
def getSrcRG( clFullName ):    
   print '---begin to get source group of cl'
   rc = db.get_snapshot( 8, condition={'Name': clFullName} )
   info = rc.next()
   srcRG = info['CataInfo'][0]['GroupName']   
   return srcRG

# get target group    
def getTgtRG( srcRG ):
   print '---begin to get target group'
   rc = db.get_list( 7, condition={'Role': 0} ) 
   while True:
      try:
         info = rc.next()
         dataRG = info['GroupName']
         if( dataRG != srcRG ):
            tgtRG = dataRG
            break
      except SDBEndOfCursor:
         break
      except ( Exception ), e:
         raise e
         
   return tgtRG

def splitByCondition( clFullName, srcRG, tgtRG ):
   print '---begin to insert 10 records'
   for i in range(10):
      cl = db.get_collection( clFullName )
      cl.insert( {'a':i} )
      
   print '---begin to split by condition, source = %s, target = %s' % (srcRG, tgtRG)
   cl.split_by_condition( srcRG, tgtRG, {'a':0}, {'a':5} )
   
   print '---begin to check split result'
   srcSvcHost = db.get_replica_group_by_name( srcRG ).get_master()
   tgtSvcHost = db.get_replica_group_by_name( tgtRG ).get_master()
   srcHost = srcSvcHost.get_hostname()
   tgtHost = tgtSvcHost.get_hostname()
   srcSvc  = srcSvcHost.get_servicename()
   tgtSvc  = tgtSvcHost.get_servicename() 
    
   dbSrc = client( srcHost, srcSvc )
   dbTgt = client( tgtHost, tgtSvc )
   
   cntSrc = dbSrc.get_collection( clFullName ).get_count()
   cntTgt = dbTgt.get_collection( clFullName ).get_count()
   
   if ( cntSrc != 5 ):
      print 'exec: db=client(%s,%s), db.cs.cl.get_count()' % ( srcHost, srcSvc )
      print 'expect: 5, actual: %d ' % ( cntSrc )
      raise  Exception( 'SOURCERG_COUNT_ERROR' )
   if ( cntTgt != 5 ):
      print 'exec: db=client(%s,%s), db.cs.cl.get_count()' % ( tgtHost, tgtSvc )
      print 'expect: 5, actual: %d ' % ( cntTgt )
      raise  Exception( 'TARGET_COUNT_ERROR' )

def splitByPercent( clFullName, srcRG, tgtRG ):
   print '---begin to insert 100 records'
   for i in range(100):
      cl = db.get_collection( clFullName )
      cl.insert( {'a':i} )
      
   print '---begin to split by percent, source = %s, target = %s' % (srcRG, tgtRG)
   cl.split_by_percent( srcRG, tgtRG, 50.0 )
   
   print '---begin to check split result'
   srcSvcHost = db.get_replica_group_by_name( srcRG ).get_master()
   tgtSvcHost = db.get_replica_group_by_name( tgtRG ).get_master()
   srcHost = srcSvcHost.get_hostname()
   tgtHost = tgtSvcHost.get_hostname()
   srcSvc  = srcSvcHost.get_servicename()
   tgtSvc  = tgtSvcHost.get_servicename() 
    
   dbSrc = client( srcHost, srcSvc )
   dbTgt = client( tgtHost, tgtSvc )
   
   cntSrc = dbSrc.get_collection( clFullName ).get_count()
   cntTgt = dbTgt.get_collection( clFullName ).get_count()
   
   if ( cntSrc == 0 ):
      print 'exec: db=client(%s,%s), db.cs.cl.get_count()' % ( srcHost, srcSvc )
      print 'expect: >0, actual: %d ' % ( cntSrc )
      raise  Exception( 'SOURCERG_COUNT_ERROR' )
   if ( cntTgt == 0 ):
      print 'exec: db=client(%s,%s), db.cs.cl.get_count()' % ( tgtHost, tgtSvc )
      print 'expect: >0, actual: %d ' % ( cntTgt )
      raise  Exception( 'TARGET_COUNT_ERROR' )
                 
def clean( csName ):
   print '---begin to drop cs in finally'
   try:
      db.drop_collection_space( csName )
   except SDBBaseError, e:
      if ( -34 != e.code ):
         pysequoiadb._print(e.detail)              
         raise e
      
if __name__ == "__main__":   
   try:    
      parse_option()
      
      csName = "pydriver_split_cs"
      clName = "pydriver_split_cl"
      clFullName = csName+'.'+clName
      db = client( hostname, service )
      
      if( isStandalone( db ) == True ):
         print 'Mode is standalone!'
         exit(0)
      elif( getDataRGNum() < 2 ):
         print 'This testcase needs at least 2 data groups to split!'
         exit(0)  
      else:       
         createSplitCL( csName, clName, 'range' )
         srcRG = getSrcRG( clFullName )
         tgtRG = getTgtRG( srcRG )
         splitByCondition( clFullName, srcRG, tgtRG )
         
         createSplitCL( csName, clName, 'hash' )
         srcRG = getSrcRG( clFullName )
         tgtRG = getTgtRG( srcRG )
         splitByPercent( clFullName, srcRG, tgtRG )
   
   except SDBBaseError, e:
      pysequoiadb._print( e.detail )
      raise e  
            
   finally:  
      if( locals().has_key('db') ):                    
         clean( csName )     
         db.disconnect()
         del db 