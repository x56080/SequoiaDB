/*******************************************************************************
@Description : Create Index common functions
@Modify list :
               2014-5-20  xiaojun Hu  Init
*******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

var csName = COMMCSNAME;
var clName = COMMCLNAME;

// common functions
function createIndex ( cl, idxName, idxKeygen, unique, enforced, errno )
{
   if( undefined == unique ) { unique = false; }
   if( undefined == enforced ) { enforced = false; }
   if( undefined == errno ) { errno = ""; }
   try
   {
      if( undefined == cl || undefined == idxName || undefined == idxKeygen )
      {
         throw new Error( "ErrArg" );
      }
      cl.createIndex( idxName, idxKeygen, unique, enforced );
      // inspect the index we created
   }
   catch( e )
   {
      if( errno != e.message )
      {
         throw e;
      }
   }
}

//inspect the index is created success or not.
function inspecIndex ( cl, indexName, indexKey, keyValue, idxUnique, idxEnforced )
{
   if( undefined == idxUnique ) { idxUnique = false; }
   if( undefined == idxEnforced ) { idxEnforced = false; }
   if( undefined == cl || undefined == indexName || undefined == indexKey || undefined == keyValue )
   {
      throw new Error( "ErrArg" );
   }
   var getIndex = new Boolean( true );
   try
   {
      getIndex = cl.getIndex( indexName );
   }
   catch( e )
   {
      getIndex = undefined;
   }
   var cnt = 0;
   while( cnt < 20 )
   {
      try
      {
         getIndex = cl.getIndex( indexName );
      }
      catch( e )
      {
         getIndex = undefined;
      }
      if( undefined != getIndex )
      {
         break;
      }
      ++cnt;
   }
   if( undefined == getIndex )
   {
      throw new Error( "ErrIdxName" );
   }
   var indexDef = getIndex.toString();
   indexDef = eval( '(' + indexDef + ')' );
   var index = indexDef["IndexDef"];

   assert.equal( keyValue, index["key"][indexKey] );
   assert.equal( idxUnique, index["unique"] );
   assert.equal( idxEnforced, index["enforced"] );
}

/****************************************************
@description: check the scanType of the explain
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkExplain ( CL, keyValue )
{
   listIndex = CL.find( keyValue ).explain()
   var scanType = listIndex.current().toObj()["ScanType"];
   var expectType = "ixscan";
   assert.equal( expectType, scanType );
}

/****************************************************
@description: check the result of query
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkResult ( idxCL, keyValue )
{
   var rc = idxCL.find( keyValue );
   var expRecs = [];
   expRecs.push( keyValue );
   checkRec( rc, expRecs );
}

function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length );

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
}

/******************************************************************************
 * @description: 检查集合中索引是否打开NotArray
 * @param {*}
 * @return {*}
 ******************************************************************************/
function checkNotArray ( cl, idxname, expResult )
{
   var index = cl.getIndex( idxname );
   var notArray = index.toObj().IndexDef.NotArray;
   assert.equal( expResult, notArray );
}

/******************************************************************************
 * @description: 检查分析结果是否有索引覆盖
 * @param {*} explain
 * @param {*} expResult
 * @return {*}
 ******************************************************************************/
function checkIndexCover ( explain, expResult )
{
   while( explain.next() )
   {
      var result = explain.current().toObj();
      var actResult = result.IndexCover;
      assert.equal( expResult, actResult );
   }
}