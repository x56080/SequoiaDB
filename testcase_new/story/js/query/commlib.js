/*******************************************************************************
@Description : Query common functions
@Modify list :
               2014-5-20  xiaojun Hu  Init
*******************************************************************************/
// var cataPort = CATASVCNAME ;
var csName = COMMCSNAME;
var clName = COMMCLNAME;

//var cdb = new Sdb( hostName, cataPort ) ;

// inspect the group and make sure have two group
function inspectGroup ( db )
{
   try
   {
      var catGroupNum = 1;
      var listGroups = db.listReplicaGroups();
      var listGroupsArr = new Array();
      while( listGroups.next() )
      {
         if( listGroupsArr.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            listGroupsArr.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      if( listGroupsArr.length < 2 )
      {
         println( "Error, Don't have enough data groups" + listGroupsArr.length + "<2" );
         return false;
      }
      else
      {
         println( "There are enough data groups : " + listGroupsArr.length + ">2" );
         return true;
      }
   }
   catch( e )
   {
      if( -159 == e )
      {
         println( "Standalone database." );
         return false;
      }
      else
      {
         println( "Execute listReplicaGroups happed error, e = " + e );
         throw e;
      }
   }
}

// create cl. public function commCreateCL() in libs folder cannot to create
// ShardinKey and ShardingType .
function createCL ( db, csName, clName, shardKey, shardType, partition, replSize, compressed )
{
   commCreateCS( db, csName, true, "commCreateCL auto to create collection space" );
   if( undefined == replSize || "" == replSize ) { replSize = 0; }
   try
   {
      if( undefined == partition || "" == partition )
      {
         return eval( 'db.' + csName + '.createCL("' + clName +
            '",{ShardingKey:' + shardKey + ',ShardingType:"' + shardType +
            '","ReplSize":' + replSize + ', "Compressed":' + compressed + '})' );
         println( 'db.' + csName + '.createCL("' + clName +
            '",{ShardingKey:' + shardKey + ',ShardingType:"' + shardType +
            '","ReplSize":' + replSize + ', "Compressed":' + compressed + '})' );
      }
      else
      {
         return eval( 'db.' + csName + '.createCL("' + clName +
            '",{ShardingKey:' + shardKey + ',ShardingType:"' + shardType + '","Partition":'
            + partition + ',"ReplSize":' + replSize + ', "Compressed":' + compressed + '})' );
         println( 'db.' + csName + '.createCL("' + clName +
            '",{ShardingKey:' + shardKey + ',ShardingType:' + shardType + ',"Partition":"'
            + partition + '","ReplSize":' + replSize + ', "Compressed":' + compressed + '})' );
      }
   }
   catch( e )
   {
      println( "Failed to create CS" + csName + "CL" + clName );
      throw e;
   }
}

// Get the group1 and group2. Position is mean the group in array you specify
// it's location
function getTwoGroupSplit ( db, csName, clName, splitArg1, splitArg2 )
{
   try
   {
      // get collection
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );

      var listGroups = db.listReplicaGroups();
      var listGroupsArr = new Array();

      // Check over arguement "splitArg1" "splitArg2"
      if( "" == splitArg1 || undefined == splitArg1 )
      {
         println( "Wrong argument." );
         throw "ErrArg";
      }

      // argument : when the split is percent
      var argument = "";
      if( undefined == splitArg2 || "" == splitArg2 ) { argument = splitArg1; }
      // Get group where Collection Space located in
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            listGroupsArr.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      var groupNum = listGroupsArr.length;

      var snapShotCL = db.snapshot( SDB_SNAP_COLLECTIONS );
      var snapShotClName = new Array();
      var snapShotClGroup = new Array();
      var group = "";
      while( snapShotCL.next() )
      {
         snapShotClName.push( snapShotCL.current().toObj()["Name"] );
         snapShotClGroup.push( snapShotCL.current().toObj()["Details"][0]["GroupName"] );
      }
      for( var i = 0; i < snapShotClGroup.length; i++ )
      {
         if( snapShotClName[i] == csName + "." + clName )
         {
            group = snapShotClGroup[i].toString();
            break;
         }
      }
      if( "" == group )
      {
         println( "Failed to get Group where CL located in, snapShotCL = "
            + snapShotCL );
         throw "ErrGetGroup";
      }
      println( "The source group = " + group );
      // Get the other group where split to
      var groupSplit = "";
      var i = 0;
      do
      {
         if( group != listGroupsArr[i] )
         {
            groupSplit = listGroupsArr[i];
            break;
         }
         ++i;

      } while( i <= groupNum || i <= 8 );

      if( "" == groupSplit )
      {
         println( "Failed to get Split Group, Groups = " + listGroups );
         throw "ErrGetSplitGroup";
      }
      println( "The destination [split]group = " + groupSplit );
      println( "Argument : " + argument );
      if( "" == argument )
         cl.split( group, groupSplit, splitArg1, splitArg2 );
      else
         cl.split( group, groupSplit, argument );
      println( "Success to Split" );

   }
   catch( e )
   {
      println( "Failed to get the group " + e );
      throw e;
   }

}


// Insert the data to database
function insertData ( db, csName, clName )
{
   try
   {
      // get collection
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );

      //println( "Insert data begin" ) ;
      for( var i = 0; i < 100; ++i )
      {
         var d = new Date();
         var date = d.getTime();
         var chname = "名字" + i;
         var carNum = 100000 + i;
         var idNum = i;
         var pscode = i;
         var phone = 1000 + i;
         var fax = 3000 + i;
         var clepho = 1370000 + i;
         var email = 10467890 + i + "@qq.com";
         var string = "{\"Date\":" + date + ",\"CustomerNumber\":" + carNum + ",\"interest\":[\"football\",\"basketball\",\"pingpong\"],\"PersonalDetails\":[{\"Title\":\"Mr\",\"FirstName\":\"abc\",\"LastName\":\"def\",\"NameofChinese\":\"" + chname + "\",\"Former/OtherName\":\"huizhou\",\"Gender\":\"Male\",\"IDType\":\"Passport\",\"IDNumber\":" + idNum + ",\"Nationality\":\"中国\",\"Countryofbirth\":\"中国广东省广州市\",\"CountryofResidence\":\"china\",\"LanguageofStatement\":\"中文\",\"TypeofStatement\":\"CompositeMonthlyStatement\"}],\"Address/ContactDetails\":[{\"CorrespondenceAddress\":\"guangzhou\",\"ResidentialAddress\":\"广州天河\",\"PostalCode\":" + pscode + ",\"PermanentAddress\":\"china\",\"ContactDetails\":[{\"Phone\":" + phone + ",\"Fax\":\"" + fax + "\",\"Cell/Mobile\":" + clepho + ",\"Email\":\"" + email + "\",Number:[123,456,789],Teamer:[\"Tom\",\"Lenoad\",\"Fly\",\"Helen\"]}]}]}"

         //println(i+string) ;
         var insertStr = "";
         insertStr = eval( "(" + string + ")" );
         cl.insert( insertStr );
      }
      sleep( 10 );
      // inspect the number of the date
      var cntNum = 0;
      var i = 0;
      while( i < 100 )
      {
         ++i;
         //println( "count:"+i ) ;
         cntNum = cl.count();
         if( cntNum == 100 )
         {
            println( "the number is 100" );
            break;
         }
         //println( cntNum ) ;
      }
      if( cntNum != 100 )
      {
         println( "Error,wrong numbers of insert data " + cntNum );
         throw "ErrNumData";
      }
   }
   catch( e )
   {
      println( "Failed to insert data to SequoaiDB, rc = " + e );
      throw e;
   }
}

