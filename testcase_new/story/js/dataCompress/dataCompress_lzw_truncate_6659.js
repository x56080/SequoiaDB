/************************************************************************
@Description:    seqDB-6659:truncate删除记录后并再次插入数据_st.compress.03.018
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert({INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,IVC_NAME: "电子银行业务回单(付款)",OPEN_BRANCH_NAME:"中国民生银行福州闽江支行"}) ;
         3 truncate() ;
         4 insert again ;
         5 check attribute of CL[lzw];
           check records, get random records, then compare the records;
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

      insertRecs( noCL, noCSName, noCLName, insertRecsNum );
      insertRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum );

      truncateRecs( noCL, noCSName, noCLName );
      truncateRecs( lzwCL, lzwCSName, lzwCLName );

      //insert again
      insertRecs( noCL, noCSName, noCLName, insertRecsNum );
      insertRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum );

      checkAttributeOfCL( lzwCSName, lzwCLName, true, "lzw" );
      checkRecs( lzwCL, insertRecsNum, checkRecsNum );
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

function insertRecs ( cl, csName, clName, insertRecsNum )
{
   println( "\n---Begin to insert records, CL[" + csName + "." + clName + "], " + "insertRecsNum: " + insertRecsNum );

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

function truncateRecs ( cl, csName, clName )
{
   println( "\n---Begin to truncate records, CL[" + csName + "." + clName + "]" );

   cl.truncate();

   var recsCnt = cl.count();
   if( parseInt( recsCnt ) !== 0 )
   {
      throw buildException( "Failed to truncate.", null, "[truncateRecs]",
         "recsCnt: 0", "recsCnt: " + recsCnt );
   }
}

function checkRecs ( cl, insertRecsNum, checkRecsNum )
{
   println( "\n---Begin to check Records. checkRecsNum: " + checkRecsNum );

   //get random records, compare the records
   println( '   recs: {INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,IVC_NAME: "电子银行业务回单(付款)",OPEN_BRANCH_NAME:"中国民生银行福州闽江支行"}' );

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );
      println( "   random i: " + i );

      var recsCnt = cl.find( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行" } ).count();
      var expctCnt = 1;
      if( parseInt( recsCnt ) !== expctCnt )
      {
         throw buildException( "Failed to check Records.", null, "[checkRecords]",
            "recsCnt: " + expctCnt, "recsCnt: " + parseInt( recsCnt ) );
      }
   }
}