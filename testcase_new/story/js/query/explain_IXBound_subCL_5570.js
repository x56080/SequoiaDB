/************************************************************************
*@Description:  seqDB-5570:主子表上使用访问计划，查询条件为空/仅为索引字段/不仅有索引字段_ST.explainAdd.02
*@Author:  2016/7/11  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      if( commIsStandalone( db ) )
      {
         println( " Deploy mode is standalone!" );
         return;
      }

      var mainCLName = COMMCLNAME + "_mcl_5570";
      var subCLName = COMMCLNAME + "_scl_5570";
      var idxName = CHANGEDPREFIX + "_idx";

      commDropCL( db, COMMCSNAME, mainCLName, true, true, "Failed to drop CL in the begin." );
      commDropCL( db, COMMCSNAME, subCLName, true, true, "Failed to drop CL in the begin." );

      var mainCL = createMainCL( COMMCSNAME, mainCLName );
      createSubCL( COMMCSNAME, subCLName );
      attachCL( COMMCSNAME, mainCL, subCLName );

      insertRecs( mainCL );
      createIdx( mainCL, idxName );
      var rc = explain( mainCL );
      checkResult( rc );

      cleanCL( subCLName );
      cleanCL( mainCLName );
   }
   catch( e )
   {
      throw e;
   }
}

function createMainCL ( csName, mainCLName )
{
   println( "\n---Begin to create MainCL." );

   var options = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mainCL = commCreateCL( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createSubCL ( csName, subCLName )
{
   println( "\n---Begin to create subCL." );

   var options = {
      ShardingKey: { a: 1 }, ShardingType: "hash",
      ReplSize: 0, Compressed: true
   };
   var subCL = commCreateCL( db, csName, subCLName, options, false,
      true, "Failed to create subCL." );
   return subCL;
}

function attachCL ( csName, mainCL, subCLName )
{
   println( "\n---Begin to attach CL." );

   var options = { LowBound: { "a": { $minKey: 1 } }, UpBound: { "a": { $maxKey: 1 } } };
   mainCL.attachCL( csName + "." + subCLName, options );
}

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { a: 1, b: 1, c: 1 } );
   cl.insert( { a: 2, b: 2, c: 2 } );
}

function createIdx ( cl, idxName )
{
   println( "\n---Begin to create index." );

   cl.createIndex( idxName, { b: 1 } );
}

function explain ( cl )
{
   println( "\n---Begin to find and explain." );

   var rc = [];
   var rc0 = cl.find().explain( { Run: true } ).current().toObj();
   var rc1 = cl.find( { a: 2 } ).explain( { Run: true } ).current().toObj();
   var rc2 = cl.find( { "$and": [{ "a": { "$gte": 1 } }, { b: 2 }] } ).explain( { Run: true } ).current().toObj();
   rc.push( rc0 );
   rc.push( rc1 );
   rc.push( rc2 );

   return rc;
}

function checkResult ( rc )
{
   println( "\n---Begin to check result." );

   //compare the returned records for rc[0]
   var Query = JSON.stringify( rc[0]["SubCollections"][0]["Query"]["$and"] );
   var IXBound = rc[0]["SubCollections"][0]["IXBound"];
   var NeedMatch = rc[0]["SubCollections"][0]["NeedMatch"];
   var expQuery = '[]';
   var expIXBound = null;
   var expNeedMatch = false;
   if( Query !== expQuery || IXBound !== expIXBound
      || NeedMatch !== expNeedMatch )
   {
      throw buildException( "checkResult", null, "[ rc0 ]",
         "[Query:" + expQuery + ",IXBound:" + expIXBound + ",NeedMatch:" + expNeedMatch + "]",
         "[Query:" + Query + ",IXBound:" + IXBound + ",NeedMatch:" + NeedMatch + "]" );
   }

   //compare the returned records for rc[1]
   var Query = JSON.stringify( rc[1]["SubCollections"][0]["Query"]["$and"] );
   var IXBound = JSON.stringify( rc[1]["SubCollections"][0]["IXBound"] );
   var NeedMatch = rc[1]["SubCollections"][0]["NeedMatch"];

   var expQuery = '[{"a":{"$et":2}}]';
   var expIXBound = '{"a":[[2,2]]}';
   var expNeedMatch = false;
   if( Query !== expQuery || IXBound !== expIXBound
      || NeedMatch !== expNeedMatch )
   {
      throw buildException( "checkResult", null, "[ rc1 ]",
         "[Query:" + expQuery + ",IXBound:" + expIXBound + ",NeedMatch:" + expNeedMatch + "]",
         "[Query:" + Query + ",IXBound:" + IXBound + ",NeedMatch:" + NeedMatch + "]" );
   }

   //compare the returned records for rc[2]
   var Query = JSON.stringify( rc[2]["SubCollections"][0]["Query"]["$and"] );
   var IXBound = JSON.stringify( rc[2]["SubCollections"][0]["IXBound"] );
   var NeedMatch = rc[2]["SubCollections"][0]["NeedMatch"];

   //var expQuery     = '[{"$and":[{"b":{"$et":2}},{"a":{"$gte":1}}]}]';
   var expQuery = '[{"b":{"$et":2}},{"a":{"$gte":1}}]';
   var expIXBound = '{"b":[[2,2]]}';
   var expNeedMatch = true;
   if( Query !== expQuery || IXBound !== expIXBound
      || NeedMatch !== expNeedMatch )
   {
      throw buildException( "checkResult", null, "[ rc2 ]",
         "[Query:" + expQuery + ",IXBound:" + expIXBound + ",NeedMatch:" + expNeedMatch + "]",
         "[Query:" + Query + ",IXBound:" + IXBound + ",NeedMatch:" + NeedMatch + "]" );
   }

}
