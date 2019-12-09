/************************************************************************
@Description:   seqDB-6758:开启压缩，创建CL
                seqDB-6626:创建CL，不开压缩，对CL做CRUD 
                     (不开压缩，CRUD操作在其他其他用例都要覆盖，此用例只验证CL属性及插入数据正确性)
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert({atest:i,btest:i,ctest:"test"+i,dtest:"abcdefg890abcdefg890abcdefg890"}) ;
         3 check attribute of CL[no];
           check attribute of CL[lzw];
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
      var noCLName = COMMCLNAME + "_no";   //no compressed
      var lzwCLName = COMMCLNAME + "_lzw";  //CompressionType:"lzw"
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

      checkAttributeOfCL( noCSName, noCLName, false );
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
         doc.push( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } )
      };
      cl.insert( doc );
   }
}

function checkRecs ( cl, insertRecsNum, checkRecsNum )
{
   println( "\n---Begin to check Records. checkRecsNum: " + checkRecsNum );

   //get random records, compare the records
   println( '   recs: {atest:i,btest:i,ctest:"test"+i,dtest:"abcdefg890abcdefg890abcdefg890"}' );
   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );
      println( "   random i: " + i );
      var recsCnt = cl.find( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } ).count();
      var expctCnt = 1;
      if( parseInt( recsCnt ) !== expctCnt )
      {
         throw buildException( "Failed to check Records.", null, "[checkRecords]",
            "recsCnt: " + expctCnt, "recsCnt: " + parseInt( recsCnt ) );
      }
   }
}