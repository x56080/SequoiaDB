/************************************************************************
*@Description:  $group+$match组合查询,$match中field1字段在$group中不存在_SD.aggregate.01.011
*@Author:  2015/11/2  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_aggre"

      var cl = readyCL( clName );

      insertRecs( cl );
      aggreOper( cl );  //aggregate and check result

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

   cl.insert( { no: 2, score: 60, name: "Tom", age: 12 } );
   cl.insert( { no: 1, score: 70, name: "Json", age: 13 } );
}

function aggreOper ( cl )
{
   println( "\n---Begin to aggregate records." );

   try
   {
      cl.aggregate( { $group: { _id: "$name", avg_score: { $avg: "$score" } } },
         { $match: { score: 80 } } );
   }
   catch( e )
   {
      //check result  //e:-6
      var expectE = -6;
      if( e !== expectE )
      {
         throw buildException( "checkResult", e, '{$project:{no:"test"}}',
            "[e:" + expectE + "]", "[e:" + e + "]" );
      }
   }
}