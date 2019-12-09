/******************************************************************************
*@Description : Public function for testing domain.
*@Modify list :
*               2014-6-17  xiaojun Hu  Init
******************************************************************************/

var cataPort = CATASVCNAME;
var csName = CHANGEDPREFIX + "_dom_cs";
var clName = CHANGEDPREFIX + "_dom_cl";


// Create domain
function createDomain ( db, domName, domGroups, domAutoSplit )
{
   try
   {
      if( undefined == domGroups && undefined == domAutoSplit )
         db.createDomain( domName );
      else if( undefined != domGroups && undefined == domAutoSplit )
         db.createDomain( domName, domGroups );
      else
         db.createDomain( domName, domGroups, domAutoSplit );
      //println( "Success to create Domain : " + domName ) ;
   }
   catch( e )
   {
      println( "Failed to create domain : [" + domName + "], rc = " + e );
      throw e;
   }
}

// List domains
function listDomain ( db, domBson )
{
   if( undefined == domBson ) { domBson = ""; }
   try
   {
      if( "" == domBson )
         var listDom = db.listDomains();
      else
         var listDom = db.listDomains( domBson );
      return listDom;
   }
   catch( e )
   {
      println( "Failed to list domains, rc = " + e );
      throw e;
   }
}

// Get domain
function getDomain ( db, domName )
{
   try
   {
      var dom = db.getDomain( domName );
      return dom;
   }
   catch( e )
   {
      println( "Failed to get domain, rc = " + e );
      throw e;
   }
}

// Drop domain
function dropDomain ( db, domName, allowErrName )
{
   if( undefined == allowErrName ) { allowErrName = false; }
   if( undefined == domName ) { domName = "" }
   try
   {
      db.dropDomain( domName );
   }
   catch( e )
   {
      if( false == allowErrName && -214 != e )
      {
         println( "Failed to drop domain, rc = " + e );
         throw e;
      }
   }
}

// Alter domain
function alterDomain ( db, domName, alterOption )
{
   try
   {
      var dom = getDomain( db, domName );
      dom.alter( alterOption );
   }
   catch( e )
   {
      println( "Failed to alter domains, rc = " + e );
      throw e;
   }
}

// Domain list collections
function listCollections ( db, domName )
{
   try
   {
      var dom = getDomain( db, domName );
      dom.listCollections();
   }
   catch( e )
   {
      println( "Failed to list collections, rc = " + e );
      throw e;
   }
}

// Domain list collection spaces
function listCollectionSpace ( db, domName )
{
   try
   {
      var dom = getDomain( db, domName );
      dom.listCollectionSpaces();
   }
   catch( e )
   {
      println( "Failed to list collection, rc = " + e );
      throw e;
   }
}

// Inspect domain is created correct or wrong
function inspectDomain ( db, domName )
{
   try
   {
      // Cata Node list domain information
      var listDom = db.listDomains();
      var domArray = new Array();
      while( listDom.next() )
      {
         domArray.push( listDom.current().toObj()["Name"] );
      }
      var i = 0;
      do
      {
         if( domName == domArray[i] )
            break;
         ++i;
      } while( i <= domArray.length );
      // println( "domArray length : " + domArray.length ) ;
      // If traverse domain name, don't find "domName", throw error.
      if( i >= domArray.length )
      {
         println( "Don't have domain, Name = " + domName );
         throw "NoDomain";
      }
   }
   catch( e )
   {
      println( "Failed to inspect the domain name, rc = " + e );
      throw e;
   }
}

// Insert data to SequoiaDB
function insertData ( db, csName, clName, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }
   var data = [];
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      for( var i = 0; i < insertNum; ++i )
      {
         var no = i;
         var user = "布斯" + i;
         var phone = 13700000000 + i;
         var compa1 = "MI" + i;
         var compa2 = "GOLDEN" + i;
         var date = new Date();
         var time = date.getTime();
         var card = 6800000000 + i;
         /**********************************************************************
         data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                         "company":[MI5,GOLDEN5],"openDate":1402990912105,
                         "cardID":6800000005}
         **********************************************************************/
         //println( "Start to deal string" ) ;
         var insertString = { "No": no, "customerName": user, "phoneNumber": phone, "company": [compa1, compa2], "openDate": time, "cardID": card };
         data.push( insertString );
         //println( "Begin to insert" ) ;
      }
      //insert date
      cl.insert( data );

      // inspect the data is insert success or not
      var cnt = 0;
      do
      {
         var count = cl.count();
         if( insertNum == count )
            break;
         ++cnt;
      } while( cnt < 100 );
      if( insertNum != count )
      {
         println( "Wrong quantity of records, count = " + count + " not equal " + insertNum );
         throw "ErrNumRecord";
      }
      println( "Success to insert data. " );
   }
   catch( e )
   {
      println( "Failed to insert data to Sdb, rc = " + e );
      throw e;
   }
}

