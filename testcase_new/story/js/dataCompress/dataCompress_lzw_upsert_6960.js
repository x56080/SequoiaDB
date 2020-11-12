/************************************************************************
@Description:   seqDB-6960:upsert指定更新符删除已压缩记录中字段_st.compress.07.003
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert({atest:i,btest:i,ctest:"test"+i,dtest:""}) ;
         3 upsert({$unset:{dtest:"abcdefg890abcdefg890abcdefg890"}) ;
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

   upsertRecs( noCL, noCSName, noCLName );
   upsertRecs( lzwCL, lzwCSName, lzwCLName );

   checkRecs( lzwCL, insertRecsNum, checkRecsNum );
   checkNodeCnt( lzwCSName, lzwCLName, rgName, insertRecsNum );
   checkCompressedRate( noCSName, lzwCSName );

   commDropCS( db, noCSName );
   commDropCS( db, lzwCSName );
}

function insertRecs ( cl, csName, clName, insertRecsNum )
{

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

function upsertRecs ( cl, csName, clName )
{

   cl.upsert( { $unset: { dtest: "abcdefg890abcdefg890abcdefg890" } } );
}

function checkRecs ( cl, insertRecsNum, checkRecsNum )
{

   //get random records, compare the records
   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );
      var recsCnt1 = cl.find( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } ).count();
      var recsCnt2 = cl.find( { atest: i, btest: i, ctest: "test" + i } ).count();
      var expctCnt1 = 0;
      var expctCnt2 = 1;
      if( parseInt( recsCnt1 ) !== expctCnt1 || parseInt( recsCnt2 ) !== expctCnt2 )
      {
         throw new Error( "Failed to check Records. fail,[checkRecords]" +
            "recsCnt1: " + expctCnt1 + ", recsCnt2: " + expctCnt2 +
            "recsCnt1: " + parseInt( recsCnt1 ) + ", recsCnt2: " + parseInt( recsCnt2 ) );
      }
   }
}