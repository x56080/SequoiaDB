import( "../lib/main.js" );

/*******************************************************************************
@Description : 检查shcema的字段
@Modify list : 2023-02-21 Cheng Jingjing init
*******************************************************************************/
function checkColumnDef ( db, schemaName, expectedColumnDef )
{
   // complete expectedColumnDef
   for( var i in expectedColumnDef )
   {
      if( !expectedColumnDef[i].hasOwnProperty( "Restrict" ) )
      {
         expectedColumnDef[i]["Restrict"] = 0;
      }
      if( !expectedColumnDef[i].hasOwnProperty( "RestrictDesc" ) )
      {
         expectedColumnDef[i]["RestrictDesc"] = "";
      }
   }
   // compare
   var ret = db.list( SDB_LIST_SCHEMAS, { Name: schemaName } );
   var actColumnDef = ret.current().toObj().Columns;
   ret.close();
   assert.equal( actColumnDef, expectedColumnDef );
}

/*******************************************************************************
@Description : 检查shcema是否绑定
@Modify list : 2023-02-21 Cheng Jingjing init
*******************************************************************************/
function checkAddSchema ( db, csName, clName, schemaName )
{
   var clFullName = csName + "." + clName;
   // check SDB_SNAP_CATALOG
   var ret = db.snapshot( SDB_SNAP_CATALOG, { "Name": clFullName, "Schema": schemaName } );
   if( !ret.next() )
   {
      throw new Error( "cl add schema failed! catalog snapshot is empty." );
   }
   ret.close();

   // check SDB_LIST_SCHEMAS
   var ret = db.list( SDB_LIST_SCHEMAS, { "Collection": clFullName, "Name": schemaName } );
   if( !ret.next() )
   {
      throw new Error( "cl add schema failed! schemas list is empty." );
   }
   ret.close();
}

/*********************************************************************
@description  校验集合内部模式字段属性（分区表只适用于所有group中内部模式全部一致的场景，不同group内部模式不一致的场景不适用）
@author liuli
@parameter
   dbcl             {object}        :   必填项，集合对象
   expColumnDef     {object}        :   期望结果集
   exceptId         {boolean}       :   默认为 true，不考虑 _id
********************************************************************* */
function checkInternalSchema ( dbcl, expInternalColumnDef, exceptId )
{
   if( exceptId == undefined ) { exceptId = true; }
   var cursor = dbcl.getInternalSchema();
   {
      var obj = cursor.current().toObj();
      var actInternalColumnDef = obj["InternalSchemas"][0]["Columns"];
      if( exceptId )
      {
         delete actInternalColumnDef._id;
      }
      assert.equal( actInternalColumnDef, expInternalColumnDef, "InternalSchemas: " + JSON.stringify( obj ) );
   }
   cursor.close();
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}

/****************************************************
@description: check the scanType of the explain
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkExplain ( dbcl, keyValue, expectType, expIndexName )
{
   if( undefined == expectType ) { var expectType = "tbscan"; }
   if( undefined == expIndexName ) { var expIndexName = ""; }

   var listIndex = dbcl.find( keyValue ).explain().current().toObj();
   var scanType = listIndex.ScanType;
   var indexName = listIndex.IndexName;
   assert.equal( scanType, expectType );
   assert.equal( indexName, expIndexName );
}

/*********************************************************************
@description  校验主备节点一致性(包括数据以及内部模式)
@author Cheng Jingjing
@parameter
   sortOpt              {object}        :   排序字段
   expResult            {array}         :   非贴源查询期望结果集
   primalResult         {array}         ：  贴源查询期望结果集(undefined不校验)
   expInternalColumnDef {object}        :   内部模式字段期望结果集(undefined不校验)
@usage
      e.g:

********************************************************************* */
function checkConsistence ( db, csName, clName, sortOpt, expResult, primalResult, expInternalColumnDef )
{
   var nodes = commGetCLNodes( db, csName + "." + clName );
   for( var i = 0; i < nodes.length; i++ )
   {
      var data = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
      try
      {
         var dbcl = data.getCS( COMMCSNAME ).getCL( clName );
         var actResult = dbcl.find().sort( sortOpt );
         commCompareResults( actResult, expResult );
         if( primalResult != undefined )
         {
            var actResult = dbcl.find().sort( sortOpt ).flags( SDB_FLG_QUERY_PRIMAL_DATA );
            commCompareResults( actResult, primalResult );
         }
         if( expInternalColumnDef != undefined )
         {
            checkInternalSchema( dbcl, expInternalColumnDef );
         }
      } catch( e )
      {
         throw new Error( "\nnode: " + data + " failed:\n" + e );
      } finally
      {
         data.close();
      }
   }
}