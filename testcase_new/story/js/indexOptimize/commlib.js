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
function checkExplain( expectExplains, actExplains )
{
   if(expectExplains.length != actExplains.length)
   {
      throw buildException("checkExplain","length"," check length", expectExplains.length, actExplains.length);
   }

   var keySortExpectExplains = new Array();
   var keySortActExplains = new Array();
   for(var i = 0; i < expectExplains.length; i++)
   {
      var newObj1 = objSortByKey(expectExplains[i]);
      keySortExpectExplains.push(newObj1);  
  
      var newObj2 = objSortByKey(actExplains[i]);
      keySortActExplains.push(newObj2);
   }  
 
   for(var i = 0; i < keySortExpectExplains.length; i++)
   {
      if(JSON.stringify(keySortExpectExplains).indexOf(JSON.stringify(keySortActExplains)) === -1
            || JSON.stringify(keySortActExplains).indexOf(JSON.stringify(keySortExpectExplains)) === -1)
      {
         throw buildException("check explain", "explain", "explain result", 
   		                  JSON.stringify(keySortExpectExplains), JSON.stringify(keySortActExplains));
      }
   }
}

/*****************************************************************
 * sort key of Objects 
 *****************************************************************/
function objSortByKey(obj)
{
   var newKey = Object.keys(obj).sort();
   var newObj = {};
   for(var i=0;i<newKey.length;i++){
      newObj[newKey[i]] =obj[newKey[i]];
   }
   return newObj;
}
