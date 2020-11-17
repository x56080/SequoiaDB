/******************************************************************************
*@Description : seqDB-18281:options参数校验
*@Author      : 2019-5-6  XiaoNi Huang
******************************************************************************/


main( test );

function test ()
{
   var clName = "cl_18281_1";
   var indexName = "idx";

   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false );

   /**************************** test1, field name lowercase ***************************/
   cl.createIndex( indexName, { a: 1 }, { unique: true, enforced: true } );

   checkIndex( cl, indexName, true, true, false );

   cl.dropIndex( indexName );


   /**************************** test2, field name invalid ***************************/
   var keyArr = [{ isUnique: true }, { enforced: true }, { sortBufferSize: true }, { notNull: true }, { aa: true }];
   for( i = 0; i < keyArr.length; i++ ) 
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         cl.createIndex( indexName, { a: 1 }, keyArr[i] );
      } );
   }

   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      cl.getIndex( indexName );
   } );


   /**************************** test3, default value ***************************/
   cl.createIndex( indexName, { a: 1 } );

   checkIndex( cl, indexName, false, false, false );

   // clean index
   cl.dropIndex( indexName );


   /**************************** test4, 2 diff name for same field ***************************/
   var keyArr = [{ enforced: true, Enforced: false }, { unique: false, Unique: false }, { NotNull: true, aa: false }];
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, { a: 1 }, keyArr[i] );
   } );


   /**************************** test5, boolean:0 ***************************/
   cl.createIndex( indexName, { a: 1 }, { unique: 0, enforced: 0, NotNull: 0 } );
   checkIndex( cl, indexName, false, false, false );

   var recs = [{ a: 1, b: 1 }, { b: 2 }, { a: null, b: 3 }, { a: 1, b: 4 }];
   cl.insert( recs );
   checkRecords( cl, recs );

   // clean index
   cl.dropIndex( indexName );
   cl.remove();


   /**************************** test6, unique:1, enforced:1,NotNull:1 ***************************/
   cl.createIndex( indexName, { a: 1 }, { unique: 1, enforced: 1, NotNull: 1 } );
   checkIndex( cl, indexName, true, true, true );

   var valRecs = [{ a: 1, b: 1 }];
   var invRecs = [{ b: 2 }, { a: null, b: 3 }];
   cl.insert( valRecs );
   for( i = 0; i < invRecs.length; i++ ) 
   {
      assert.tryThrow( SDB_IXM_KEY_NOTNULL, function()
      {
         cl.insert( invRecs[i] );
      } );
   }
   checkRecords( cl, valRecs );

   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( { a: 1, b: 4 } );
   } );
   cl.dropIndex( indexName );
   cl.remove();

   /**************************** test7, unique:1, enforced:1 ***************************/
   cl.createIndex( indexName, { a: 1 }, { unique: 1, enforced: 1 } );
   checkIndex( cl, indexName, true, true );
   var insertR1 = [{ b: 1 }];
   cl.insert( insertR1 );
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( [{ b: 2 }] );
   } );
   checkRecords( cl, insertR1 );
   cl.dropIndex( indexName );
   cl.remove();

   /**************************** test8, unique:0, enforced:0,NotNull:0 ***************************/
   cl.createIndex( indexName, { a: 1 }, { unique: 0, enforced: 0, NotNull: 0 } );
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
   var keyArr = [{ NotNull: "true" }, { NotNull: "false" }, { NotNull: 2 }];

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.createIndex( indexName, { a: 1 }, keyArr[i] );
   } );

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
      throw new Error( "checkResult fail,", expResults, "  " + actResults );
   }
}

function checkRecords ( cl, expRecs ) 
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { b: 1 } );
   var actRecs = new Array();
   while( tmpRecs = rc.next() )
   {
      actRecs.push( tmpRecs.toObj() );
   }

   assert.equal( expRecs, actRecs );
}