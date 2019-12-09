/************************************************************************
@Description:   seqDB-6663:创建复合键唯一索引，批量修改_st.compress.03.022
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
           create index[{INNER_NO:1,SA_ACCT_NO:-1}, true, true] ;
         2 insert({INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,QRCODE_STRING: "need update",SA_OP_ACCT_NO: "6217001820000548390"}) ;
         3 find({$and:[{INNER_NO:{$gte:300000}},{SA_ACCT_NO:{$lt:700000}]}).update({$set:{QRCODE_STRING: "need update by index"}})  ;
         4 check records, get random records, then compare the records;
           check records for each node in the group;
           check compressed rate. 
@output:   successfull
@Author:   
           2016/3/23   XiaoNi Huang init
************************************************************************/
main();

function main ()
{
   try
   {
      var noCSName = COMMCSNAME + "_no";
      var lzwCSName = COMMCSNAME + "_lzw";
      var noCLName = COMMCLNAME + "_no";
      var lzwCLName = COMMCLNAME + "_lzw";
      var rgName = getDataGroupsName()[0];
      var idxName = "idx";
      var insertRecsNum = 800000;
      var checkRecsNum = 3; //get random 3 records

      println( "\n---Begin to drop CS in the pre-condition." );
      commDropCS( db, noCSName, true, "Failed to drop CS[" + noCSName + "]." );
      commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

      println( "\n---Begin to create CS." );
      commCreateCS( db, noCSName, false, "Failed to create CS[" + noCSName + "]." );
      commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );

      var noCL = createCL( noCSName, noCLName, rgName, false );
      var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );

      createIdx( noCL, idxName );
      createIdx( lzwCL, idxName );

      insertRecs( noCL, noCSName, noCLName, insertRecsNum );
      insertRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum );

      findAndUpdateRecs( noCL, noCSName, noCLName );
      findAndUpdateRecs( lzwCL, lzwCSName, lzwCLName );

      checkRecs( lzwCL, insertRecsNum, checkRecsNum, idxName );
      checkNodeCnt( lzwCSName, lzwCLName, rgName, insertRecsNum );
      checkCompressedRate( noCSName, lzwCSName );

      println( "\n---Begin to drop cs in the end-condition." );
      commDropCS( db, noCSName, false, "Failed to drop CS[" + noCSName + "]." );
      commDropCS( db, lzwCSName, false, "Failed to drop CS[" + lzwCSName + "]." );
   }
   catch( e )
   {
      throw e;
   }
}

function createIdx ( cl, idxName )
{
   println( "\n---Begin to create index[{INNER_NO:1,SA_ACCT_NO:-1}, true, true]" );

   cl.createIndex( idxName, { INNER_NO: 1, SA_ACCT_NO: -1 }, true, true );
}

function insertRecs ( cl, csName, clName, insertRecsNum )
{
   println( "\n---Begin to insert records, CL[" + csName + "." + clName + "], " + "insertRecsNum: " + insertRecsNum );

   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, QRCODE_STRING: "need update", SA_OP_ACCT_NO: "6217001820000548390" } )
      };
      cl.insert( doc );
   }
}

function findAndUpdateRecs ( cl, csName, clName )
{
   println( "\n---Begin to findAndUpdate records, CL[" + csName + "." + clName + "]" );

   var rcFU = cl.find( { $and: [{ INNER_NO: { $gte: 300000 } }, { SA_ACCT_NO: { $lt: 700000 } }] } ).update( { $set: { QRCODE_STRING: "need update by index" } } );
   while( rcFU.next() );
}

function checkRecs ( cl, insertRecsNum, checkRecsNum, idxName )
{
   println( "\n---Begin to check Records. checkRecsNum: " + checkRecsNum );

   //get random records, compare the records
   println( '   recs befor update: {INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,QRCODE_STRING: "need update",         SA_OP_ACCT_NO: "6217001820000548390"}' );
   println( '   recs befor update: {INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,QRCODE_STRING: "need update by index",SA_OP_ACCT_NO: "6217001820000548390"}' );

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );
      println( "   random i: " + i );

      if( i < 300000 || i >= 700000 )
      {  //before update
         var recsCnt = cl.find(
            { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, QRCODE_STRING: "need update", SA_OP_ACCT_NO: "6217001820000548390" } ).count();
      }
      else
      {  //after update
         var recsCnt = cl.find(
            { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, QRCODE_STRING: "need update by index", SA_OP_ACCT_NO: "6217001820000548390" } ).count();
      }
      var expctCnt = 1;
      if( parseInt( recsCnt ) !== expctCnt )
      {
         throw buildException( "Failed to check Records.", null, "[checkRecords]",
            "recsCnt: " + expctCnt, "recsCnt: " + parseInt( recsCnt ) );
      }
   }

   var tmpInfo = cl.find( { INNER_NO: 0, SA_ACCT_NO: 0 } ).explain().current().toObj();
   var scanType = tmpInfo["ScanType"];
   var indexName = tmpInfo["IndexName"];
   if( scanType !== "ixscan" || indexName !== idxName )
   {
      throw buildException( "Failed to check explain by index key.", null, "[checkRecords]",
         "scanType: 'ixscan', indexName: " + idxName, "scanType: " + scanType + ", indexName: " + indexName );
   }

}