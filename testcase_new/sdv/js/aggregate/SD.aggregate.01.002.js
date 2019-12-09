/************************************************************************
*@Description:  seqDB-1949:$sort+$skip+$limit组合查询_SD.aggregate.01.002
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

   cl.insert( { no: 4, score: 80.5 } );
   cl.insert( { no: 2, score: 89 } );
   cl.insert( { no: 1, score: 98 } );
   cl.insert( { no: 3, score: 100 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $sort: { no: 1 } }, { $skip: 1 }, { $limit: 2 } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var no1 = rc.current().toObj()["no"];
   var no2 = rc.next().toObj()["no"];
   //expect results:{no:2, score:89},{no:3,score:100}
   var expNo1 = 2;
   var expNo2 = 3;
   if( no1 != expNo1 || no2 != expNo2 )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[no1:" + expNo1 + ",no2:" + expNo2 + "]",
         "[no1:" + no1 + ",no2:" + no2 + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}