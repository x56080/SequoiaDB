/************************************************************************
@Description:   seqDB-6658:findAndRemove删除记录_st.compress.03.017
@input:    
         1 create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert({INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,IVC_NAME: "电子银行业务回单(付款)",OPEN_BRANCH_NAME:"中国民生银行福州闽江支行"}) ;
         3 findAndRemove:
               find({$and:[{INNER_NO:{$gte:200000}},{$et:{IVC_NAME: "电子银行业务回单(付款)"}}]}).remove() ;
         4 check records, get random records, then compare the records;
           check records for each node in the group;
@output:   successfull
@Author:   
           2016/3/23   XiaoNi Huang init
************************************************************************/
main( test );

function test ()
{
   var lzwCSName = COMMCSNAME + "_lzw";
   var lzwCLName = COMMCLNAME + "_lzw";
   var rgName = getDataGroupsName()[0];
   var insertRecsNum = 800000;
   var remainNum = 200000; //remaining records after removed
   var checkRecsNum = 3; //get random 3 records

   commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

   commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );

   var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );

   insertRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum );

   findAndRemoveRecs( lzwCL, lzwCSName, lzwCLName );

   checkRecs( lzwCL, insertRecsNum, checkRecsNum );
   checkNodeCnt( lzwCSName, lzwCLName, rgName, remainNum );

   clearCS( db, lzwCSName );
}

function insertRecs ( cl, csName, clName, insertRecsNum )
{

   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行" } )
      };
      cl.insert( doc );
   }
}

function findAndRemoveRecs ( cl, csName, clName )
{

   var rc = cl.find( { $and: [{ INNER_NO: { $gte: 200000 } }, { IVC_NAME: { $et: "电子银行业务回单(付款)" } }] } ).remove();
   while( rc.next() );
}

function checkRecs ( cl, insertRecsNum, checkRecsNum )
{

   //get random records, compare the records

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );

      var recsCnt = cl.find( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行" } ).count();
      if( i < 200000 )
      {  //not removed
         var expctCnt = 1;
      }
      else
      {  //removed
         var expctCnt = 0;
      }

      assert.equal( recsCnt, expctCnt );
   }
}