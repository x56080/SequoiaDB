/************************************************************************
*@Description:  seqDB-1966:$limit中指定的记录条数大于实际的记录总数_SD.aggregate.01.019
*@Author:  2015/10/21  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre"

      var cl = readyCL();

      insertRecs( cl );
      var rc = aggreOper( cl );
      checkResult( rc );

      cleanCL();
   }
   catch( e )
   {
      throw e;
   }
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { no: 1, score: 80 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $limit: 2 } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var no = rc.current().toObj()["no"];
   //expect results:{no:1, score:80}
   var expNo = 1;
   if( no !== expNo )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[no:" + expNo + "]",
         "[no:" + no + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}