// Query the data
function queryData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      var query =
         cl.find( {
            $and: [{
               "No": { $gte: 50 }, "phoneNumber": { $lt: 13700001000 },
               "customerName": { $nin: ["MI500", "GOLDEN500"] },
               "cardID": { $lte: 6800001111 },
               "openDate": { $gt: 1402990912105 }
            }]
         } ).count();
      if( 950 != query )
      {
         println( "Wrong query the data, count = " + query + "not equal 950" );
         throw "ErrQueryNum";
      }
      println( "Success to query the data" );
   }
   catch( e )
   {
      println( "Failed to query data, rc = " + e );
      throw e;
   }
}

// Update the data
function updateData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      cl.update( {
         $inc: { "cardID": 5 }, $set: { "customerName": "布斯" },
         $unset: { "openDate": "" }, $addtoset: { company: [2, 3] },
         $pull_all: { "company": ["MI88", "GOLDEN88", 2, 3] }
      } );
      var count = cl.find( {
         "No": 88, "customerName": "布斯",
         "phoneNumber": 13700000088, "cardID": 6800000093,
         "company": ["MI88", "GOLDEN88", 2, 3]
      } ).count();
      if( 1 != count )
      {
         println( "Wrong update the data, count = " + count + "not equal 1" );
         throw "ErrUpdateData";
      }
      println( "Success to update the data." );
   }
   catch( e )
   {
      println( "Failed to update the data, rc = " + e );
      throw e;
   }
}
// Remove data
function removeData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      cl.remove( { "No": { $gte: 89 } }, { "": "$id" } );
      var cnt = 0;
      do
      {
         var count = cl.count();
         if( 89 == count )
            break;
         ++cnt;
      } while( cnt <= 20 );
      if( 89 != count )
      {
         println( "Wrong remove the date, count = " + count + "not equal 89" );
         throw "ErrRemoveData";
      }
      println( "Success to remove the data." );
   }
   catch( e )
   {
      println( "Failed to remove the data, rc = " + e );
      throw e;
   }
}
// Get group from Sdb
function getGroup ( db )
{
   try
   {
      var listGroups = db.listReplicaGroups();
      var groupArray = new Array();
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()['GroupID'] >= DATA_GROUP_ID_BEGIN )
         {
            groupArray.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      return groupArray;
   }
   catch( e )
   {
      println( "Failed to get groups from sdb, rc = " + e );
      throw e;
   }
}

// Get domains from Sdb
function getDomains ( db )
{
   try
   {
      var listDom = db.listDomains();
      var domArray = new Array();
      while( listDom.next() )
      {
         domArray.push( listDom.current().toObj()["Name"] );
      }
      //println( "Get domains = [" + domArray + "]" ) ;
      return domArray;
   }
   catch( e )
   {
      if( -214 != e )
      {
         println( "Failed to get Domain, rc = " + e );
         throw e;
      }
      else
         return domArray;
   }
}

// Inspect domain.alter
function inspectAlter ( db, group, domName )
{
   try
   {
      // undefined function throw type error
      var typeErr = "TypeError: listDom.current().toObj().Groups[i]" +
         "is undefined";
      var i = 0;
      do
      {
         // the function cannot do twice, need new one
         //var listDom =
         //db.listDomains({"Name":"local_test_cs_DomAlterSpecifyGroup"}) ;
         // Using { "Name" : domName } is Wrong
         var listDom = db.listDomains( { Name: domName } );
         var groupArray = new Array();
         while( listDom.next() )
         {
            groupArray.push( listDom.current().toObj()["Groups"][i
            ]["GroupName"] );
         }
         //println( "1111Group : " + group) ;

         // "GroupName" just have one
         if( group == groupArray )
         {
            println( "Have alter group : [ " + groupArray + " ]" );
            break;
         }
         if( "" == groupArray )
         {
            println( "Don't have group : [ " + group + " ]" );
            throw "NoDomainGroup";
         }
         //println( "count" + i ) ;
         ++i; // addselt 1
      } while( i < 100 );
      println( "Inspect alter get groups in domain = [" + group + "]" );
   }
   catch( e )
   {
      if( typeErr == e )
      {
         println( "Domain don'n have group : [ " + group + " ]" );
         throw e;
      }
      else
      {
         println( "Failed to inspect domain alter, rc = " + e );
         throw e;
      }
   }
}


// Clear domain in the beginning or in the end
function clearDomain ( db, domName )
{
   try
   {
      var getDoms = new Array();
      getDoms = getDomains( db );
      if( "" == getDoms )
         throw "Domain don't exist";
      for( var i = 0; i < getDoms.length; ++i )
      {
         if( domName == getDoms[i] )
         {
            var domCSarr = new Array();
            var dom = db.getDomain( domName );
            // println( "domani :" + dom ) ;
            domCS = dom.listCollectionSpaces();
            while( domCS.next() )
            {
               domCSarr.push( domCS.current().toObj()["Name"] );
            }
            if( "" != domCSarr )
            {
               for( var j = 0; j < domCSarr.length; ++j )
               {
                  db.dropCS( domCSarr[j] );
                  //println( "Clear collection space : [" + domCSarr[j] + "]" ) ;
               }
            }
            //println( "Drop domain : [" + getDoms[i] + "]" ) ;
            dropDomain( db, getDoms[i] );
         }
      }
      // inspect the domain was droped or not
      //println( "Begin to inspect domain : [" + domName + "]" ) ;
      var clearDom = new Array();
      clearDom = getDomains( db );
      for( var i = 0; i < clearDom.length; ++i )
      {
         if( domName == clearDom[i] )
         {
            println( "After clear , domain : [" + clearDom + "]" );
            throw "ErrClearDomain";
         }
      }
   }
   catch( e )
   {
      if( "Domain don't exist" != e )
      {
         println( "Failed to clear domains, rc = " + e );
         throw e;
      }
      else
         println( "Don't exist domain : [" + domName + "]" +
            " Make sure using in the begnning" );
   }
}

