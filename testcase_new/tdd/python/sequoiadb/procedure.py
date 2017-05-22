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
   except getopt.GetoptError  as err:
      print( str( err ) )
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
         print( 'arguments error' )
         usage()
         sys.exit(1)
         
def usage():
   print( 'Command options:' )
   print( '-h,--help  help' )
   print( '-H   arg   hostname' )
   print( '-p   arg   coord_port' )

def isStandalone( db ):   
   print( '---begin to get cluster mode' )   
   is_standalone = False
   try:
      db.list_replica_groups()
   except SDBBaseError as e:
      if ( -159 == e.code ):
         is_standalone = True
      else:
         raise e   
         
   return is_standalone 
   
def ready():
   print( '---begin to remove procedure in ready' )
   try:
      db.remove_procedure( 'sum' )
   except SDBBaseError as e:
      if ( -233 != e.code ):         
         raise e
   
def procedure():     
   print( '---begin to create procedure' )
   code = 'function sum(x,y){return x+y;}'
   db.create_procedure( code )
   
   print( '---begin to list procedure' )
   cond = {'name': 'sum'}
   cursor = db.list_procedures( condition=cond )
   
   i = 0        
   while True:
      try:
         record = cursor.next()
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e
   if( i != 1 ):
      print( 'exeute: db.list_procedures( condition=%s )' % ( cond ) )
      print( 'return record number, expect: 1, actual: %d' % ( i ) )
      raise  Exception( 'COUNT_ERROR' ) 
   
   print( '---begin to exec procedure' )
   cursor = db.eval_procedure( 'sum(1,2)' )
   
   i = 0        
   while True:
      try:
         result = cursor.next()
         if( result['value'] != 3 ):
            print( 'exeute: eval_procedure("sum(1,2)")' )
            print( 'expect: 3, actual: %d' % ( result['value'] ) )
            raise  Exception( 'RESULT_ERROR' )
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e
   if( i != 1 ):
      print( 'exeute: db.list_procedures( condition=%s )' % ( cond ) )
      print( 'return record number, expect: 1, actual: %d' % ( i ) )
      raise  Exception( 'COUNT_ERROR' ) 
    
   print( '---begin to remove procedure' )
   db.remove_procedure('sum')
   
   cond = {'name': 'sum'}
   cursor = db.list_procedures( condition=cond )  
   i = 0        
   while True:
      try:
         record = cursor.next()
         i = i + 1
      except SDBEndOfCursor :
         break
      except SDBBaseError :
         raise e
   if( i != 0 ):
      print( 'exeute: remove, list_procedures( condition=%s )' % ( cond ) )
      print( 'return record number, expect: 0, actual: %d' % ( i ) )
      raise  Exception( 'COUNT_ERROR' )     
      
def clean():
   print( '---begin to remove procedure in clean' )
   try:
      db.remove_procedure( 'sum' )
   except SDBBaseError as e:
      if ( -233 != e.code and -159 != e.code ): 
         pysequoiadb._print(e.detail)          
         raise e
      
if __name__ == "__main__":   
   try:    
      parse_option()
            
      db = client( hostname, service )
           
      # main
      if( isStandalone( db ) == True ):
         print( 'Mode is standalone!' )
         exit(0) 
      else:  
         ready()
         procedure()
      
   except SDBBaseError as e:
      pysequoiadb._print( e.detail )
      raise e  
            
   finally:  
      if( 'db' in locals() ):                    
         clean()     
         db.disconnect()
         del db 