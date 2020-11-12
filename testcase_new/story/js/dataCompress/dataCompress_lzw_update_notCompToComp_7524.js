/************************************************************************
@Description:    seqDB-7524:记录未压缩，更新记录为压缩
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert:
                 ({total_account:i,account_id:i,tx_number:"test"+i,tx_info:"xzposs/565bf18944f4f14fea84341b/image/2016_1.png"}) 
                 ({INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,IVC_NAME: "电子银行业务回单(付款)",OPEN_BRANCH_NAME:"中国民生银行福州闽江支行"}) ;
         3 check records, get random records, then compare the records;
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
   var number1 = 700000;
   var insertRecsNum = 800000;
   var checkRecsNum = 3; //get random 3 records

   commDropCS( db, noCSName, true, "Failed to drop CS[" + noCSName + "]." );
   commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

   commCreateCS( db, noCSName, false, "Failed to create CS[" + noCSName + "]." );
   commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );

   var noCL = createCL( noCSName, noCLName, rgName, false );
   var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );

   insertRecs( noCL, noCSName, noCLName, number1, insertRecsNum );
   insertRecs( lzwCL, lzwCSName, lzwCLName, number1, insertRecsNum );

   findAndUpdateRecs( noCL, noCSName, noCLName, insertRecsNum );
   findAndUpdateRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum );

   checkRecs( lzwCL, number1, insertRecsNum, checkRecsNum );
   checkNodeCnt( lzwCSName, lzwCLName, rgName, insertRecsNum );
   checkCompressedRate( noCSName, lzwCSName );

   clearCS( db, noCSName );
   clearCS( db, lzwCSName );
}

function insertRecs ( cl, csName, clName, number1, insertRecsNum )
{

   for( k = 0; k < number1; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { total_account: i, account_id: i, tx_number: "test" + i, tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png" } )
      };
      cl.insert( doc );
   }

   for( k = number1; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行" } )
      };
      cl.insert( doc );
   }
}

function findAndUpdateRecs ( cl, csName, clName, insertRecsNum )
{

   var rc = cl.find( { INNER_NO: { $exists: 1 } } ).update( { $set: { total_account: insertRecsNum, tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png" } } );
   while( rc.next() );
}

function checkRecs ( cl, number1, insertRecsNum, checkRecsNum )
{

   //get random records, compare the records

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum + 1 );

      if( i < number1 )
      {  //before update
         var recsCnt = cl.find(
            { total_account: i, account_id: i, tx_number: "test" + i, tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png" } ).count();
         var expctCnt = 1;
      }
      else
      {  //after update
         var recsCnt = cl.find(
            { total_account: insertRecsNum, tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png" } ).count();
         var expctCnt = insertRecsNum - number1;
      }

      assert.equal( recsCnt, expctCnt );

   }
}