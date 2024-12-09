/******************************************************************************
@Description : SEQUOIADBMAINSTREAM-10198，多次更新集合属性，主备LSN不一致
@Modify list :
               2024-12-10 fangjiabin  Init
******************************************************************************/
testConf.skipStandAlone = true ;
main( test ) ;

function test ()
{
   var clName = "alter10198" ;
   var clFullName = COMMCSNAME + "." + clName ;

   commDropCL( db, COMMCSNAME, clName ) ;

   var cl = commCreateCL( db, COMMCSNAME, clName, { "ReplSize": 0 } ) ;

   checkClAttribute( 1, clFullName ) ;

   cl.createIdIndex() ;
   checkClAttribute( 2, clFullName ) ;

   cl.setAttributes( { AutoIndexId: true } ) ;
   checkClAttribute( 3, clFullName ) ;

   cl.setAttributes( { AutoIndexId: true, CompressionType: 'snappy' } ) ;
   checkClAttribute( 4, clFullName ) ;

   cl.setAttributes( { AutoIndexId: true, CompressionType: 'snappy' } ) ;
   checkClAttribute( 5, clFullName ) ;

   cl.dropIdIndex() ;
   checkClAttribute( 6, clFullName ) ;

   cl.dropIdIndex() ;
   checkClAttribute( 7, clFullName ) ;

   cl.setAttributes( { AutoIndexId: false } ) ;
   checkClAttribute( 8, clFullName ) ;

   cl.enableSharding( { ShardingKey: { a: 1 } } ) ;
   checkClAttribute( 9, clFullName ) ;

   cl.enableSharding( { ShardingKey: { a: 1 } } ) ;
   checkClAttribute( 10, clFullName ) ;

   cl.disableSharding() ;
   checkClAttribute( 11, clFullName ) ;

   cl.disableSharding() ;
   checkClAttribute( 12, clFullName ) ;

   commDropCL( db, COMMCSNAME, clName ) ;
}

function checkClAttribute( step, clFullName )
{
   try
   {
      var compressionType = "" ;
      var dataCommitLSN = "" ;
      var indexCommitLSN = "" ;
      var t1_sqlStr = "select * from $SNAPSHOT_CL where Name='" + clFullName + "' split by Details" ;
      var t2_sqlStr = "select T1.Name, T1.Details.CompressionType, T1.Details.DataCommitLSN, T1.Details.IndexCommitLSN, T1.Details.NodeName from ( " + t1_sqlStr + " ) as T1" ;

      db.sync() ;

      var isFirst = true ;
      var firstObj = "" ;
      var rc = db.exec( t2_sqlStr ) ;
      println( "Begin " + step ) ;
      while ( rc.next() )
      {
         var obj = rc.current().toObj() ;
         println( JSON.stringify( obj ) ) ;

         if ( isFirst )
         {
            compressionType = obj.CompressionType ;
            dataCommitLSN = obj.DataCommitLSN ;
            indexCommitLSN = obj.IndexCommitLSN ;
            firstObj = obj ;
            isFirst = false ;
            continue ;
         }

         println( JSON.stringify( { "CompressionType": compressionType, "DataCommitLSN": dataCommitLSN, "IndexCommitLSN": indexCommitLSN } ) ) ;

         if ( compressionType != obj.CompressionType ||
              dataCommitLSN != obj.DataCommitLSN ||
              indexCommitLSN != obj.IndexCommitLSN )
         {
            if ( 0 == retryTime )
            {
               throw new Error( "Diff info: \nexpect: " + JSON.stringify( firstObj ) + ", \nbut found: " + JSON.stringify( obj ) ) ;
            }
         }
      }
      println( "End " + step ) ;
   }
   catch( e )
   {
      if( e instanceof Error )
      {
         print( "Error Stack:\n" + e.stack ) ;
      }
      throw e ;
   }
}