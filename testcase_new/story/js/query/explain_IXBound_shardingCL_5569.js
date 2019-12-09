/************************************************************************
*@Description:  seqDB-5569:切分表上使用访问计划，查询条件为空/仅为索引字段/不仅有索引字段_ST.explainAdd.02
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

      var clName = COMMCLNAME + "_5569";
      var idxName = CHANGEDPREFIX + "_idx";

      var cl = createCL( clName );

      insertRecs( cl );
      createIdx( cl, idxName );
      var rc = explain( cl );
      checkResult( rc );

      cleanCL( clName );
   }
   catch( e )
   {
      throw e;
   }
}

function createCL ( clName )
{
   println( "\n---Begin to create CL." );

   commDropCL( db, COMMCSNAME, clName, true, true, "Failed to drop CL in the begin." );
   var options = { ShardingKey: { a: 1 }, ShardingType: "range", ReplSize: 0 };
   var cl = commCreateCLByOption( db, COMMCSNAME, clName, options, false,
      true, "Failed to create cl." );
   return cl;
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
   var Query = String( rc[0]["Query"]["$and"] );
   var NeedMatch = rc[0]["NeedMatch"];

   var expQuery = "";
   var expNeedMatch = false;
   if( Query !== expQuery || NeedMatch !== expNeedMatch )
   {
      throw buildException( "checkResult", null, "[ rc0 ]",
         "[Query:" + expQuery + ",NeedMatch:" + expNeedMatch + "]",
         "[Query:" + Query + ",NeedMatch:" + NeedMatch + "]" );
   }

   //compare the returned records for rc[1]
   var Query = JSON.stringify( rc[1]["Query"]["$and"] );
   var IXBound = JSON.stringify( rc[1]["IXBound"] );
   var NeedMatch = rc[1]["NeedMatch"];

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
   var Query = JSON.stringify( rc[2]["Query"]["$and"] );
   var IXBound = JSON.stringify( rc[2]["IXBound"] );
   var NeedMatch = rc[2]["NeedMatch"];

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
