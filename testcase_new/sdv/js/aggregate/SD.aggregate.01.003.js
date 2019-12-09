/************************************************************************
*@Description:  seqDB-1950:$limit+$skip+$limit+$skip组合查询_SD.aggregate.01.003
*     多个$limit做合并后取最小值，多个$skip合并取和，如：$limit(4)+$skip(1)+$limit(5)+$skip(2)=$limit(4)+$skip(3)。
*@Author:  huangxiaoni  2015/10/10
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

   cl.insert( { no: 4, score: 77 } );
   cl.insert( { no: 1, score: 80.5 } );
   cl.insert( { no: 3, score: 98 } );
   cl.insert( { no: 2, score: 100 } );
   cl.insert( { no: 5, score: 90 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $sort: { no: 1 } }, { $limit: 4 }, { $skip: 1 }, { $limit: 5 }, { $skip: 2 } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var no = rc.current().toObj()["no"];
   var score = rc.current().toObj()["score"];
   //expect results:{no:4,score:77}
   var expNo = 4;
   var expScore = 77;
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