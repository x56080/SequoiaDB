/************************************************************************
*@Description:   使用$type查询，value取无效编码
*@Author:  2016/5/23  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8093";
      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = findRecs( cl );  //find and check result

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

   cl.insert( { a: "test" } );
}

function findRecs ( cl )
{
   println( "\n---Begin to find records by matches[$type]." );

   try
   {
      cl.find( { a: { $type: 1, $et: 6 } } );
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