/************************************************************************
*@Description:  seqDB-5568:普通表上使用访问计划，查询条件为空/仅为索引字段/不仅有索引字段_ST.explainAdd.01
*@Author:  2016/7/11  huangxiaoni
************************************************************************/
main();

function main ()
{
   try
   {
      var clName = COMMCLNAME + "_5568";
      var idxName = CHANGEDPREFIX + "_idx";

      var cl = readyCL( clName );

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

function insertRecs ( cl )
{
   println( "\n---Begin to insert records." );

   cl.insert( { a: 1, b: 1 } );
   cl.insert( { a: 2, b: 2 } );
   cl.insert( { a: 3, b: 3 } );
}

function createIdx ( cl, idxName )
{
   println( "\n---Begin to create index." );

   cl.createIndex( idxName, { a: 1 } );
}

function explain ( cl )
{
   println( "\n---Begin to find and explain." );

   var rc = [];
   var rc0 = cl.find().explain( { Run: true } ).current().toObj();
   var rc1 = cl.find( { a: 2 } ).explain( { Run: true } ).current().toObj();
   var rc2 = cl.find( { "$and": [{ "a": { "$gte": 1 } }, { b: 2 }] } ).hint( { '': '' } ).explain( { Run: true } ).current().toObj();
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
   var expIXBound = '{"a":[[1,{"$decimal":"MAX"}]]}';
   var expNeedMatch = true;
   if( Query !== expQuery || IXBound !== expIXBound
      || NeedMatch !== expNeedMatch )
   {
      throw buildException( "checkResult", null, "[ rc2 ]",
         "[Query:" + expQuery + ",IXBound:" + expIXBound + ",NeedMatch:" + expNeedMatch + "]",
         "[Query:" + Query + ",IXBound:" + IXBound + ",NeedMatch:" + NeedMatch + "]" );
   }

}
