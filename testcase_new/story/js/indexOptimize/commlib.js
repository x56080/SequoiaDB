/*****************************************************************
 * get explain 
 *****************************************************************/
function getExplain( dbcl, findCond, sortCond, hintCond )
{
   if ( typeof(findCond) == "undefined" ) { findCond = null; }
   if ( typeof(sortCond) == "undefined" ) { sortCond = null; }
   if ( typeof(hintCond) == "undefined" ) { hintCond = null; }
   
   //保存所有访问计划
   var explains = new Array();
   var rc = dbcl.find(findCond).sort(sortCond).hint(hintCond).explain({Run:true}).toArray();
   var groupExplains = eval("(" + rc + ")");
   var explainObj = {};
   for( var f in groupExplains )
   {
       if((f == "ScanType") || (f == "IndexName"))
       {
           explainObj[f] = groupExplains[f];     
       }
   }
   explains.push(explainObj);
   return explains;
}

/*****************************************************************
 * check explain 
 *****************************************************************/
function checkExplain( actResults, expectScanType, expectIndexName )
{
   if(actResults.length == 0)
   {
      throw buildException("checkExplain","explain","explain length", actResults.length, 0);
   }
   if ( typeof(expectIndexName) == "undefined" ) { expectIndexName = ""; }

   var actResult = actResults[0];
   if(expectScanType != actResult["ScanType"] || expectIndexName != actResult["IndexName"])
   {
      var expResult = {"ScanType" : expectScanType, "IndexName" : expectIndexName};
      throw buildException("check explain", "explain", "explain result",
                                  JSON.stringify(expResult),  JSON.stringify(actResult));
   }
}
