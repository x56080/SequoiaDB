/************************************************************************
*@Description:  $project+$match+$limit+$skip+$group+$sort组合查询_SD.aggregate.01.006
*@Author:  2015/10/10  huangxiaoni
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
   cl.insert( { no: 4, score: 80, name: "Tom", age: 12 } );
   cl.insert( { no: 6, score: 92, name: "Tom", age: 12 } );
   cl.insert( { no: 3, score: 80, name: "Aber", age: 15 } );
   cl.insert( { no: 5, score: 80, name: "Tom", age: 13 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   var rc = cl.aggregate( { $sort: { no: 1 } },
      { $project: { no: 1, score: 1, name: 1 } }, { $match: { score: 80 } }, { $limit: 4 }, { $skip: 1 },
      { $group: { _id: "$name", count: { $count: "$name" }, name: { $first: "$name" } } } );
   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records
   var count1 = rc.current().toObj()["count"];
   var name1 = rc.current().toObj()["name"];
   var count2 = rc.next().toObj()["count"];
   var name2 = rc.current().toObj()["name"];
   //expect results: {count:1,name:"Aber"},{count:2,name:"Tom"}
   var expCount1 = 1;
   var expName1 = "Aber";
   var expCount2 = 2;
   var expName2 = "Tom";
   if( count1 != expCount1 || name1 != expName1 || count2 != expCount2 || name2 != expName2 )
   {
      throw buildException( "checkResult", null, "[compare the records]",
         "[count1:" + expCount1 + ",name1:" + expName1
         + ",count2:" + expCount2 + ",name2:" + expName2 + "]",
         "[count1:" + count1 + ",name1:" + name1
         + ",count2:" + count2 + ",name2:" + name2 + "]" );
   }

   //compare the number of records
   var nextRecs = rc.next();
   if( nextRecs !== undefined )
   {
      throw buildException( "checkResult", null, "[compare the number of records]",
         "[nextRecs:undefined]", "[nextRecs:" + nextRecs + "]" )
   }
}
