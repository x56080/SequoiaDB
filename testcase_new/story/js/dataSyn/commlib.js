// create WORKDIR in local host
var saveRecordsDir = WORKDIR + "/datasyn/"
var cmd = new Cmd(); 
readyTmpDir( saveRecordsDir );
/************************************
@Description: Insert data to SequoiaDB
@parameter:
   dbcl: db.cs.cl
   insertNum: insert nums
@return: the insert datas
@author£ºwuyan 2018/12/27
**************************************/
function insertData( dbcl, insertNum )
{
	if( undefined == insertNum ){ insertNum = 10000 ; }
	
	try
   {
      println( "---begin to insert data." ) ;
      var docs = [];     
      for( var i = 0 ; i < insertNum ; ++i )
      {      
         var no = i ;
         var str = getRandomString(100) + "teststr_" + i ;        
         var inta = i ;         
         var fc = i + 0.5896;
         var objs = { "no":no, "str":str, "inta":inta, "fc":fc};         
              
         docs.push(objs);         
      }	
      dbcl.insert( docs ) ; 
      return docs;
      println("--end insert data")
   }
   catch ( e )
   {
      throw buildException("insertDate()", e);      
   }
}

/************************************
@Description: buckInsert data to SequoiaDB
@parameter:
   dbcl: db.cs.cl
   insertNum: the number of inserts must be an integer of 1w
@return: the insert datas
@author£ºwuyan 2018/12/27
**************************************/
function buckInsertData( dbcl, insertNums, beginNums)
{	
   if( undefined == beginNums ){ beginNums = 0 ; }
	try
   {
      println( "---begin to buckInsert data." ) ;
      var batchNums = 10000;     
      var recs = [];
      var times = insertNums/batchNums;
      
      for(var k = 0; k < times; k++)
      {        
         var doc = [];  
         for( var i = 0; i < batchNums; ++i )
         {    
            var count = beginNums++  
            var no = count ;
            var str = getRandomString(100) +  "teststr_" + count ;         
            var inta = count ;         
            var fc = count + 0.7898;
            var objs = { "no":no, "str":str, "inta":inta, "fc":fc};         
            doc.push(objs);             
            recs.push(objs);              
         }	
         dbcl.insert( doc );  
      }    
      println("---end bulkInsert data.")
      return recs;
   }
   catch ( e )
   {
      throw buildException("bulkInsertDate()", e);      
   }
}


function getRandomFloat( min, max )
{
   var range = max - min;
   var value = min + Math.random() * range;
   return value;
}

function getRandomString(len) 
{
   var strLen =  parseInt( Math.random() * len );   
   var str = "";
   var chars = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
   var maxPos = chars.length;
   for( var i = 0; i < strLen; i++ )
   {
      str += chars.charAt(Math.floor(Math.random() * maxPos));     
   }
   return str;
}

function createIndex( cl, idxName, idxKeygen, unique, enforced , errno )
{
   if ( undefined == unique ) { unique = false ; }
   if ( undefined == enforced ) { enforced = false ; }
   if ( undefined == errno ) { errno = "" ; }
   try
   {
      if( undefined == cl || undefined == idxName || undefined == idxKeygen )
      {
         println( "please check the argument of createIndex" ) ;
         throw "ErrArg" ;
      }
      cl.createIndex( idxName, idxKeygen, unique, enforced ) ;     
   }
   catch ( e )
   {
      if ( errno != e )
      {
         println( "failed to create index, rc = "+ e ) ;
         throw e ;
      }
   }
}

/****************************************************
@description: check the result of query
@modify list:
              2016-3-3 yan WU init
****************************************************/
function checkRec( rc, expRecs, filterId, fileNameId )
{	
   println("---begin to check data context.");   		
	//get actual records to array
	var actRecs = [];

   while( rc.next() )
   {
		actRecs.push( rc.current().toObj() );
   }

   //check count
	if( actRecs.length !== expRecs.length )
   {
   	save( fileNameId, actRecs, expRecs);
   	throw buildException("check count", null, "",
									expRecs.length, actRecs.length);
   }

   //check every records every fields
   for( var i in expRecs )
   {
   	var actRec = actRecs[i];
   	var expRec = expRecs[i];
   	for ( var f in expRec )
   	{   		
   		if(!compareObj( actRec[f], expRec[f], filterId ))
	   	{   	   
	   		println("\nerror occurs in "+(parseInt(i)+1)+"th record, in field '"+f+"'");
	   		println("\nactual recs in cl= "+JSON.stringify(actRecs[i])+"\n\nexpect recs= "+JSON.stringify(expRecs[i])); 
	   		saveCheckRecords( fileNameId, actRecs, expRecs);		
	   		throw buildException("checkRec()", "check actRecs fail!");
	   	}
   	}
   } 
   
}

