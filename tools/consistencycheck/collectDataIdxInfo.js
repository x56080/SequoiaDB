/****************************************************************
@decription:   Collect data collection index infos

@input:        HOSTNAME:                   String, eg: "localhost", required
               SVCNAME:                    Number, eg: 11810, required
               USERNAME:                   String, required
               PASSWD:                     String, default: ""
               TOKEN:                      String
               CIPHER_FILE:                String
               PLAN:                       Array

@example:
    ../../bin/sdb -f collectDataIdxInfo.js -e 'var HOSTNAME="localhost";var SVCNAME=11810;
                                               var USERNAME="xxx";var PASSWD="xxx";var TOKEN="xxx";
                                               var CIPHER_FILE="xxx";var PLAN={xxx}'

@author:       FangJiabin 2024-11-20
****************************************************************/

import("./config.js")

/*
PLAN =
{
   "Skip": skip,
   "Limit": limit,
   "CoordAddr": { "HostName": "192.168.17.50", "NodeName": 11810 },
   "UserName": USERNAME,
   "Password": PASSWD,
   "Token": TOKEN,
   "CipherFile": CIPHER_FILE,
   "CollectionsNum": collectionNum
}
*/
var skip = PLAN.Skip ;
var limit = PLAN.Limit ;
var hostname = PLAN.CoordAddr.HostName ;
var nodename = PLAN.CoordAddr.NodeName ;
var username = PLAN.UserName ;
var password = PLAN.Password ;
var token = PLAN.Token ;
var cipherFile = PLAN.CipherFile ;
var collectionNum = PLAN.CollectionsNum ;

try
{
   var USER = null ;
   if ( cipherFile == "" )
   {
      USER = new User( username, password );
      var db = new Sdb( hostname, nodename, username, password ) ;
   }
   else
   {
      if ( cipherFile == "~/sequoiadb/passwd" )
      {
         USER = new CipherUser( username ).token( token ) ;
      }
      else
      {
         USER = new CipherUser( username ).token( token ).cipherFile( cipherFile ) ;
      }
      var db = new Sdb( hostname, nodename, USER ) ;
   }

   var pid = System.getPID() ;
   var filePath = PROGRESS_TMP_FILEPATH_PREFIXX + pid ;
   var file = new File( filePath, 0664, SDB_FILE_READWRITE|SDB_FILE_CREATE ) ;
   file.truncate( 0 ) ;
}
catch( e )
{
   println( "Failed to connect coord[" + hostname + ":" + nodename +
            "], username[" + username + "], error[" + e + "]" ) ;
   throw new Error() ;
}

collectDataIndexInfo() ;

function canRetrySnapIndexes( errNodes )
{
   for ( i in errNodes )
   {
      var errNode = errNodes[i] ;
      if ( -338 == errNode.ErrInfo.errno )
      {
         return true ;
      }
      else
      {
         return false ;
      }
   }
}

