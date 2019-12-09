/************************************************************************
*@Description:  	seqDB-1974:$aggregate使用数组查询_SD.aggregate.01.027
*@Author:  2015/10/21  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre"

      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = aggreOper( cl );
      checkResult( rc );

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

   cl.insert( { a: { b: [1, 2] } } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $project: { b: "$a.b.$[0]" } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );
   //compare the returned records
   var actB = rc.current().toObj()["b"];
   //expect results:{b:1}
   var expB = 1;
   if( actB !== expB )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[actB:" + expB + "]",
         "[actB:" + actB + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}