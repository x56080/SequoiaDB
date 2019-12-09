/************************************************************************
*@Description:  seqDB-1957:$project+$sort组合查询,$project中field1字段值为0,$sort按field1排序_SD.aggregate.01.010
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
      cl.aggregate( { $project: { no: 0 } }, { $sort: { no: 1 } } );
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