function queryGetCurrentSessions ( db, clName )
{
   var masNode = new Array;
   var masHost = new Array;
   var clRG = commGetCLGroups( db, clName );
   //println( "collection located in : " + clRG ) ;
   for( var i = 0; i < clRG.length; ++i )
   {
      var group = commGetGroups( db );
      for( var j = 0; j < group.length; ++j )
      {
         if( clRG[i] == group[j][0].GroupName )
         {
            for( var m = 0; m < group[j][0].Length; ++m )
            {
               masNode[m] = group[j][m + 1].svcname;    // get master node
               masHost[m] = group[j][m + 1].HostName;   // get master host
            }
         }
      }
   }
   var currentSession = db.snapshot( SDB_SNAP_SESSIONS_CURRENT, {},
      {
         "NodeName": 1, "SessionID": 1, "TotalIndexRead": 1,
         "TotalDataRead": 1
      } ).toArray();
   var dataIdx = new Array( 0, 0 );
   //println( "current session length : " + currentSession.length ) ;
   //println( "get svcnames : " + masNode.length ) ;
   // get total data read and total index read in this loop
   for( var i = 0; i < currentSession.length; ++i )
   {
      var sessionObj = eval( "(" + currentSession[i] + ")" );
      var _nodeName = sessionObj.NodeName;
      var nodeSplit = _nodeName.split( ":" );  // get host and split
      if( false == commIsStandalone( db ) )   // group
      {
         for( var j = 0; j < masNode.length; ++j )
         {
            if( masNode[j] == nodeSplit[1] && masHost[j] == nodeSplit[0] )
            {
               dataIdx[0] += sessionObj.TotalDataRead;
               dataIdx[1] += sessionObj.TotalIndexRead;
               //println( "host : " + masHost[j] + " node : " + masNode[j]) ;
               break;
            }
         }
      }
      else   // standalone
      {
         dataIdx[0] = sessionObj.TotalDataRead;
         dataIdx[1] = sessionObj.TotalIndexRead;
         //println( db.snapshot( SDB_SNAP_SESSIONS_CURRENT, {},
         //                      {"SessionID":1, "TotalIndexRead":1,
         //                      "TotalDataRead":1 }) ) ;
      }
   }
   //println( j + " times : " + dataIdx[0] + "--" + dataIdx[1] ) ;
   return dataIdx;
}

