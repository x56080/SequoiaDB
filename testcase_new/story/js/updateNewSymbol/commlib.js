
/************************************
*@Description: insert data
*@author:      liuxiaoxuan
*@createDate:  2017.09.18
**************************************/
function insertData( dbcl, condition )
{  
   try
   {
      dbcl.insert(condition);
      println( "--insert data success" ) ;
   }
   catch(e)
   {
      throw buildException("insertData()",e,"insert", "insert success","insert fail");
   }
}
/************************************
*@Description: update data
*@author:      liuxiaoxuan
*@createDate:  2017.09.18
**************************************/
function updateData( dbcl, updateCondition, findCondition )
{
   if ( typeof(findCondition) == "undefined" ) { findCondition = null ; }  
   try
   {
      dbcl.update( updateCondition, findCondition );
      println( "--update data success" ) ;
   }
   catch(e)
   {
      throw buildException("updateData()",e,"update", "update success","update fail");
   }
}

/************************************
*@Description: upsert data
*@author:      liuxiaoxuan
*@createDate:  2017.09.18
**************************************/
function upsertData( dbcl, upsertCondition, findConditions )
{
   if ( typeof(findConditions) == "undefined" ) 
	{ 
      findConditions = []; 
		findConditions[0] = null; 
	}  
	for( var i in findConditions )
	{
		try
      {
         dbcl.upsert( upsertCondition, findConditions[i] );
         println( "--upsert data success" ) ;
      }
		catch(e)
      {
         throw buildException("upsertData()",e,"upsert", "upsert success","upsert fail");
      }
	}
  
}

/************************************
*@Description: create index
*@author:      liuxiaoxuan
*@createDate:  2017.09.18
**************************************/
function createIndex( dbcl, indexName, key )
{
	try
	{
	   dbcl.createIndex(indexName , key);
	   println("--create index success");
   }
	catch(e)
   {
	   throw buildException("createIndex()",e,"create index", "create success","create fail");
   }
}


/************************************
*@Description: find and sort data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function sortFindData(dbcl, findCondition, findCondition2, sortCondition )
{
	 if ( typeof( findCondition ) == "undefined" ) { findCondition = null; }
   try
   {
      var sortResult = dbcl.find( findCondition, findCondition2 ).sort( sortCondition );
   }
   catch(e)
   {
      throw buildException("sortFindData()",e,"find and sort data", "find and sort data success","find and sort data fail");
   }
   return sortResult;
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
   	throw buildException("check count", null, "",
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
	   		println("\nactual recs in cl= "+JSON.stringify(actRecs)+"\n\nexpect recs= "+JSON.stringify(expRecs));   		
	   		throw buildException("checkRec()", "rec ERROR");
	   	}
   	}
   }
}