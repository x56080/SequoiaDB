/******************************************************************************
*@Description : seqDB-18277:创建单键索引，指定NotNull，创建索引前后插入数据 
*@Author      : 2019-4-29  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName = "cl_18277";
   var indexName = "idx";
   var validRecs1 = [{ a: 1, b: 1 }];
   var validRecs2 = [{ a: 1, b: 1 }, { b: 2 }, { a: null, b: 3 }]; // a contain: not exist, null 
   var invRecs = [{ b: 2 }, { a: null, b: 3 }];

   // ready cl
   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false,
      "Failed to create CL." );

   /**************************** test1, create index[ NotNull:true ] -> insert ***************************/
   var NotNull = true;
   println( "\n---Test1, create index[ NotNull:" + NotNull + " ] -> insert." );
   cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );

   var valRecs = validRecs1;
   cl.insert( valRecs );
   for( i = 0; i < invRecs.length; i++ ) 
   {
      try
      {
         cl.insert( invRecs[i] );
      }
      catch( e ) 
      {
         if( e !== -339 )
         {
            throw buildException( "checkResult", null, "", -339, "  " + e );
         }
      }
   }

   checkIndex( cl, indexName, NotNull );
   checkRecords( cl, valRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test2, create index[ NotNull:false ] -> insert ***************************/
   var NotNull = false;
   println( "\n---Test2, create index[ NotNull:" + NotNull + " ] -> insert." );
   cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );

   var valRecs = validRecs2;
   cl.insert( valRecs );

   checkIndex( cl, indexName, NotNull );
   checkRecords( cl, valRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test3, insert -> create index[ NotNull:true ]  ***************************/
   var NotNull = true;
   println( "\n---Test3, insert -> create index[ NotNull:" + NotNull + " ]." );

   var valRecs = validRecs2;
   cl.insert( valRecs );

   // create index
   try
   {
      cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );
   }
   catch( e ) 
   {
      if( e !== -339 )
      {
         throw buildException( "checkResult", null, "", -339, "  " + e );
      }
   }

   // check results
   checkRecords( cl, valRecs );

   // clean index
   cl.remove();


   /**************************** test4, insert -> create index[ NotNull:false ]  ***************************/
   var NotNull = false;
   println( "\n---Test4, insert -> create index[ NotNull:" + NotNull + " ]." );

   var valRecs = validRecs2;
   cl.insert( valRecs );
   cl.createIndex( indexName, { a: 1 }, { NotNull: NotNull } );
   checkRecords( cl, valRecs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expNot ) 
{
   var indexDef = cl.getIndex( indexName ).toObj().IndexDef;
   var actNot = indexDef.NotNull;
   if( actNot !== expNot )
   {
      throw buildException( "checkResult", null, "", expNot, "  " + actNot );
   }
}

function checkRecords ( cl, expRecs ) 
{
   println( "---Check results." );
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { b: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }

   if( JSON.stringify( expRecs ) !== JSON.stringify( actRecs ) )
   {
      throw buildException( "checkResult", null, "", JSON.stringify( expRecs ), "  " + JSON.stringify( actRecs ) );
   }
}