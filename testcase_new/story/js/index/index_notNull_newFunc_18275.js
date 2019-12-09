/******************************************************************************
*@Description : seqDB-18275:新增接口创建索引，覆盖所有参数 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName = "cl_18275";
   var indexName = "idx";

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
      "Failed to create CL." );

   /**************************** test1, cover: all param ***************************/
   println( "\n---Begin to create index." );
   var unique = false;
   var enforced = false
   var NotNull = false;
   var options = { Unique: unique, Enforced: enforced, NotNull: NotNull, SortBufferSize: 32 };
   cl.createIndex( indexName, { a: 1 }, options );

   println( "---Begin to check results." );
   checkIndex( cl, indexName, unique, enforced, NotNull );

   // clean index
   cl.dropIndex( indexName );


   /**************************** test2, SortBufferSize:0 ***************************/
   println( "\n---Begin to create index[SortBufferSize:0]." );
   var unique = true;
   var enforced = true;
   var NotNull = true;
   var options = { Unique: unique, Enforced: enforced, NotNull: NotNull, SortBufferSize: 32 };
   cl.createIndex( indexName, { a: 1 }, options );

   println( "---Begin to check results." );
   checkIndex( cl, indexName, unique, enforced, NotNull );

   // clean index
   cl.dropIndex( indexName );


   /**************************** test3, SortBufferSize < 0 ***************************/
   println( "\n---Begin to create index[SortBufferSize:-1]." );
   try 
   {
      var options = { SortBufferSize: -1 };
      cl.createIndex( indexName, { a: 1 }, options );
   }
   catch( e )
   {
      if( e !== -6 )
      {
         throw buildException( "checkResult", null, "", -6, "  " + e );
      }
   }

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expUni, expEnf, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actUni = indexDef.unique;
   var actEnf = indexDef.enforced;
   var actNot = indexDef.NotNull;
   if( actUni !== expUni || actEnf !== expEnf || actNot !== expNot )
   {
      var expResults = JSON.stringify( { unique: expUni, enforced: expEnf, NotNull: expNot } );
      var actResults = JSON.stringify( { unique: actUni, enforced: actEnf, NotNull: actNot } );
      throw buildException( "checkResult", null, "", expResults, "  " + actResults );
   }
}