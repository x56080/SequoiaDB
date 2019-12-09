/************************************************************************
*@Description:  seqDB-1954:使用$sort+$group+$skip+$limit+$match+$project组合查询_SD.aggregate.01.007
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

   cl.insert( { no: 6, score: 80, name: "Tom", age: 12 } );
   cl.insert( { no: 2, score: 80, name: "Json", age: 13 } );
   cl.insert( { no: 4, score: 80, name: "Canace", age: 12 } );
   cl.insert( { no: 1, score: 92, name: "Baby", age: 12 } );
   cl.insert( { no: 3, score: 80, name: "Aber", age: 15 } );
   cl.insert( { no: 5, score: 80, name: "Tom" } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $sort: { no: 1 } },
      { $group: { _id: "$name" } }, { $skip: 1 }, { $limit: 3 },
      { $match: { score: 80 } }, { $project: { score: 1, name: 1 } } );
   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var score1 = rc.current().toObj()["score"];
   var name1 = rc.current().toObj()["name"];
   var score2 = rc.next().toObj()["score"];
   var name2 = rc.current().toObj()["name"];
   //expect results:{score:80,name:"Canace"},{score:80,name:"Json"}
   var expScore1 = 80;
   var expName1 = "Canace";
   var expScore2 = 80;
   var expName2 = "Json";
   if( score1 !== expScore1 || name1 !== expName1
      || score2 !== expScore2 || name2 !== expName2 )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[score1:" + expScore1 + ",name1:" + expName2
         + ",score2:" + expScore2 + ",name2:" + expName2 + "]",
         "[score1:" + score1 + ",name1:" + name1
         + ",score2:" + score2 + ",name2:" + name2 + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}