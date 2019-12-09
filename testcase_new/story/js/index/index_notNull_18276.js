/******************************************************************************
*@Description : seqDB-18276:新旧接口创建相同/不同索引 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName = "cl_18276";
   var indexName1 = "idx1";
   var indexName2 = "idx2";

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
      "Failed to create CL." );

   /**************************** test1, different index ***************************/
   println( "\n---Begin to create different index." );
   println( "---Begin to create idx1, and check results." );
   // old function
   var unique = false;
   var enforced = false;
   var sortBufferSize = 32;
   cl.createIndex( indexName1, { a: 1 }, unique, enforced, sortBufferSize );
   checkIndex( cl, indexName1, unique, enforced, NotNull );

   // new function
   println( "---Begin to create idx2, and check results." );
   var unique = true;
   var enforced = true;
   var NotNull = true;
   var options = { Unique: unique, Enforced: enforced, NotNull: NotNull, SortBufferSize: 128 };
   cl.createIndex( indexName2, { b: 1 }, options );
   checkIndex( cl, indexName2, unique, enforced, NotNull );

   // clean index
   cl.dropIndex( indexName1 );
   cl.dropIndex( indexName2 );


   /**************************** test2, -247 ***************************/

   println( "\n---Begin to create index, e: -247." );
   cl.createIndex( indexName1, { a: 1 }, true );
   try 
   {
      cl.createIndex( indexName1, { a: 1 }, { Unique: true } );
   }
   catch( e )
   {
      if( e !== -247 )
      {
         throw buildException( "checkResult", null, "", -6, "  " + e );
      }
   }

   // clean index
   cl.dropIndex( indexName1 );


   /**************************** test3, -46 ***************************/
   println( "\n---Begin to create index, e: -46." );
   cl.createIndex( indexName1, { a: 1 }, true );
   try 
   {
      cl.createIndex( indexName1, { b: 1 }, { Unique: true } );
   }
   catch( e )
   {
      if( e !== -46 )
      {
         throw buildException( "checkResult", null, "", -46, "  " + e );
      }
   }

   // clean index
   cl.dropIndex( indexName1 );


   /**************************** test4, -291 ***************************/
   println( "\n---Begin to create index, e: -291." );
   cl.createIndex( indexName1, { a: 1 }, true );
   try 
   {
      cl.createIndex( indexName2, { a: 1 }, { Unique: true } );
   }
   catch( e )
   {
      if( e !== -291 )
      {
         throw buildException( "checkResult", null, "", -291, "  " + e );
      }
   }

   // clean index
   cl.dropIndex( indexName1 );

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expUni, expEnf, expNot ) 
{
   if( typeof ( expNot ) == "undefined" ) { expNot = false; }
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