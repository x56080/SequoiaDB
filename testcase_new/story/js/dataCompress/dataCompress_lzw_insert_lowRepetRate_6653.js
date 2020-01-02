/************************************************************************
@Description:   seqDB-6653:构建字典后，插入记录中大部分子串在字典中都匹配不到
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
      var number1 = 600000;  //first insert
      var insertRecsNum = 800000;  //total number
      var checkRecsNum = 3; //get random 3 records

      println( "\n---Begin to drop CS in the pre-condition." );
      commDropCS( db, noCSName, true, "Failed to drop CS[" + noCSName + "]." );
      commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

      println( "\n---Begin to create CS." );
      commCreateCS( db, noCSName, false, "Failed to create CS[" + noCSName + "]." );
      commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );

      var noCL = createCL( noCSName, noCLName, rgName, false );
      var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );

      insertRecs( noCL, noCSName, noCLName, number1, insertRecsNum );
      insertRecs( lzwCL, lzwCSName, lzwCLName, number1, insertRecsNum );

      checkRecs( lzwCL, number1, insertRecsNum, checkRecsNum );
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

function insertRecs ( cl, csName, clName, number1, insertRecsNum )
{
   println( "\n---Begin to insert records, CL[" + csName + "." + clName + "], " + "insertRecsNum: " + insertRecsNum );

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

function checkRecs ( cl, number1, insertRecsNum, checkRecsNum )
{
   println( "\n---Begin to check Records. checkRecsNum: " + checkRecsNum );

   //get random records, compare the records
   println( '   i < ' + number1
      + ', recs:           {total_account:i,account_id:i,tx_number:"test"+i,tx_info:"xzposs/565bf18944f4f14fea84341b/image/2016_1.png"}' );
   println( '   ' + number1 + ' <= i < ' + insertRecsNum
      + ', recs: {INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,IVC_NAME: "电子银行业务回单(付款)",OPEN_BRANCH_NAME:"中国民生银行福州闽江支行"}' );

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );
      println( "   random i: " + i );

      if( i < number1 )
      {
         var recsCnt = cl.find(
            { total_account: i, account_id: i, tx_number: "test" + i, tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png" } ).count();
      }
      else
      {
         var recsCnt = cl.find(
            { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行" } ).count();
      }

      var expctCnt = 1;
      if( parseInt( recsCnt ) !== expctCnt )
      {
         throw buildException( "Failed to check Records.", null, "[checkRecords]",
            "recsCnt: " + expctCnt, "recsCnt: " + parseInt( recsCnt ) );
      }

   }
}