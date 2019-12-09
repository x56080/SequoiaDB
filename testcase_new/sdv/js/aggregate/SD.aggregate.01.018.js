/************************************************************************
*@Description:  seqDB-1965:$project中字段值为非法_SD.aggregate.01.018
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
      aggreOper( cl );  //aggregate and check result

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

   try
   {
      cl.aggregate( { $project: { no: "test" } } );
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