/************************************************************************
*@Description:  	seqDB-1955:$group使用多个分组键查询_SD.aggregate.01.008
*@Author:  2015/11/3  huangxiaoni
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

   cl.insert( { no: 2, score: 80, name: "Tom", age: 12 } );
   cl.insert( { no: 1, score: 80, name: "Json", age: 13 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $group: { _id: "$name", avg_score: { $avg: "$score" }, firstName: { $first: "$name" } } } );
   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var avgScore1 = rc.current().toObj()["avg_score"];
   var firstName1 = rc.current().toObj()["firstName"];
   var avgScore2 = rc.next().toObj()["avg_score"];
   var firstName2 = rc.current().toObj()["firstName"];
   //expect results:{avg_score:80,firstNme:Json}, {avg_score:80,firstNme:Tom}
   var expScore1 = 80;
   var expName1 = "Json";
   var expScore2 = 80;
   var expName2 = "Tom";
   if( avgScore1 !== expScore1 || firstName1 !== expName1
      || avgScore2 !== expScore2 || firstName2 !== expName2 )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[avgScore1:" + expScore1 + ",firstName1:" + expName1
         + ",avgScore2:" + expScore2 + ",firstName2:" + expName2 + "]",
         "[avgScore1:" + avgScore1 + ",firstName1:" + firstName1
         + ",avgScore2:" + avgScore2 + ",firstName2:" + firstName2 + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}