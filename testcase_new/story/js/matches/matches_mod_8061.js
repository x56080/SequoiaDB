/************************************************************************
*@Description:  seqDB-8061:使用$mod查询，被除数为小于1的小数 
*@Author:  2016/5/20  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8061";
      var cl = readyCL( clName );

      insertRecs( cl );
      var rc = findRecs( cl, 0.6, 1 );  //find and check result

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
   cl.insert( [{ a: 3 }] );
}

function findRecs ( cl, div, rem )
{
   println( "\n---Begin to find records by matches[$mod]." );

   try
   {
      cl.find( { a: { $mod: [div, rem] } } );
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