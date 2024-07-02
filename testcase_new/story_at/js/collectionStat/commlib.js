import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function checkCollectionStat( cl, clName, version, IsDefault, IsExpired, AvgNumFields, SampleRecords, TotalRecords, TotalDataPages, TotalDataSize ){
   var actResult = cl.getCollectionStat().toObj() ;
   delete( actResult.StatTimestamp ) ;
   var expResult = {
      "Collection": COMMCSNAME + "." + clName,
      "InternalV": version,
      "IsDefault": IsDefault ,
      "IsExpired": IsExpired ,
      "AvgNumFields": AvgNumFields ,
      "SampleRecords": SampleRecords ,
      "TotalRecords": TotalRecords ,
      "TotalDataPages": TotalDataPages ,
      "TotalDataSize":TotalDataSize
   } ;
   if( !commCompareObject( expResult, actResult ) ){
      throw new Error( "\nExpected:\n" + JSON.stringify( expResult ) + "\nactual:\n" + JSON.stringify( actResult ) );
   }
}

function getCollectionGroupCnt( db, clName ) {
   var sql = " select count(T.CataInfo.GroupID) as GroupCnt from ( select * from $SNAPSHOT_CATA where Name = \"" + clName + "\" split by CataInfo ) as T group by T.GroupID" ;
   var cursor = db.exec( sql ) ;
   var obj ;
   var cnt = 0 ;
   while ( obj = cursor.next() ) {
      cnt = obj.toObj()["GroupCnt"] ;
   }
   return cnt ;
}
