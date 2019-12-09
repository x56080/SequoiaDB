/****************************************************
@description: seqDB-11201:query.hint.count查询
@author:
              2017-3-7 huangxiaoni init
****************************************************/
main();

function main ()
{
   try
   {
      db.setSessionAttr( { PreferedInstance: "M" } );

      var clName = COMMCLNAME + "_11201";
      var idxName = "idx";

      var cl = readyCL( clName );
      cl.createIndex( idxName, { a: -1 } );
      insertRecs( cl );
      queryRecs( cl, idxName );  //query and check result

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   for( i = 0; i < 50; i++ )
   {
      cl.insert( { a: i, b: i } );
   }
}

function queryRecs ( cl, idxName )
{
   println( "\n---Begin to exec[query.hint.count]." );

   var cnt1 = cl.find( { b: { $gte: 10 } } ).hint( { "": "" } ).sort( { a: 1 } ).skip( 10 ).limit( 20 ).count();
   var cnt2 = cl.find( { b: { $gte: 10 } } ).hint( { "": idxName } ).sort( { a: 1 } ).skip( 10 ).limit( 20 ).count();

   //check result
   var expCnt = 40;
   var actCnt1 = Number( cnt1 );
   var actCnt2 = Number( cnt2 );
   if( expCnt !== actCnt1 && actCnt1 !== actCnt2 )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[count1:" + expCnt + ", count2:" + expCnt + "]",
         "[count1:" + actCnt1 + ", count2:" + actCnt2 + "]" );
   }

}