function idxAutoGenData ( cl, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }
   try
   {
      var record = [];
      for( var i = 0; i < insertNum; ++i )
      {
         record.push( {
            "no": i, "no1": i * 2, "no2": i * 3,
            "obj_id": { "$oid": "123abcd00ef12358902300ef" },
            "subobj": { "obj": { "val": "sub" } },
            "string": "西边个喇嘛，东边个哑巴",
            "array": [i + "arr" + i, 5 * i, 2 * i + "ARR" + i, "arrayIndex"], "no3": 4 * i
         } );
      }
      cl.insert( record );
      cnt = 0;
      while( insertNum != cl.count() && cnt < 1000 )
      {
         ++cnt;
         sleep( 2 );
      }
      if( insertNum != cl.count() )
         throw "expect insert number: " + insertNum + ", actual: " + cl.count();
   }
   catch( e )
   {
      println( "failed to insert data to db, rc = " + e );
      throw e;
   }
}

function idxQueryCheck ( cl, queryCond, verifyNum, idxName )
{
   try
   {
      var query = cl.find( queryCond ).explain( { Run: true } ).toArray();
      var queryObj = eval( "(" + query + ")" );
      /*
            if( "tbscan" == queryObj.ScanType )
            {
               println( "expect idxscan, actual: " + queryObj.ScanType ) ;
               throw "ErrorScanType" ;
            }
            if( idxName != queryObj.IndexName )
            {
               println( "expect index name: " + idxName + ", actual: " + queryObj.IndexName ) ;
               throw "ErrorIdxName" ;
            }
      */
      if( verifyNum != queryObj.ReturnNum )
      {
         println( "expect number: " + verifyNum + ", actual: " + queryObj.ReturnNum );
         throw "ErrorvReturnNum";
      }
   }
   catch( e )
   {
      println( query );
      println( "failed to inspect: " + e );
      throw e;
   }
}

function queryGetPrimaryNode ( db, clName )
{
   if( true == commIsStandalone( db ) )
   {
      println( "this is standalone mode" );
      return;
   }
   try
   {
      var group = commGetCLGroups( db, clName );
      var rg = db.getRG( group[0] );
      var primaryNode = rg.getMaster().toString().split( ":" );
      return primaryNode;
   }
   catch( e )
   {
      assert( false );
      println( "failed to get primary node, rc = " + e );
      throw e;
   }
}