// Inspect parameter : AutoSplit. {AutoSplit : true}
function inspectAutoSplit ( db, csName, clName, domName )
{
   try
   {
      // get group where collection locate in
      var catCL = csName + "." + clName;
      var lsCatCL = db.snapshot( 8, { Name: catCL } );
      //var lsCatCL = db.snapshot(8) ;
      var lsCatCLarr = new Array();
      while( lsCatCL.next() )
      {
         lsCatCLarr.push( lsCatCL.current().toObj()["CataInfo"][0]["GroupName"] );
      }
      var clGroup = lsCatCLarr[0];
      // get group in domain
      var domGroup = db.listReplicaGroups();
      var domGroupArr = new Array();
      while( domGroup.next() )
      {
         domGroupArr.push( domGroup.current().toObj()["GroupName"] );
      }
      //println( "Groups array " + domGroupArr.length ) ;
      for( var i = 0; i < domGroupArr.length; ++i )
      {
         var domRG = domGroupArr[i];
         //println( "csGroup" + clGroup + "Groups" +domRG) ;
         if( clGroup == domRG )
         {
            println( "CS located in Group : [" + domRG + "]" );
            // get master node in domain group
            var rg = db.getRG( domGroupArr[i] );
            var master = rg.getMaster();
            master = master.toString();
            var masterNode = new Array();
            //println( "get Master : [" + typeof(master) + "]" ) ;
            mastNode = master.split( ":" );
            dataNode = new Sdb( mastNode[0], mastNode[1] );
            // inspect the count is greate than 0
            var cs = dataNode.getCS( csName );
            var cl = cs.getCL( clName );
            var k = 0;
            do
            {
               var count = cl.count();
               if( count > 0 )
                  break;
               ++k;
            } while( k < 10 );
            if( count <= 0 )
            {
               println( "Wrong AutoSplit parameter, count = [" + count + "]" );
               throw "ErrDomAutoSplit";
            }
            println( "CL : " + cl + " have " + count + " records." );
         }
      }
   }
   catch( e )
   {
      println( "Failed to inspect autosplit parameter, rc = " + e );
      throw e;
   }
}

// createCS and make sure located in one group
function getCSGroup ( db, csName, clName )
{
   try
   {
      var name = csName + "." + clName;
      var catQuery = db.snapshot( 8 );
      var csGroup = new Array();
      var catCsName = new Array();
      while( catQuery.next() )
      {
         var tmpObj = catQuery.current().toObj();
         if( typeof ( tmpObj["CataInfo"] ) == "undefined" ) { continue; } // main cl
         if( tmpObj["CataInfo"].length == 0 ) { continue; }
         csGroup.push( tmpObj["CataInfo"][0]["GroupName"] );
         catCsName.push( tmpObj["Name"] );
      }
      var group = "";
      for( var i = 0; i < catCsName.length; ++i )
      {
         println( "name :" + name + "typeof :" + typeof ( name ) );
         println( "cata cs name : " + catCsName[i] + "typeof : " +
            typeof ( catCsName[i] ) );
         if( name == catCsName[i] )
            group = csGroup[i];
      }
      if( "" == group )
      {
         println( "Don't get group for cs name, cs = [" + catCsName +
            "]--cs group[" + csGroup + "]" );
         throw "ErrGetCSGroup";
      }
      return group;
   }
   catch( e )
   {
      println( "Failed to get CS corresponding group, rc = " + e );
      throw e;
   }
}

// Create index
//function createIndex( db, csName, clName , idxName)
//{
//   var cs = db.getCS( csName ) ;
//   var cl = cs.getCL( clName ) ;
//   try
//   {
//      cl.createIndex( "domainIndex", {"No":-1}, true, true ) ;
//      //var listIdx = cl.getIndex( "domIndex" ) ;
//      ////idxObj = listIdx.current().toObj()["IndexDef"] ;
//      //var idxName = listIdx.name ;
//      //if( "domIndex" != idxName )
//      //   throw "Don't have index = domIndex" ;
//
//   }
//   catch( e )
//   {
//      println( "Failed to create index, rc = " + e ) ;
//      throw  e ;
//   }
//}

// Inspect the run mode
function inspectRunMode ( db )
{
   try
   {
      db.listReplicaGroups();
      return "group";
   }
   catch( e )
   {
      if( -159 == e )
      {
         return "standalone";
      }
      else
      {
         println( "Failed to get run mode, rc = " + e );
         throw e;
      }
   }
}

