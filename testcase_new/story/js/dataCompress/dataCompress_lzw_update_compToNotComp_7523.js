/************************************************************************
@Description:   seqDB-7523:记录已压缩，更新记录为不压缩
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert({INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,QRCODE_STRING: "need update",SA_OP_ACCT_NO: "6217001820000548390"}) ;
         3 find({INNER_NO:{$gte:500000}}).update({$replace:{"电子银行业务回单(付款)":"中国民生银行福州闽江支行"}})  ;
         4 check records, get random records, then compare the records;
           check records for each node in the group;
           check compressed rate. 
@output:   successfull
@Author:   
           2016/3/23   XiaoNi Huang init
************************************************************************/
main( test );

function test ()
{
   var noCSName = COMMCSNAME + "_no";
   var lzwCSName = COMMCSNAME + "_lzw";
   var noCLName = COMMCLNAME + "_no";
   var lzwCLName = COMMCLNAME + "_lzw";
   var rgName = getDataGroupsName()[0];
   var insertRecsNum = 800000;
   var checkRecsNum = 3; //get random 3 records

   commDropCS( db, noCSName, true, "Failed to drop CS[" + noCSName + "]." );
   commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

   commCreateCS( db, noCSName, false, "Failed to create CS[" + noCSName + "]." );
   commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );

   var noCL = createCL( noCSName, noCLName, rgName, false );
   var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );

   insertRecs( noCL, noCSName, noCLName, insertRecsNum );
   insertRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum );

   findAndUpdateRecs( noCL, noCSName, noCLName );
   findAndUpdateRecs( lzwCL, lzwCSName, lzwCLName );

   checkRecs( lzwCL, insertRecsNum, checkRecsNum );
   checkNodeCnt( lzwCSName, lzwCLName, rgName, insertRecsNum );
   checkCompressedRate( noCSName, lzwCSName );

   clearCS( db, noCSName );
   clearCS( db, lzwCSName );
}

function insertRecs ( cl, csName, clName, insertRecsNum )
{

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

   var rc = cl.find( { INNER_NO: { $gte: 300000 } } ).update( { $replace: { "电子银行业务回单(付款)": "中国民生银行福州闽江支行" } } );
   while( rc.next() );
}

function checkRecs ( cl, insertRecsNum, checkRecsNum )
{

   //get random records, compare the records

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );

      if( i < 300000 )
      {  //before update
         var recsCnt = cl.find( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, QRCODE_STRING: "need update", SA_OP_ACCT_NO: "6217001820000548390" } ).count();
         var expctCnt = 1;
      }
      else
      {  //after update
         var recsCnt = cl.find( { "电子银行业务回单(付款)": "中国民生银行福州闽江支行" } ).count();
         var expctCnt = insertRecsNum - 300000;
      }
      assert.equal( recsCnt, expctCnt );
   }
}