/******************add by TingYU*******************/
function Collection ( csName, clName, opt )
{
   this.csName = csName;
   this.clName = clName;

   this.create =
      function()
      {
         println( "---begin to create cl = " + csName + '.' + clName );
         commDropCL( db, csName, clName, true, true, "drop cl in begin" );
         this.cl = commCreateCLByOption( db, csName, clName, opt, true, false, "create cl in begin" );
         return this.cl;
      }

   this.getSelf =
      function()
      {
         return this.cl;
      }

   this.insert =
      function( recNum, dataTypes, fieldNames )
      {
         println( "---begin to insert" );
         var rd = new commDataGenerator();
         var recs = rd.getRecords( recNum, dataTypes, fieldNames );
         db.getCS( csName ).getCL( clName ).insert( recs );
      }

   this.insertRecs =
      function( recs )
      {
         println( "---begin to insert" );
         db.getCS( csName ).getCL( clName ).insert( recs );
      }

   this.getGroups =
      function()
      {
         println( "---begin to get groups of cl" );
         var clFullName = csName + '.' + clName;
         return commGetCLGroups( db, clFullName );
      }

   this.createIndex =
      function( indexName, indexDef, isUnique, enforced )
      {
         if( isUnique === undefined ) { isUnique = false; }
         if( enforced === undefined ) { enforced = false; }
         println( "---begin to create index" );
         db.getCS( csName ).getCL( clName ).createIndex( indexName, indexDef, isUnique, enforced );
      }

}

function select2RG ()
{
   var dataRGInfo = commGetGroups( db );
   var rgsName = {};
   rgsName.srcRG = dataRGInfo[0][0]["GroupName"]; //source group
   rgsName.tgtRG = dataRGInfo[1][0]["GroupName"]; //target group

   return rgsName;
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
   if( actRecs.length !== expRecs.length )
   {
      println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
      throw buildException( "check count", null, "",
         expRecs.length, actRecs.length );
   }

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
}

function checkExplain ( rc, expIdxName )
{
   var plan = rc.explain().current().toObj();
   var expScanType = "ixscan";
   if( expIdxName == "" ) { expScanType = "tbscan"; }

   if( plan.ScanType !== expScanType )
   {
      throw buildException( "checkExplain()", null, "query.explain().ScanType",
         expScanType, plan.ScanType );
   }
   if( plan.IndexName !== expIdxName )
   {
      throw buildException( "checkExplain()", null, "query.explain().IndexName",
         expIdxName, plan.IndexName );
   }
}

function insertAnotherSession ( csName, clName, recs )
{
   println( "---begin to insert in other session" );

   var dbAnother = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   dbAnother.getCS( csName ).getCL( clName ).insert( recs );
   dbAnother.close();
}

/*******************************************************************************
*@Description : common functions
*@Modify list :
*              2016/7/11 huangxiaoni
*******************************************************************************/
function readyCL ( clName )
{
   println( "\n---Begin to create CL." );

   commDropCL( db, COMMCSNAME, clName, true, true,
      "Failed to drop CL in the pre-condition." );

   var cl = commCreateCL( db, COMMCSNAME, clName, -1, true, true, false,
      "Failed to create CL." );
   return cl;
}

function cleanCL ( clName )
{
   println( "\n---Begin to drop CL." );

   commDropCL( db, COMMCSNAME, clName, false, false,
      "Failed to drop CL in the end-condition" );
}

function attachCL ( dbcl, subCLName, range )
{
   try
   {
      dbcl.attachCL( subCLName, range );
      println( "--attach cl success" );
   }
   catch( e )
   {
      throw buildException( "attachCL()", e, "attach cl", "attach cl success", "attach cl fail" );
   }
}

/*******************************************************************************
*@Description : insert data of various data types
*@Modify list :
*              2019/3/1 wangkexin
*******************************************************************************/
function readyData ( cl )
{
   try
   {
      println( "\n---Begin to insert cl data." );
      var record = [{ _id: 1, Key: 123 }, { _id: 2, Key: { "$numberLong": "3000000000" } }, { _id: 3, Key: 123.456 }, { _id: 4, Key: { $decimal: "123.456" } }, { _id: 5, Key: "value" }, { _id: 6, Key: { "$oid": "123abcd00ef12358902300ef" } }, { _id: 7, Key: true }, { _id: 8, Key: { "$date": "2012-01-01" } }, { _id: 9, Key: { "$timestamp": "2012-01-01-13.14.26.124233" } }, { _id: 10, Key: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } }, { _id: 11, Key: { "$regex": "^张", "$options": "i" } }, { _id: 12, Key: { "subobj": "value" } }, { _id: 13, Key: ["abc", 0, "def"] }, { _id: 14, Key: null }, { _id: 15, Key: { "$minKey": 1 } }, { _id: 16, Key: { "$maxKey": 1 } }];

      cl.insert( record );
   }
   catch( e )
   {
      throw buildException( "readyData()", e, "insert", "insert success", "insert fail" );
   }
}