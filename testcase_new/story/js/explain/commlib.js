/************************************
*@description : Public function for explain
*@author : chimanzhao
*@createdate:  2020.5.6
**************************************/
import( "../lib/basic_operation/sequoiadb.js" );
import( "../lib/main.js" );

function testExplain ( conds, dbcl, indexName, scanType )
{
   for( var i = 0; i < conds.length; ++i )
   {
      checkExplain( dbcl, conds[i], indexName, scanType );
   }
}

function checkExplain ( dbcl, cond, expIndexName, expScanType, sortCond, hintCond )
{
   if( sortCond == undefined )
   {
      sortCond = {};
   }
   if( hintCond == undefined )
   {
      hintCond = {};
   }
   var explainObj = dbcl.find( cond ).sort( sortCond ).hint( hintCond ).explain().next().toObj();
   var IndexName = explainObj.IndexName;
   var ScanType = explainObj.ScanType;
   if( expIndexName !== IndexName || expScanType !== ScanType )
   {
      throw new Error( "索引选择错误！" )
   }
}
function checkNeedEvalIO ( cl, expNeedEvalIO )
{
   if( commIsStandalone( db ) )
   {
      var actObj = cl.find().explain( { Evaluate: true } ).current().toObj();
      var actNeedEvalIO = actObj.Search.Input.NeedEvalIO;
      if( actNeedEvalIO !== expNeedEvalIO )
      {
         throw new Error( "expect NeedEvalIO is " + expNeedEvalIO + ",but actually NeedEvalIO is " + actNeedEvalIO );
      }
   }
   else
   {
      var actObj = cl.find().explain( { Evaluate: true } ).current().toObj();
      var actNeedEvalIO = actObj.PlanPath.ChildOperators[0].Search.Input.NeedEvalIO;
      if( actNeedEvalIO !== expNeedEvalIO )
      {
         throw new Error( "expect NeedEvalIO is " + expNeedEvalIO + ",but actually NeedEvalIO is " + actNeedEvalIO );
      }
   }

}


