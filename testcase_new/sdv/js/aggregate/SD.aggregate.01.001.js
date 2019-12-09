/************************************************************************
*@Description:  seqDB-1948:$sort+$match组合查询_SD.aggregate.01.001
*@Author:  2015/10/10  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre";

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

   cl.insert( { no: 1, score: 80.5 } );
   cl.insert( { no: 3, score: 89 } );
   cl.insert( { no: 2, score: 98 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $sort: { no: 1 } }, { $match: { score: { $gt: 90 } } } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var no = rc.current().toObj()["no"];
   var score = rc.current().toObj()["score"];
   //expect result: {no:2,score:98}
   var expNo = 2;
   var expScore = 98;
   if( no !== expNo || score !== expScore )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[no:" + expNo + ",score:" + expScore + "]",
         "[no:" + no + ",score:" + score + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}