/************************************
*@Description: insert data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function insertData( dbcl, insertData )
{  
   try
   {
      dbcl.insert(insertData);
      println( "--insert data success" ) ;
   }
   catch(e)
   {
      throw buildException("insertData()",e,"insert", "insert success","insert fail");
   }
}

/************************************
*@Description: find and sort data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function sortFindData(dbcl, findCondition1, findCondition2, sortCondition )
{
   try
   {
      var sortResult = dbcl.find( findCondition1, findCondition2 ).sort( sortCondition );
   }
   catch(e)
   {
      throw buildException("sortFindData()",e,"find and sort data", "find and sort data success","find and sort data fail");
   }
   return sortResult;
}

/************************************
*@Description: compare actual and expect result,
               they is not the same ,return error ,
               else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkRec( rc, expRecs )
{				
	//get actual records to array
	var actRecs = [];
   while( rc.next() )
   {
		actRecs.push( rc.current().toObj() );
   }
   //check count
	if( actRecs.length !== expRecs.length )
   {
   	println("\nactual recs in cl= "+JSON.stringify(actRecs)+"\n\nexpect recs= "+JSON.stringify(expRecs));
   	throw buildException("check count", "count ERR", "check count",
									expRecs.length, actRecs.length);
   }
   
   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
   	var actRec = actRecs[i];
   	var expRec = expRecs[i];
   	
   	for ( var f in expRec )
   	{
   		if( JSON.stringify(actRec[f]) !== JSON.stringify(expRec[f]) )
	   	{
	   		println("\nerror occurs in "+(parseInt(i)+1)+"th record, in field '"+f+"'");
	   		println("\nactual record= "+JSON.stringify(actRec)+"\n\nexpect record= "+JSON.stringify(expRec)); 
	   		println("\nactual recs in cl= "+JSON.stringify(actRecs)+"\n\nexpect recs= "+JSON.stringify(expRecs));  		
	   		throw buildException("checkRec()", "rec ERROR");
	   	}
   	}
   }
   //check every records every fields,actRecs as compare source
   for( var i in actRecs )
   {
   	var actRec = actRecs[i];
   	var expRec = expRecs[i];
   	
   	for ( var f in actRec )
   	{
   	   if(f == "_id")
   	   {
   	      continue;
   	   }
   		if( JSON.stringify(actRec[f]) !== JSON.stringify(expRec[f]) )
	   	{
	   		println("\nerror occurs in "+(parseInt(i)+1)+"th record, in field '"+f+"'");
	   		println("\nactual record= "+JSON.stringify(actRec)+"\n\nexpect record= "+JSON.stringify(expRec));
	   		println("\nactual recs in cl= "+JSON.stringify(actRecs)+"\n\nexpect recs= "+JSON.stringify(expRecs));  		
	   		throw buildException("checkRec()", "rec ERROR");
	   	}
   	}
   }
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkResult( dbcl, findCondition, findCondition2, expRecs, sortCondition )
{
   var rc = sortFindData(dbcl, findCondition, findCondition2, sortCondition );
   println("--begin to check the data");
   checkRec( rc, expRecs );
   println("--end check the data");
}

/************************************
*@Description: check result when the expect result of find data is failed.
*@author:      zhaoyu 
*@createDate:  2016/4/28
*@parameters:               
**************************************/
function InvalidArgCheck( dbcl, condition, condition2, expRecs )
{ 
   try
   {
      dbcl.find( condition, condition2 ).toArray();   
      throw "need throw error";
  
   }
   catch(e)
   { 
     if(expRecs != e )
   	 {
		    throw buildException("InvalidArgCheck() "+e+"\ncheckResult "+" \nExpect result: "+expRecs+" \nActual result:"+e);   
   	 }
   	 else
   	 {
   	    println("check result is ok!");   		
   	 } 
   }
}


