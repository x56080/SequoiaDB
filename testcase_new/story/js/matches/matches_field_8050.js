/************************************************************************
*@Description:   seqDB-8050: 使用$field查询，字段不存在 
                    cover all data type
*@Author:  2016/5/25  xiaoni huang
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_matches8050";
      var cl = readyCL( clName );

      var rawData = insertRecs( cl );

      var findRecsArray = findRecs( cl );
      checkResult( findRecsArray );

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

   cl.insert( { a: 0, b: 0 } );
}

function findRecs ( cl )
{
   println( "\n---Begin to find records." );

   var rc = cl.find( { "test": { $field: "world" } }, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var findRecsArray = [];
   while( tmpRecs = rc.next() )
   {
      findRecsArray.push( tmpRecs.toObj() );
   }
   return findRecsArray;
}

function checkResult ( findRecsArray )
{
   println( "\n---Begin to check result." );

   var expLen = 0;
   if( findRecsArray.length !== expLen )   //return size after find by type
   {
      throw buildException( "checkResult", null, "[compare number]",
         "[recsNum:" + expLen + "]",
         "[recsNum:" + findRecsArray.length + "]" );
   }
}