function collectDataIndexInfo()
{
   /*

   {
      "NodeName": "u16-fjb:20000",
      "GroupName": "db1",
      "IndexDef": {
         "UniqueID": 300647710720,
         "key": {
            "_id": 1
         },
         "v": 0,
         "unique": true,
         "dropDups": false,
         "enforced": true,
         "NotNull": false,
         "NotArray": true,
         "Global": false,
         "Standalone": false
      },
      "IndexName": "$id",
      "ClFullName": "csName_0.subcl_0",
      "MainClName": "csName_0.maincl",
      "DataClFullName": "csName_0.subcl_0",
      "IndexDataType": 0,
      "UniqueID": 300647710720,
      "NodeInfo": {
         "NodeName": "u16-fjb:20000",
         "IndexName": "$id",
         "UniqueID": 300647710720,
         "IndexDef": {
            "UniqueID": 300647710720,
            "key": {
            "_id": 1
            },
            "v": 0,
            "unique": true,
            "dropDups": false,
            "enforced": true,
            "NotNull": false,
            "NotArray": true,
            "Global": false,
            "Standalone": false
         }
      }
   }

   */
   try
   {
      var errMsg = "" ;
      var batchRecords = [] ;
      var tmpCS = db.getCS( TMP_CS_UPGRADE_INDEX ) ;
      var dataIndexInfoCl = tmpCS.getCL( TMP_CL_DATA_INDEX_INFO ) ;
      var cataClusterCl = tmpCS.getCL( TMP_CL_CATA_CLUSTER_CL_INFO ) ;
      var clCount = 0 ;

      var rc = cataClusterCl.find( { "DataClFullName": { "$isnull": 1 } }, { "ClFullName": 1, "MainClName": 1 } ).skip( skip ).limit( limit ).sort( { "ClFullName": 1, "MainClName": 1 } ) ;
      while( rc.next() )
      {
         var tryTime = 10 ;
         var needRetry = false ;
         var obj = rc.current().toObj() ;
         var mainClName = obj.MainClName ;
         var clFullName = obj.ClFullName ;
         var csName = clFullName.split(".")[0] ;
         var clName = clFullName.split(".")[1] ;
         var cl = db.getCS( csName ).getCL( clName ) ;
         var rc1 ;

         while ( tryTime >= 0 )
         {
            needRetry = false ;
            rc1 = cl.snapshotIndexes( { RawData: true }, { NodeName: 1, GroupName: 1, IndexDef: 1 } ) ;
            while ( rc1.next() )
            {
               var obj1 = rc1.current().toObj() ;

               if ( undefined == obj1.IndexDef )
               {
                  errMsg = "Failed to collect data index info[PID: " + pid + ", errMsg: " + JSON.stringify( obj1 ) + "]\n" ;
                  file.write( errMsg ) ;

                  if ( tryTime <= 0 || !canRetrySnapIndexes( obj1.ErrNodes ) )
                  {
                     throw new Error( errMsg ) ;
                  }

                  file.write( "Sleep 1s and try agant later\n" ) ;

                  // Need to remove all indexes info of current cl before retry
                  dataIndexInfoCl.insert( batchRecords ) ;
                  batchRecords = [] ;
                  dataIndexInfoCl.remove( { "ClFullName": clFullName } )

                  sleep( 1000 ) ;
                  tryTime-- ;
                  rc1.close() ;
                  needRetry = true ;

                  break ;
               }

               delete obj1.IndexDef._id ;
               obj1.IndexName = obj1.IndexDef.name ;
               delete obj1.IndexDef.name ;
               obj1.ClFullName = clFullName ;
               obj1.MainClName = mainClName ;
               obj1.DataClFullName = clFullName ;
               obj1.IndexDataType = 0 ;

               if ( undefined == obj1.IndexDef.UniqueID )
               {
                  obj1.UniqueID = "" ;
               }
               else
               {
                  obj1.UniqueID = obj1.IndexDef.UniqueID ;
               }
               obj1.NodeInfo = { "NodeName": obj1.NodeName, "IndexName": obj1.IndexName,
                                 "UniqueID": obj1.UniqueID, "IndexDef": obj1.IndexDef } ;

               batchRecords.push( obj1 ) ;

               if ( batchRecords.length > BATCH_NUM )
               {
                  dataIndexInfoCl.insert( batchRecords ) ;
                  batchRecords = [] ;
               }
            }

            if ( !needRetry )
            {
               break ;
            }
         }

         clCount++ ;

         if ( clCount == collectionNum )
         {
            if ( batchRecords.length > 0 )
            {
               dataIndexInfoCl.insert( batchRecords ) ;
               batchRecords = [] ;
            }
         }

         file.write( clFullName + "\n" ) ;
      }

      if ( clCount != collectionNum )
      {
         errMsg = "Failed to collect data index info[PID: " + pid +
                  ", expect cl count: " + collectionNum + ", actual cl count: " + clCount + "]\n" ;
         file.write( errMsg ) ;
         throw new Error( errMsg ) ;
      }

      file.truncate( 0 ) ;
      file.write( "0" ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         errMsg = "Failed to collect data index infos[PID: " + pid + "], error Stack: " + e.stack + "\n" ;
         file.write( errMsg ) ;
      }
      throw e ;
   }
   finally
   {
      file.close() ;
      db.close() ;
   }
}