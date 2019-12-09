/************************************************************************
@Description:   seqDB-6655:数据未压缩，更新记录达到压缩条件_st.compress.03.014
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert({atest:i,btest:i,ctest:"test"+i,dtest:""}) ;
         3 update({$set:{dtest:"abcdefg890abcdefg890abcdefg890"}) ;
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

      updateRecs( noCL, noCSName, noCLName );
      updateRecs( lzwCL, lzwCSName, lzwCLName );

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
         doc.push( { atest: i, btest: i, ctest: "test" + i, dtest: "" } )
      };
      cl.insert( doc );
   }
}

function updateRecs ( cl, csName, clName )
{
   println( "\n---Begin to update records, CL[" + csName + "." + clName + "]" );

   cl.update( { $set: { dtest: "abcdefg890abcdefg890abcdefg890" } } );
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