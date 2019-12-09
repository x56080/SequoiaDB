/************************************************************************
*@Description:  seqDB-1967:$skip中指定的记录条数大于总记录数_SD.aggregate.01.020
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

   var rc = cl.aggregate( { $skip: 2 } );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   try
   {
      crtRecs = rc.current();
   }
   catch( e )
   {
      var expectE = -29;  //-29: End of collection
      if( e !== expectE )
      {
         throw buildException( "checkResult", null, "[compare the records]",
            "[e:" + expectE + "]", "[e:" + e + "]" );
      }
   }

}