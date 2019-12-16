/******************************************************************************
*@Description : seqDB-18281:options参数校验
*@Author      : 2019-5-6  XiaoNi Zhao
******************************************************************************/
main();
function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "\nThe mode is standalone." );
      return;
   }

   var mainClName = "mainCl_18281_2";
   var subClName = "subCl_18281_2";
   var indexName = "idx";

   commDropCL( db, COMMCSNAME, mainClName );
   commDropCL( db, COMMCSNAME, subClName );

   var mainCl = commCreateCL( db, COMMCSNAME, mainClName, { IsMainCL: true, ShardingKey: { a: 1 } } );
   var subCl = commCreateCL( db, COMMCSNAME, subClName, { ShardingKey: { a: 1 } } );
   var fullName = COMMCSNAME + "." + subClName;
   mainCl.attachCL( fullName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   /**************************** test1, unique:0, enforced:0, NotNull:0 ***************************/
   println( "\n---Test1, create index, unique:0, enforced:0,NotNull:0." );
   mainCl.createIndex( indexName, { b: 1, c: 1 }, { unique: 0, enforced: 0, NotNull: 0 } );
   println( "---Check results." );
   checkIndex( mainCl, indexName, false, false, false );
   var insertR1s = [{ a: 1 }, { a: 1 }, { a: 1, b: 1, c: 1 }, { a: 1, b: 1, c: 1 }];
   mainCl.insert( insertR1s );
   checkRecords( mainCl, insertR1s );
   mainCl.dropIndex( indexName );
   mainCl.remove();


   /**************************** test2, unique:1, enforced:1 ***************************/
   println( "\n---Test2, create index, unique:1, enforced:1, NotNull:1." );
   mainCl.createIndex( indexName, { a: 1, b: 1, c: 1 }, { unique: 1, enforced: 1, NotNull: 1 } );
   println( "---Check results." );
   checkIndex( mainCl, indexName, true, true, true );

   var insertR1 = [{ a: 1, b: 1, c: 1 }];
   mainCl.insert( insertR1 );
   try
   {
      mainCl.insert( insertR1 );
      throw "insert error1!";
   }
   catch( e ) 
   {
      if( e !== -38 )
      {
         throw e;
      }
   }

   var insertR2 = [{ a: 1, c: 1 }];
   try
   {
      mainCl.insert( insertR2 );
      throw "insert error!";
   } catch( e )
   {
      if( e != -339 )
      {
         throw e;
      }
   }
   checkRecords( mainCl, insertR1 );
   mainCl.dropIndex( indexName );
   mainCl.remove();

   commDropCL( db, COMMCSNAME, mainClName, false, false, "Failed to drop mainCl in the end-condition" );
}

function checkIndex ( mainCl, indexName, expUni, expEnf, expNot ) 
{
   if( expUni == undefined ) { expUni = false };
   if( expEnf == undefined ) { expEnf = false };
   if( expNot == undefined ) { expNot = false };

   var indexDef = mainCl.getIndex( indexName ).toObj().IndexDef;
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