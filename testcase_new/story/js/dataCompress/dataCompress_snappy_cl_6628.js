/************************************************************************
@Description:   seqDB-6628:开启压缩，创建CL_st.compress.02.001
@input:    
         1 create CL[Compressed: true, CompressionType: "snappy"] ;
         2 insert/update/query/remove ;
         3 check attribute of CL[lzw];
           check records, get random records, then compare the records;
@output:   successfull
@Author:   
           2016/3/23   XiaoNi Huang init
************************************************************************/
main( test );

function test ()
{
   var csName = COMMCSNAME + "_snappy";
   var clName = COMMCLNAME + "_snappy";
   var rgName = getDataGroupsName()[0];
   var insertRecsNum = 20000;
   var checkRecsNum = 1; //get random 3 records

   commDropCS( db, csName, true, "Failed to drop CS[" + csName + "]." );

   commCreateCS( db, csName, false, "Failed to create CS[" + csName + "]." );

   var cl = createCL( csName, clName, rgName, true, "snappy" );
   checkAttributeOfCL( csName, clName, true, "snappy" );

   insertRecs( cl, csName, clName, insertRecsNum );
   checkRecs( cl, insertRecsNum, checkRecsNum, "insert" );

   updateRecs( cl, csName, clName );
   checkRecs( cl, insertRecsNum, checkRecsNum, "update" );

   findAndRemoveRecs( cl, csName, clName );
   checkRecs( cl, insertRecsNum, checkRecsNum, "findAndRemove" );

   clearCS( db, csName );
}

function insertRecs ( cl, csName, clName, insertRecsNum )
{

   for( k = 0; k < insertRecsNum; k += 10000 )
   {
      var doc = [];
      for( i = 0 + k; i < 10000 + k; i++ )
      {
         doc.push( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } )
      };
      cl.insert( doc );
   }
}

function updateRecs ( cl, csName, clName )
{

   cl.update( { $set: { dtest: "电子银行业务回单(付款)" } } );
}

function findAndRemoveRecs ( cl, csName, clName )
{

   var rc = cl.find( { $and: [{ atest: { $lt: 10000 } }, { dtest: { $et: "电子银行业务回单(付款)" } }] } ).remove();
   while( rc.next() );
}

function checkRecs ( cl, insertRecsNum, checkRecsNum, oper )
{

   //get random records, compare the records
   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );

      if( oper == "insert" )
      {
         var recsCnt = cl.find( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } ).count();
         var expctCnt = 1;
      }
      else if( oper == "update" )
      {
         var recsCnt = cl.find( { atest: i, btest: i, ctest: "test" + i, dtest: "电子银行业务回单(付款)" } ).count();
         var expctCnt = 1;
      }
      else if( oper == "findAndRemove" )
      {
         var recsCnt = cl.find( { atest: i, btest: i, ctest: "test" + i, dtest: "电子银行业务回单(付款)" } ).count();
         if( i < 10000 )
         {
            var expctCnt = 0;
         }
         else
         {
            var expctCnt = 1;
         }
      }

      assert.equal( recsCnt, expctCnt );
   }

   //check total count
   var totalCnt = cl.count();
   if( oper == "insert" || oper == "update" )
   {
      var expctCnt = insertRecsNum;
   }
   else if( oper == "findAndRemove" )
   {
      var expctCnt = insertRecsNum - 10000;
   }

   assert.equal( totalCnt, expctCnt );

}