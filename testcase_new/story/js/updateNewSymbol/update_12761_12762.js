/************************************
*@Description: update object(basic types,null array) with pull_all_by
*@author:      liuxiaoxuan
*@createdate:  2017.09.19
*@testlinkCase: seqDB-12761/seqDB-12762
**************************************/
function main()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,"drop CL in the beginning" ) ;
   
   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0 );
   
   //insert data   
	var doc = [{a1:1},
	           {a2:'aaa'},
	           {a3:[]}];
	insertData(dbcl, doc);
	
	//pull_by
	var updateRule = {$pull_all_by:{a1:[1],a2:['aaa'],a3:[[]]}};
	updateData( dbcl, updateRule );
   
   //check result
   var expResult =[{a1:1},
	                {a2:'aaa'},
	                {a3:[]}];
   checkResult( dbcl, null,null, expResult, {_id:1} );
}

main();