function compareObj(lobj, robj, ignoreId)
{
   if (typeof(lobj) === "object" && 
       typeof(robj) === "object")
   {
      if (lobj === null && robj === null) return true;
      if (lobj.constructor !== robj.constructor) return false;
      var _idNum=0;
      for (key in lobj)
      {
         if (ignoreId && key === "_id") {_idNum=1;continue};
         if (undefined === robj[key]) return false;
         if (!compareObj(lobj[key], robj[key])) return false;
      }
      
      var lkeys = Object.getOwnPropertyNames(lobj);
      var rkeys = Object.getOwnPropertyNames(robj);
      if (lkeys.length !== rkeys.length + _idNum) return false;
      return true;
   }
   else if (lobj === robj)
   {
      return true;
   }else
   {
      return false;
   }
}

function checkDataContent(sdb,csName, clName, sortCond, expRecs, testcaseId, filterId,findCond)
{
   if ( undefined == filterId ) { filterId = true ; }	
   if ( undefined == findCond ) { findCond = null ; }	
   println("---begin to check result from master node.");
   sdb.setSessionAttr( { PreferedInstance: "M" } );
   var dbcl = sdb.getCS(csName).getCL(clName);
   var rc = dbcl.find(findCond).sort(sortCond);          
   checkRec( rc, expRecs, filterId, testcaseId);  
}

function checkResult(csName, clName, groups, sortCond, expRecs, testcaseId, filterId,findCond)
{
   if ( undefined == filterId ) { filterId = true ; }	
   if ( undefined == findCond ) { findCond = null ; }	
    
   println("---begin to check result.");
   for( var i = 1; i < groups.length; i++ )
   {
      try
      {
         var subDB = new Sdb( groups[i].HostName, groups[i].svcname);
         var dbcl = subDB.getCS(csName).getCL(clName);
         //var rc = dbcl.find({},{ "_id": { "$include": 0 } }).sort(sortCond); 
         var rc = dbcl.find(findCond).sort(sortCond);          
         checkRec( rc, expRecs, filterId, testcaseId);
      }catch(e)
      {
         throw buildException( "checkResult", e, "checkResult", "data sync", 
                  "data inconsistency,node info:" + groups[i].HostName + ":" + groups[i].svcname);
      }
      finally
      {
         if ( undefined !== subDB )
         {
            subDB.close();
         }
      }      
   }    
}

/******************************************************************************
@Description : check inpect result
@input:         csName
                clName
                checkTimes
******************************************************************************/
function checkInspectResult(csName, clName, checkTimes)
{
   println("---begin to check data consistency.");
   if ( typeof(checkTimes) == "undefined" )  {  checkTimes = 20;  }

   var inspectBinFile = WORKDIR + "/" + "inspect_" + csName + "_" + clName + ".bin" ;
   var inspectReportFile = WORKDIR + "/" + "inspect_" + csName + "_" + clName + ".bin.report" ;
   var installPath = commGetInstallPath();   
   var inspectCommand = installPath + "/bin/sdbinspect" + " -d " + COORDHOSTNAME + ":" + COORDSVCNAME + " -c " + csName + " -l " + clName + " -o " + inspectBinFile + " -t " + checkTimes; 
   try 
   {  
      // exec sdbinspect 
      cmd.run(inspectCommand) ;
      var info = cmd.run("tail -n 1 " + inspectReportFile);
      var actResult = info.split("\n")[0].split("\:")[1].trim();
      var expectRusult = "exit with no records different";
      // compare result
      if(actResult == expectRusult)
      {      
         // remove report files
         cmd.run("rm -f " + inspectBinFile);
         cmd.run("rm -f " + inspectReportFile);
         println("---check consistency success!") ;
      }
      else
      {         
         println("---check consistency fail, cl name: " + csName + "." + clName); 
      }      
   }   
   catch(e) 
   { 
      throw buildException("checkConsistency", "check consistency fail", "fail",
                                          e, e);  
   }   
}

function readyTmpDir( dir )
{
    try
    {
        cmd.run( "rm -rf "+ dir );
        cmd.run( "mkdir -p "+ dir );
    }
    catch( e )
    {
        throw buildException( "readyTmpDir", e, "make dir " + dir , 0, e ) ;
    } 
}

function saveCheckRecords( fileNameId, actRecs, expRecs)
{  
   var fileName1 = saveRecordsDir + "actRecs_" + fileNameId + ".txt";
   var file1 = new File( fileName1 );
   for( var i in actRecs )
   {
      var actRec = actRecs[i];      
      file1.write( JSON.stringify(actRecs[i]) + "\n" );  
   }
   file1.close();
   
   var fileName2 = saveRecordsDir + "expRecs_" + fileNameId + ".txt";
   var file2 = new File( fileName2 );
   for( var i in expRecs )
   {
      var expRec = expRecs[i];
      file2.write( JSON.stringify(expRecs[i]) + "\n" );
   }
   file2.close();
   
}

function getOneGroups(db)
{
   var groups = commGetGroups( db, "GroupName", "", true, true );
   //random selection of a group
   var serialNo = Math.floor( Math.random()*groups.length);   
   var getGroupInfo = groups[serialNo];   
   return getGroupInfo;   
}
