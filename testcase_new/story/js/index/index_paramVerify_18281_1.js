/******************************************************************************
*@Description : seqDB-18281:options参数校验
*@Author      : 2019-5-6  XiaoNi Huang
******************************************************************************/

main();
function main ()
{
   var clName = "cl_18281_1";
   var indexName = "idx";

   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false,
      "Failed to create CL." );

   /**************************** test1, field name lowercase ***************************/
   println( "\n---Test1, create index, field name lowercase." );
   cl.createIndex( indexName, { a: 1 }, { unique: true, enforced: true } );

   println( "---Check results." );
   checkIndex( cl, indexName, true, true, false );

   cl.dropIndex( indexName );


   /**************************** test2, field name invalid ***************************/
   println( "\n---Test2, create index, field name invalid." );
   var keyArr = [{ isUnique: true }, { enforced: true }, { sortBufferSize: true }, { notNull: true }, { aa: true }];
   for( i = 0; i < keyArr.length; i++ ) 
   {
      try
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      }
      catch( e ) 
      {
         if( e !== -6 )
         {
            throw buildException( "checkResult", null, "", -6, "  " + e );
         }
      }
   }

   try 
   {
      cl.getIndex( indexName );
   }
   catch( e ) 
   {
      if( e !== -47 )
      {
         throw buildException( "checkResult", null, "", -47, "  " + e );
      }
   }


   /**************************** test3, default value ***************************/
   println( "\n---Test3, create index, default value." );
   cl.createIndex( indexName, { a: 1 } );

   println( "---Check results." );
   checkIndex( cl, indexName, false, false, false );

   // clean index
   cl.dropIndex( indexName );


   /**************************** test4, 2 diff name for same field ***************************/
   println( "\n---Test3, create index, 2 diff name for same field." );
   var keyArr = [{ enforced: true, Enforced: false }, { unique: false, Unique: false }, { NotNull: true, aa: false }];
   try
   {
      cl.createIndex( indexName, { a: 1 }, keyArr[i] );
   }
   catch( e ) 
   {
      if( e !== -6 )
      {
         throw buildException( "checkResult", null, "", -6, "  " + e );
      }
   }


   /**************************** test5, boolean:0 ***************************/
   println( "\n---Test5, create index, boolean:0." );
   cl.createIndex( indexName, { a: 1 }, { unique: 0, enforced: 0, NotNull: 0 } );
   println( "---Check results." );
   checkIndex( cl, indexName, false, false, false );

   var recs = [{ a: 1, b: 1 }, { b: 2 }, { a: null, b: 3 }, { a: 1, b: 4 }];
   cl.insert( recs );
   checkRecords( cl, recs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test6, unique:1, enforced:1,NotNull:1 ***************************/
   println( "\n---Test6, create index, unique:1, enforced:1,NotNull:1." );
   cl.createIndex( indexName, { a: 1 }, { unique: 1, enforced: 1, NotNull: 1 } );
   println( "---Check results." );
   checkIndex( cl, indexName, true, true, true );

   var valRecs = [{ a: 1, b: 1 }];
   var invRecs = [{ b: 2 }, { a: null, b: 3 }];
   cl.insert( valRecs );
   for( i = 0; i < invRecs.length; i++ ) 
   {
      try
      {
         cl.insert( invRecs[i] );
         throw "insert error!";
      }
      catch( e ) 
      {
         if( e !== -339 )
         {
            throw e;
         }
      }
   }
   checkRecords( cl, valRecs );

   try
   {
      cl.insert( { a: 1, b: 4 } );
   }
   catch( e ) 
   {
      if( e !== -38 )
      {
         throw e;
      }
   }
   cl.dropIndex( indexName );
   cl.remove();

   /**************************** test7, unique:1, enforced:1 ***************************/
   println( "\n---Test7, create index, unique:1, enforced:1." );
   cl.createIndex( indexName, { a: 1 }, { unique: 1, enforced: 1 } );
   println( "---Check results." );
   checkIndex( cl, indexName, true, true );
   var insertR1 = [{ b: 1 }];
   cl.insert( insertR1 );
   try
   {
      cl.insert( [{ b: 2 }] );
   } catch( e )
   {
      if( e != -38 )
      {
         throw e;
      }
   }
   checkRecords( cl, insertR1 );
   cl.dropIndex( indexName );
   cl.remove();

   /**************************** test8, unique:0, enforced:0,NotNull:0 ***************************/
   println( "\n---Test8, create index, unique:0, enforced:0,NotNull:0." );
   cl.createIndex( indexName, { a: 1 }, { unique: 0, enforced: 0, NotNull: 0 } );
   println( "---Check results." );
   checkIndex( cl, indexName, false, false, false );

   var insertR1s = [{ a: 1, b: 1 }, { a: 1, b: 2 }, { b: 3 }, { b: 4 }, { a: null, b: 5 }];
   for( i = 0; i < insertR1s.length; i++ ) 
   {
      cl.insert( insertR1s[i] );
   }
   checkRecords( cl, insertR1s );
   cl.dropIndex( indexName );
   cl.remove();

   /**************************** test9, NotNull:string/otherNum ***************************/
   println( "\n---Test9, create index, NotNull:string/otherNum." );
   var keyArr = [{ NotNull: "true" }, { NotNull: "false" }, { NotNull: 2 }];
   try
   {
      cl.createIndex( indexName, { a: 1 }, keyArr[i] );
   }
   catch( e ) 
   {
      if( e !== -6 )
      {
         throw buildException( "checkResult", null, "", -6, "  " + e );
      }
   }

   // clean env
   commDropCL( db, COMMCSNAME, clName, false, false, "Failed to drop CL in the end-condition" );
}

function checkIndex ( cl, indexName, expUni, expEnf, expNot ) 
{
   if( expUni == undefined ) { expUni = false };
   if( expEnf == undefined ) { expEnf = false };
   if( expNot == undefined ) { expNot = false };

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

function checkRecords ( cl, expRecs ) 
{
   println( "   Check records." );
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