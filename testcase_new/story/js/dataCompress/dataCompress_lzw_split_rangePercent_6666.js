/************************************************************************
@Description:     seqDB-6666:range百分比切分，分区键为多个字段_st.compress.03.025
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert({INNER_NO:i,SA_ACCT_NO:i,EVT_ID:"lwy20120702"+i,IVC_NAME: "电子银行业务回单(付款)",OPEN_BRANCH_NAME:"中国民生银行福州闽江支行"}) ;
         3 split( rgName, targetRgName, 50 ) ;
         4 check attribute of CL[lzw];
           check records, get random records, then compare the records;
           check compressed rate. 
@output:   successfull
@Author:   
           2016/3/23   XiaoNi Huang init
@remarks:  alter cl, Failed to check attribute, jira: 1628
************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   db.setSessionAttr( { PreferedInstance: "M" } );

   var noCSName = COMMCSNAME + "_no";
   var lzwCSName = COMMCSNAME + "_lzw";
   var noCLName = COMMCLNAME + "_no";
   var lzwCLName = COMMCLNAME + "_lzw";
   var rgName = getDataGroupsName()[0];
   var rgName2 = getDataGroupsName()[1];
   var insertRecsNum = 800000;
   var checkRecsNum = 3; //get random 3 records

   commDropCS( db, noCSName, true, "Failed to drop CS[" + noCSName + "]." );
   commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

   commCreateCS( db, noCSName, false, "Failed to create CS[" + noCSName + "]." );
   commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );

   var noCL = createCL( noCSName, noCLName, rgName, false );
   var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );
   alterCL( noCL, noCSName, noCLName );
   alterCL( lzwCL, lzwCSName, lzwCLName );

   insertRecs( noCL, noCSName, noCLName, insertRecsNum );
   insertRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum );

   splitRecs( noCL, noCSName, noCLName, rgName, rgName2 );
   splitRecs( lzwCL, lzwCSName, lzwCLName, rgName, rgName2 );

   //checkAttributeOfCL( lzwCSName, lzwCLName, true, "lzw" );
   checkRecs( lzwCL, lzwCSName, lzwCLName, insertRecsNum, checkRecsNum, rgName, rgName2 );
   checkCompressedRate( noCSName, lzwCSName );

   clearCS( db, noCSName );
   clearCS( db, lzwCSName );
}

function alterCL ( cl, csName, clName )
{

   cl.alter( { ShardingKey: { INNER_NO: 1 }, ShardingType: "range" } );
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

function splitRecs ( cl, csName, clName, rgName, rgName2 )
{

   cl.split( rgName, rgName2, 50 );
}

function checkRecs ( cl, csName, clName, insertRecsNum, checkRecsNum, rgName, rgName2 )
{

   //get random records, compare the records

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );

      var recsCnt = cl.find( { INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i, IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行" } ).count();
      var expctCnt = 1;
      assert.equal( recsCnt, expctCnt );
   }

   //check count of records in the groups
   var groups = new Array;
   var groups = [rgName, rgName2];
   var tmpCnt = new Array;
   var totalCnt = 0;
   for( i = 0; i < groups.length; i++ )
   {
      var node = db.getRG( groups[i] ).getMaster().toString();
      var nodeDB = new Sdb( node );
      var recsCnt = nodeDB.getCS( csName ).getCL( clName ).count();
      tmpCnt.push( parseInt( recsCnt ) );
      totalCnt = tmpCnt[i] + totalCnt;

      nodeDB.close();
   }

   assert.equal( totalCnt, insertRecsNum );

}