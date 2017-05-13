/*******************************************************************************
*@Description: JavaScript common function library
*@Modify list:
*   2014-2-24 Jianhui Xu  Init
*******************************************************************************/

// begin global variable configuration
// CSPREFIX, COORDSVCNAME, COORDHOSTNAME  is input parameter
// UUID, UUNAME is input parameter
//if ( typeof(CSPREFIX) == "undefined" ) { CSPREFIX = "local_test"; }
if ( typeof(CMSVCNAME) == "undefined" ) { CMSVCNAME = "11790"; }
if ( typeof(CHANGEDPREFIX) == "undefined" ) { CHANGEDPREFIX = "local_test"; }
if ( typeof(COORDSVCNAME) == "undefined" ) { COORDSVCNAME = "50000"; }
if ( typeof(CATASVCNAME) == "undefined" ) { CATASVCNAME = "11800"; }
if ( typeof(COORDHOSTNAME) == "undefined" ) { COORDHOSTNAME = 'localhost'; }
if ( typeof(UUID) == "undefined" ) { UUID = 1 ; }
if ( typeof(UUNAME) == "undefined" ) { UUNAME = "ID"+UUID+"NAME" ; }
if ( typeof(RSRVPORTBEGIN) == "undefined" ) 	{ RSRVPORTBEGIN = '26000'; }
if ( typeof(RSRVPORTEND)   == "undefined" ) 	{ RSRVPORTEND   = '27000'; }
if ( typeof(RSRVNODEDIR)   == "undefined" ) 	{ RSRVNODEDIR   = "/opt/sequoiadb/database/"; }

var COMMCSNAME = CHANGEDPREFIX + "_cs" ;
var COMMCLNAME = CHANGEDPREFIX + "_cl" ;
var COMMDUMMYCLNAME = "test_dummy_cl" ;

var DATA_GROUP_ID_BEGIN = 1000 ;
var CATALOG_GROUPNAME = "SYSCatalogGroup" ;
var COORD_GROUPNAME = "SYSCoord" ;
var DOMAINNAME = "_SYS_DOMAIN_" ;
var SPARE_GROUPNAME = "SYSSpare";
var CATALOG_GROUPID = 1 ;
var COORD_GROUPID = 2 ;
var SPARE_GROUPID = 5 ;
// end global variable configuration

// control variable
var funcCommDropCSTimes = 0 ;
var funcCommCreateCSTimes = 0 ;
var funcCommCreateCLTimes = 0 ;
var funcCommCreateCLOptTimes =  0 ;
var funcCommDropCLTimes = 0 ;
// end control variable

doassert = function(msg) {
    // eval if msg is a function
    if (typeof(msg) == "function")
        msg = msg();

    if (typeof (msg) == "string" && msg.indexOf("assert") == 0)
        print(msg);
    else
        print("assert: " + msg);

    var ex = Error(msg);
    print(ex.stack);
    throw ex;
}

assert = function(b, msg){
    if (assert._debug && msg) print("in assert for: " + msg);
    if (b)
        return;
    doassert(msg == undefined ? "assert failed" : "assert failed : " + msg);
}


function RSize( CSname ){
//there are many groups , and I want to know the max nodes' size in one group
this.ReplSize_groups = function( db )
{
   var RGname ;
   var size = 0 ;

   try
   {
      RGname = db.listReplicaGroups() ;
   }
   catch ( e )
   {
      if ( e != -159 )
      {
         println( "list replica groups failed: " + e ) ;
         throw e ;
      }
   }

   for( var i = 1 ; i < RGname.size() ; ++i )
   {
      var eRGname = eval("("+RGname[i]+")");

      if( size < eRGname["Group"].length )
      {
         size = eRGname["Group"].length ;
      }
   }
   return size ;
} ,

//get the group' node_number
this.ReplSize_one_group = function( db , CSname )
{
   try
   {
      var catadb = new Sdb('localhost', CATASVCNAME ) ;
      var data = catadb.SYSCAT.SYSCOLLECTIONSPACES.find() ;
      for ( var i = 0 ; i < data.size() ; ++i  )
      {
         var data1 = eval("("+data[i]+")") ;
         if ( CSname == data1["Name"] )
         {
            var groupID = data1["Group"][0]["GroupID"] ;  
            break ;
         }
      }
   
      if( !(i < data.size()) )
      {
         return -1 ;
      }
   
      var RGname = db.listReplicaGroups() ;
      for ( var i = 1 ; i < RGname.size() ; ++i )
      {
         eRGname = eval("("+RGname[i]+")") ;
         if ( groupID == eRGname["GroupID"] )
         {
            return eRGname["Group"].length ;
         }
      }
   }
  catch( e )
  {
     println( "Get replica one group size failed: " + e ) ;
     return 1 ;
  }
} ,
// if you want to get the max nodes' number when there are groups,
//please enter "split" in the first var when you use this function()
this.ReplSize = function( db )
{
   return 0 ;
   /*
   //return this.ReplSize_groups( db ) ; 

   switch( arguments[0] )
   {
      case "split":
         return this.ReplSize_groups( db );
      break;
      default:
         return this.ReplSize_one_group( db , CSname );

      break;
   }
   */
}

}  //end RSize object

/* *****************************************************************************
@discription: check database mode is standalone
@author: Jianhui Xu
***************************************************************************** */
function commIsStandalone( db )
{
   try
   {
      db.listReplicaGroups() ;
      return false ;
   }
   catch( e )
   {
      if( e == -159 )
      {
         return true ;
      }
      else
      {
         println("execute listReplicaGroups happen error , e =" +e ) ;
         throw e ;
      }
   }
}

/* *****************************************************************************
@discription: create collection space
@author: Jianhui Xu
@parameter
   ignoreExisted: default = false, value: true/false
   message: user define message, default:""
   options: create CS specify options, default:"";[by  xiaojun Hu ]
           exp : {"Domain":"domName"}
           
***************************************************************************** */
function commCreateCS( db, csName, ignoreExisted, message, options )
{
   ++funcCommCreateCSTimes ;
   if ( ignoreExisted == undefined ) { ignoreExisted = false ; }
   if ( message == undefined ) { message = "" ; }
   if ( options == undefined ) { options = "" ; }
   try
   {
      if( "" == options )
         return db.createCS( csName ) ;
      else
         return db.createCS( csName, options ) ;
   }
   catch ( e )
   {
      if ( e != -33 || !ignoreExisted )
      {
         println( "commCreateCS[" + funcCommCreateCSTimes + "] Create collection space[" + csName + "] failed: " + e + ", message: " + message ) ;
         throw e ;
      }
   }
   // get collection space object
   try
   {
      return db.getCS( csName ) ;
   }
   catch ( e )
   {
      println( "commCreateCS[" + funcCommCreateCSTimes + "] Get existed collection space[" + csName + "] failed: " + e + ",message: " + message  ) ;
      throw e ;
   }
}

/* *****************************************************************************
@discription: create collection
@author: Jianhui Xu
@parameter
   replSize: default = RSize().ReplSize(), value 0, 1, 2,...
   compressed: default = true, value: true/false
   autoCreateCS: default = true, value: true/false
   ignoreExisted: default = false, value: true/false
   message: default = "", value: user defined message string
***************************************************************************** */
function commCreateCL( db, csName, clName, replSize, compressed, autoCreateCS, ignoreExisted, message )
{
   ++funcCommCreateCLTimes ;
   if ( replSize == undefined || replSize < 0 ) { replSize = new RSize( csName ).ReplSize( db ) ; }
   if ( compressed == undefined ) { compressed = true ; }
   if ( autoCreateCS == undefined ) { autoCreateCS = true ; }
   if ( ignoreExisted == undefined ) { ignoreExisted = false ; }
   if ( message == undefined ) { message = ""; }

   if ( autoCreateCS )
   {
      commCreateCS( db, csName, true, "commCreateCL auto to create collection space" ) ;
   }

   try
   {
      return eval( 'db.'+csName+'.createCL("' + clName + '", {"ReplSize":' + replSize +', "Compressed":' + compressed +'})' ) ;
   }
   catch( e )
   {
      if ( e != -22 || !ignoreExisted )
      {
         println( "commCreateCL[" + funcCommCreateCLTimes + "] create collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message ) ;
         throw e ;
      }
   }
   //get collection
   try
   {
      return eval( 'db.' +csName+'.getCL("' +clName+ '")' ) ;
   }
   catch ( e )
   {
      println( "commCreateCL[" + funcCommCreateCLTimes + "] get collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message ) ;
      throw e ;
   }

}

/* *****************************************************************************
@discription: create collection by user option
@author: Jianhui Xu
@parameter
   optionObj: option object, default {}
   compressed: default = true, value: true/false
   autoCreateCS: default = true, value: true/false
   ignoreExisted: default = false, value: true/false
   message: default = "", value: user defined message string
***************************************************************************** */
function commCreateCLByOption( db, csName, clName, optionObj, autoCreateCS, ignoreExisted, message )
{
   ++funcCommCreateCLOptTimes ;
   if ( optionObj == undefined ) { optionObj = {} ; }
   if ( autoCreateCS == undefined ) { autoCreateCS = true ; }
   if ( ignoreExisted == undefined ) { ignoreExisted = false ; }
   if ( message == undefined ) { message = ""; }

   if ( typeof( optionObj ) != "object" )
   {
      throw "commCreateCLByOption: optionObj is not object" ;
   }
   var csObj ;
   if ( autoCreateCS )
   {
      csObj = commCreateCS( db, csName, true, "commCreateCLByOption auto to create collection space" ) ;
   }
   else
   {
      try
      {
         csObj = db.getCS( csName ) ;
      }
      catch( e )
      {
         println( "commCreateCLByOption[" + funcCommCreateCLOptTimes + "] get collection space[" + csName + "] failed: " + e ) ;
         throw e ;
      }
   }

   try
   {
      return csObj.createCL( clName, optionObj ) ;
   }
   catch( e )
   {
      if ( e != -22 || !ignoreExisted )
      {
         println( "commCreateCLByOption[" + funcCommCreateCLOptTimes + "] create collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message ) ;
         throw e ;
      }
   }
   //get collection
   try
   {
      return eval( 'db.' +csName+'.getCL("' +clName+ '")' ) ;
   }
   catch ( e )
   {
      println( "commCreateCLByOption[" + funcCommCreateCLOptTimes + "] get collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message ) ;
      throw e ;
   }
}

/* *****************************************************************************
@discription: drop collection space
@author: Jianhui Xu
@parameter
   ignoreNotExist: default = true, value: true/false
   message: default = ""
***************************************************************************** */
function commDropCS( db, csName, ignoreNotExist, message )
{
   ++funcCommDropCSTimes ;
   if ( ignoreNotExist == undefined ) { ignoreNotExist = true ; }
   if ( message == undefined ) { message = "" ; }

   try
   {
      db.dropCS( csName ) ;
   }
   catch( e )
   {
      if ( e == -34 && ignoreNotExist )
      {
         // think right
      }
      else
      {
         println( "commDropCS[" + funcCommDropCSTimes + "] Drop collection space[" + csName + "] failed: " + e + ",message: " + message ) ;
         throw e ;
      }
   }
}

/* *****************************************************************************
@discription: drop collection
@author: Jianhui Xu
@parameter
   ignoreCSNotExist: default = true, value: true/false
   ignoreCLNotExist: default = true, value: true/false
   message: default = ""
***************************************************************************** */
function commDropCL( db, csName, clName, ignoreCSNotExist, ignoreCLNotExist, message )
{
   ++funcCommDropCLTimes ;
   if ( message == undefined ) { message = ""; }
   if ( ignoreCSNotExist == undefined ) { ignoreCSNotExist = true ; }
   if ( ignoreCLNotExist == undefined ) { ignoreCLNotExist = true ; }

   try
   {
      eval( 'db.' +csName+ '.dropCL("' +clName+ '")' ) ;
   }
   catch( e )
   {
      if ( ( e == -34 && ignoreCSNotExist ) || ( e == -23 && ignoreCLNotExist ) )
      {
         // think right
      }
      else
      {
         println( "commDropCL[" + funcCommDropCLTimes + "] Drop collection[" + csName + "." + clName + "] failed: " + e + ",message: " + message ) ;
         throw e ;
      }
   }
}

/* *****************************************************************************
@discription: create index
@author: Jianhui Xu
@parameter
   indexDef: index define object
   isUnique: true/false, default is false
   ignoreExist: default is false
***************************************************************************** */
function commCreateIndex( cl, name, indexDef, isUnique, ignoreExist )
{
   if ( isUnique == undefined ) { isUnique = false ; }
   if ( ignoreExist == undefined ) { ignoreExist = false ; }
   
   if ( typeof( indexDef ) != "object" )
   {
      throw "commCreateIndex: indexDef is not object" ;
   }

   try
   {
      cl.createIndex( name, indexDef, isUnique ) ;
   }
   catch( e )
   {
      println( "commCreateIndex: create index[" + name + "] failed: " + e ) ;
      if ( ignoreExist && ( e == -46 || e == -247 ) )
      {
         // ok
      }
      else
      {
         throw e ;
      }
   }
}

function commDropIndex( cl, name, ignoreNotExist )
{
   if ( ignoreNotExist == undefined ) { ignoreNotExist = false ; }
   
   try
   {
      cl.dropIndex( name ) ;
   }
   catch( e )
   {      
      if ( ignoreNotExist && e == -47 )
      {
         // ok
      }
      else
      {
         println( "commDropIndex: drop index[" + name + "] failed: " + e ) ;
         throw e ;
      }
   }
}

/* *****************************************************************************
@discription: check index
@author: Jianhui Xu
@parameter
   exist: true/false, if true check index exist, else check index not exist, default is true
   timeout: default 10 secs
***************************************************************************** */
function commCheckIndex( cl, name, exist, timeout )
{
   if ( exist == undefined ) { exist = true ; }
   if ( timeout == undefined ) { timeout = 10 ;}

   var timecount = 0 ;
   while ( true )
   {
      try
      {
         try
         {
            var tmpInfo = cl.getIndex( name ) ;
         }
         catch ( e )
         {
            tmpInfo = undefined ;
         }
         //var tmpInfo = cl.getIndex( name ) ;
         if ( ( exist && tmpInfo == undefined ) ||
              ( !exist && tmpInfo != undefined ) )
         {
            if ( timecount < timeout )
            {
               ++timecount ;
               sleep( 1000 ) ;
               continue ;
            }
            println( "commCheckIndex: check index[" + name + "] time out" ) ;
            throw "check index time out" ;
         }

         if ( tmpInfo != undefined )
         {
            var tmpObj =  eval( "(" + tmpInfo.toString() + ")" ) ;
            if ( tmpObj.IndexDef.name != name )
            {
               println("commCheckIndex: get index name[" + tmpObj.IndexDef.name + "] is not the same with name[" + name + "]" ) ;
               println( tmpInfo ) ;
               throw "check name error" ;
            }
         }
         break ;
      }
      catch( e )
      {
         println( "commCheckIndex: get index[" + name + "] failed: " + e ) ;
         throw "get index failed" ;
      }
   }
}

/* *****************************************************************************
@discription: check whether enable transaction function
@author: Jianhui Xu
***************************************************************************** */
function commIsTransEnabled( db )
{
   var isTrans = false ;
   var COMMTMPCS = COMMCSNAME + "_tmp" ;
   var COMMTMPCL = COMMCLNAME + "_tmp" ;
   var tmpCL = commCreateCL( db, COMMTMPCS, COMMTMPCL, 1, false, true, true, "Judge transaction create collection" ) ;
   try
   {
      db.transBegin() ;
      //tmpCL.update( {$unset:{a:1}}, {a:{$exists:0}} ) ; // do nothing
      tmpCL.remove() ;   // if transaction don't turn on, will throw error: -253
      isTrans = true ;
      db.transCommit() ;
      commDropCS( db, COMMTMPCS, false, "drop CS in transation temp" )
   }
   catch( e )
   {
      commDropCS( db, COMMTMPCS, true, "drop CS in transation temp" )
      if ( e == -253 )
      {
      }
      else
      {
         println( "execute transBegin happen error, e = " + e ) ;
      }
   }
   // clear when all use cases finished
   return isTrans ;
}

/* *****************************************************************************
@discription: print object function
@author: Jianhui Xu
***************************************************************************** */
function commPrintIndent( str, deep, withRN )
{
   if ( undefined == deep ) { deep = 0 ; }
   if ( undefined == withRN ) { withRN = true ; }
   for ( var i = 0 ; i < 3*deep; ++i )
   {
      print( " " ) ;
   }
   if ( withRN )
   {
      println( str ) ;
   }
   else
   {
      print( str ) ;
   }
}

function commPrint( obj, deep )
{
   var isArray = false ;
   if ( typeof( obj ) != "object" )
   {
      println( obj ) ;
      return ;
   }
   if ( undefined == deep ) { deep = 0 ; }
   if ( typeof( obj.length ) == "number" ) { isArray = true ; }

   if ( 0 == deep )
   {
      if ( !isArray )
      {
         commPrintIndent( "{", deep, false ) ;
      }
      else
      {
         commPrintIndent( "[", deep, false ) ;
      }
   }

   var i = 0 ;
   for ( var p in obj )
   {
      if ( i == 0 ) { commPrintIndent( "", 0, true ); }
      else if ( i > 0 ) { commPrintIndent( ",", 0, true ) ; }
      ++i ;

      if ( typeof( obj[p] ) == "object" )
      {
         if ( typeof( obj[p].length ) == "undefined" )
         {
            if ( !isArray )
            {
               commPrintIndent( p + ": {", deep + 1, false ) ;
            }
            else
            {
               commPrintIndent( "{", deep + 1, false ) ;
            }
         }
         else if ( !isArray )
         {
            commPrintIndent( p + ": [", deep + 1, false ) ;
         }
         else
         {
            commPrintIndent( "[", deep + 1, false ) ;
         }
         commPrint( obj[p], deep + 1 ) ;
      }
      else if ( typeof( obj[p] ) == "function" )
      {
         // not print
      }
      else if ( typeof( obj[p] ) == "string" )
      {
         if ( !isArray )
         {
            commPrintIndent( p + ': "' + obj[p] + '"', deep + 1, false ) ;
         }
         else
         {
            commPrintIndent( '"' + obj[p] + '"', deep + 1, false ) ;
         }
      }
      else
      {
         if ( !isArray )
         {
            commPrintIndent( p + ': ' + obj[p], deep + 1, false ) ;
         }
         else
         {
            commPrintIndent( obj[p], deep + 1, false ) ;
         }
      }
   }
   if ( i == 0 )
   {
      if ( !isArray )
      {
         commPrintIndent( "}", 0, false ) ;
      }
      else
      {
         commPrintIndent( "]", 0, false ) ;
      }
   }
   else
   {
      commPrintIndent( "", 0, true ) ;
      if ( !isArray )
      {
         commPrintIndent( "}", deep, false ) ;
      }
      else
      {
         commPrintIndent( "]", deep ,false ) ;
      }
   }
   if ( deep == 0 ) { commPrintIndent( "", deep, true ) } ;
}

/* ******************************************************************************
@description : get collection groups
@author : xiaojun Hu
@parameter:
   clname: collection name, such as : "foo.bar"
@return array[] ex:
   array[0] group1
   ...
***************************************************************************** */
function commGetCLGroups( db, clName )
{
   if ( typeof(clName) != "string" || clName.length == 0 )
   {
      throw "commGetCLGroups: Invalid clName parameter or clName is empty" ;
   }
   if ( commIsStandalone( db ) )
   {
      return new Array() ;
   }

   var tmpArray = new Array() ;
   var tmpInfo ;
   try
   {
      tmpInfo = db.snapshot(8).toArray() ;
   }
   catch( e )
   {
      println( "commGetCLGroups: snapshot collection space failed: " + e ) ;
      throw e ;
   }
   for ( var i = 0 ; i < tmpInfo.length ; ++i )
   {
      var tmpObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( tmpObj.Name != clName )
      {
         continue ;
      }
      for ( var j = 0 ; j < tmpObj.CataInfo.length; ++j )
      {
         tmpArray.push( tmpObj.CataInfo[j].GroupName ) ;
      }
   }
   return tmpArray ;
}

/* *****************************************************************************
@discription: get collection space groups
@author: Jianhui Xu
@parameter:
   csname: collection space name
@return array[] ex:
   array[0] group1
   ...
***************************************************************************** */
function commGetCSGroups( db, csname )
{
   if ( typeof(csname) != "string" || csname.length == 0 )
   {
      throw "commGetCSGroups: Invalid csname parameter or csname is empty" ;
   }
   if ( commIsStandalone( db ) )
   {
      return new Array() ;
   }

   var tmpArray = new Array() ;
   var tmpInfo ;
   try
   {
      tmpInfo = db.snapshot(5).toArray() ;
   }
   catch( e )
   {
      println( "commGetCSGroups: snapshot collection space failed: " + e ) ;
      throw e ;
   }

   for ( var i = 0 ; i < tmpInfo.length; ++i )
   {
      var tmpObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( tmpObj.Name != csname )
      {
         continue ;
      }
      for ( var j = 0 ; j < tmpObj.Group.length; ++j )
      {
         tmpArray.push( tmpObj.Group[j] ) ;
      }
   }
   return tmpArray ;
}

/* *****************************************************************************
@discription: get all groups
@author: Jianhui Xu
@parameter:
   filter: group name filter
   exceptCata : default true
   exceptCoord: default true
@return array[][] ex:
        [0]
           [0] {"GroupName":"XXXX", "GroupID":XXXX, "Status":XX, "Version":XX, "Role":XX, "PrimaryNode":XXXX, "Length":XXXX, "PrimaryPos":XXXX}
           [1] {"HostName":"XXXX", "dbpath":"XXXX", "svcname":"XXXX", "NodeID":XXXX}
           [N] ...
        [N]
           ...
***************************************************************************** */
function commGetGroups( db, print, filter, exceptCata, exceptCoord, exceptSpare )
{
   if ( filter == undefined ) { filter = "" ; }
   if ( undefined == print ) { print = false ; }
   if ( undefined == exceptCata ) { exceptCata = true ; }
   if ( undefined == exceptCoord ) { exceptCoord = true ;}
   if ( undefined == exceptSpare ) { exceptSpare = true ;}

   var tmpArray = new Array() ;
   var tmpInfo ;
   try
   {
      tmpInfo = db.listReplicaGroups().toArray() ;
   }
   catch( e )
   {
      if ( e == -159 )
      {
         return tmpArray ;
      }
      else
      {
         if( true == print )
            println( "commGetGroups failed: " + e ) ;
         throw e ;
      }
   }

   var index = 0 ;
   for ( var i = 0 ; i < tmpInfo.length; ++i )
   {
      var tmpObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( filter.length != 0 && tmpObj.GroupName.indexOf( filter, 0 ) == -1 )
      {
         continue ;
      }
      if ( true == exceptCata && tmpObj.GroupID == CATALOG_GROUPID )
      {
         continue ;
      }
      if ( true == exceptCoord && tmpObj.GroupID == COORD_GROUPID )
      {
         continue ;
      }
      if ( true == exceptSpare && tmpObj.GroupID == SPARE_GROUPID )
      {
         continue ;
      }
      tmpArray[index] = Array() ;
      tmpArray[index][0] = new Object() ;
      tmpArray[index][0].GroupName = tmpObj.GroupName ;         // GroupName
      tmpArray[index][0].GroupID = tmpObj.GroupID ;             // GroupID
      tmpArray[index][0].Status = tmpObj.Status ;               // Status
      tmpArray[index][0].Version = tmpObj.Version ;             // Version
      tmpArray[index][0].Role = tmpObj.Role ;                   // Role
      if ( typeof(tmpObj.PrimaryNode) == "undefined" )
      {
         tmpArray[index][0].PrimaryNode = -1 ;
      }
      else
      {
         tmpArray[index][0].PrimaryNode = tmpObj.PrimaryNode ;     // PrimaryNode
      }
      tmpArray[index][0].PrimaryPos = 0 ;                       // PrimaryPos

      var tmpGroupObj = tmpObj.Group ;
      tmpArray[index][0].Length = tmpGroupObj.length ;          // Length

      for ( var j = 0 ; j < tmpGroupObj.length; ++j )
      {
         var tmpNodeObj = tmpGroupObj[j] ;
         tmpArray[index][j+1] = new Object() ;
         tmpArray[index][j+1].HostName = tmpNodeObj.HostName ;           // HostName
         tmpArray[index][j+1].dbpath = tmpNodeObj.dbpath ;               // dbpath
         tmpArray[index][j+1].svcname = tmpNodeObj.Service[0].Name ;     // svcname
         tmpArray[index][j+1].NodeID = tmpNodeObj.NodeID ;

         if ( tmpNodeObj.NodeID == tmpObj.PrimaryNode )
         {
            tmpArray[index][0].PrimaryPos = j+1 ;                        // PrimaryPos
         }
      }
      ++index ;
   }

   return tmpArray ;
}


/* *****************************************************************************
@discription: get the number of groups
@author: Jianhua Li
@parameter:
   filter: group name filter
   exceptCata : default true
   exceptCoord: default true
@return integer
***************************************************************************** */
function commGetGroupsNum( db, print, filter, exceptCata, exceptCoord, exceptSpare )
{
   if ( filter == undefined ) { filter = "" ; }
   if ( undefined == print ) { print = false ; }
   if ( undefined == exceptCata ) { exceptCata = true ; }
   if ( undefined == exceptCoord ) { exceptCoord = true ;}
   if ( undefined == exceptSpare ) { exceptSpare = true ;}

   var tmpInfo ;
   var num = 0 ;
   try
   {
      tmpInfo = db.listReplicaGroups().toArray() ;
   }
   catch( e )
   {
      if ( e == -159 )
      {
         return num ;
      }
      else
      {
         if( true == print )
            println( "commGetGroups failed: " + e ) ;
         throw e ;
      }
   }

   for ( var i = 0 ; i < tmpInfo.length; ++i )
   {
      var tmpObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( filter.length != 0 && tmpObj.GroupName.indexOf( filter, 0 ) == -1 )
      {
         continue ;
      }
      if ( true == exceptCata && tmpObj.GroupID == CATALOG_GROUPID )
      {
         continue ;
      }
      if ( true == exceptCoord && tmpObj.GroupID == COORD_GROUPID )
      {
         continue ;
      }
      if ( true == exceptSpare && tmpObj.GroupID == SPARE_GROUPID )
      {
         continue ;
      }
      ++num ;
   }

   return num ;
}


/* *****************************************************************************
@discription: get cs and cl
@author: Jianhui Xu
@parameter:
   csfilter : collection space filter
   clfilter : collection filter
@return array[] ex:
        [0] {"cs":"XXXX", "cl":["XXXX", "XXXX",...] }
        [N] ...
***************************************************************************** */
function commGetCSCL( db, csfilter, clfilter )
{
   if ( csfilter == undefined ) { csfilter = ""; }
   if ( clfilter == undefined ) { clfilter = ""; }

   var tmpCSCL = new Array() ;
   var tmpInfo ;
   try
   {
      tmpInfo = db.snapshot( 5 ).toArray() ;
   }
   catch( e )
   {
      println( "commGetCSCL failed: " + e ) ;
      return tmpCSCL ;
   }
   
   var m = 0 ;
   for ( var i = 0 ; i < tmpInfo.length; ++i )
   {
      var tmpObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( csfilter.length != 0 && tmpObj.Name.indexOf( csfilter, 0 ) == -1 )
      {
         continue ;
      }
      tmpCSCL[ m ] = new Object() ;
      tmpCSCL[ m ].cs = tmpObj.Name ;
      tmpCSCL[ m ].cl = new Array() ;
      
      var tmpCollection = tmpObj.Collection ;
      for ( var j = 0 ; j < tmpCollection.length; ++j )
      {
         if ( clfilter.length != 0 && tmpCollection[ j ].Name.indexOf( clfilter, 0 ) == -1 )
         {
            continue ;
         }
         tmpCSCL[ m ].cl.push( tmpCollection[ j ].Name ) ;
      }
      ++m ;
   }
   return tmpCSCL ;
}

/* *****************************************************************************
@discription: get backup
@author: Jianhui Xu
@parameter:
   filter : backup name filter
   path : backup path
   isSubDir: true/false, default is false
   condObj : condition object
   grpNameArray : group name array
@return array[] ex:
        [0] "XXXX"
        [N] ...
***************************************************************************** */
function commGetBackups( db, filter, path, isSubDir, cond, grpNameArray )
{
   if ( filter == undefined ) { filter = "" ; }
   if ( path == undefined ) { path = "" ; }
   if ( isSubDir == undefined ) { isSubDir = false ; }
   if ( cond == undefined ) { cond = {} ; }
   if ( grpNameArray == undefined ) { grpNameArray = new Array() ; }

   if ( typeof( cond ) != "object" )
   {
      throw "commGetBackups: cond is not a object" ;
   }
   if ( typeof( grpNameArray ) != "object" )
   {
      throw "commGetBackups: grpNameArray is not a array" ;
   }

   var tmpBackup = new Array() ;
   var tmpInfo ;
   try
   {
      if ( path.length == 0 )
      {
         tmpInfo = db.listBackup({GroupName:grpNameArray}).toArray() ;
      }
      else
      {
         tmpInfo = db.listBackup( {Path:path, IsSubDir:isSubDir, GroupName:grpNameArray} ).toArray() ;
      }
   }
   catch( e )
   {
      println( "commGetBackups failed: " + e ) ;
      return tmpBackup ;
   }

   for ( var i = 0 ; i < tmpInfo.length; ++i )
   {
      var tmpBackupObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( typeof( tmpBackupObj.Name ) == "undefined" )
      {
         continue ;
      }
      if ( filter.length != 0 && tmpBackupObj.Name.indexOf( filter, 0 ) == -1 )
      {
         continue ;
      }
      var exist = false ;
      for ( var j = 0 ; j < tmpBackup.length; ++j )
      {
         if ( tmpBackupObj.Name == tmpBackup[j] )
         {
            exist = true ;
            break ;
         }
      }
      if ( exist )
      {
         continue ;
      }
      tmpBackup.push( tmpBackupObj.Name ) ;
   }
   return tmpBackup ;
}

/* *****************************************************************************
@discription: get procedure
@author: Jianhui Xu
@parameter:
   filter : procedure name filter
@return array[] ex:
        [0] "XXXX"
        [N] ...
***************************************************************************** */
function commGetProcedures( db, filter )
{
   if ( filter == undefined ) { filter = "" ; }

   var tmpProcedure = new Array() ;
   var tmpInfo ;
   try
   {
      tmpInfo = db.listProcedures().toArray() ;
   }
   catch( e )
   {
      println( "commGetProcedures failed: " + e ) ;
      return tmpProcedure ;
   }

   for ( var i = 0 ; i < tmpInfo.length; ++i )
   {
      var tmpProcedureObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( typeof( tmpProcedureObj.name ) == "undefined" )
      {
         continue ;
      }
      if ( filter.length != 0 && tmpProcedureObj.name.indexOf( filter, 0 ) == -1 )
      {
         continue ;
      }
      tmpProcedure.push( tmpProcedureObj.name ) ;
   }
   return tmpProcedure ;
}

/* *****************************************************************************
@discription: get domains
@author: Jianhui Xu
@parameter:
   filter : domain name filter
@return array[] ex:
        [0] "XXXX"
        [N] ...
***************************************************************************** */
function commGetDomains( db, filter )
{
   if ( filter == undefined ) { filter = "" ; }

   var tmpDomain = new Array() ;
   var tmpInfo ;
   try
   {
      tmpInfo = db.listDomains().toArray() ;
   }
   catch( e )
   {
      println( "commGetDomains failed: " + e ) ;
      return tmpDomain ;
   }

   for ( var i = 0 ; i < tmpInfo.length; ++i )
   {
      var tmpDomainObj = eval( "(" + tmpInfo[i] + ")" ) ;
      if ( typeof( tmpDomainObj.Name ) == "undefined" )
      {
         continue ;
      }
      if ( filter.length != 0 && tmpDomainObj.Name.indexOf( filter, 0 ) == -1 )
      {
         continue ;
      }
      tmpDomain.push( tmpDomainObj.Name ) ;
   }
   return tmpDomain ;
}

/* *****************************************************************************
@discription: check nodes function
@author: Jianhui Xu
@return array[] ex:
        [0] {"HostName":"XXXX", "dbpath":"XXXX", "svcname":"XXXX", "NodeID":XXXX}
        [N] ...
***************************************************************************** */
function commCheckNodes( groups )
{
   var tmpFailedGrp = new Array() ;
   for ( var i = 0 ; i < groups.length; ++i )
   {
      for ( var j = 1; j < groups[i].length; ++j )
      {
         var tmpDB ;
         try
         {
            tmpDB = new Sdb( groups[i][j].HostName, groups[i][j].svcname ) ;
            tmpDB.close() ;
         }
         catch ( e )
         {
            tmpFailedGrp.push( groups[i][j] ) ;
         }
      }
   }
   return tmpFailedGrp ;
}

/* *****************************************************************************
@discription: check business right function
@author: Jianhui Xu
@return array[][] ex:
        [0]
           [0] {"GroupName":"XXXX", "GroupID":XXXX, "PrimaryNode":XXXX, "ConnCheck":t/f, "PrimaryCheck":t/f, "LSNCheck":t/f, "ServiceCheck":t/f, "DiskCheck":t/f }
           [1] {"HostName":"XXXX", "svcname":"XXXX", "NodeID":XXXX, "Connect":t/f, "IsPrimay":t/f, "LSN":XXXX, "ServiceStatus":t/f, "FreeSpace":XXXX }
           [N] ...
        [N]
           ...
***************************************************************************** */
function commCheckBusiness( groups, checkLSN, diskThreshold )
{
   if ( checkLSN == undefined ) { checkLSN = false ;}
   if ( diskThreshold == undefined ) { diskThreshold = 134217728 ; }
   var tmpCheckGrp = new Array() ;
   var grpindex = 0 ;
   var ndindex = 1 ;
   var error = false ;

   for ( var i = 0 ; i < groups.length; ++i )
   {
      error = false ;

      tmpCheckGrp[ grpindex ] = new Array() ;
      tmpCheckGrp[ grpindex ][0] = new Object() ;
      tmpCheckGrp[ grpindex ][0].GroupName = groups[i][0].GroupName ;
      tmpCheckGrp[ grpindex ][0].GroupID = groups[i][0].GroupID ;
      tmpCheckGrp[ grpindex ][0].PrimaryNode = groups[i][0].PrimaryNode ;
      tmpCheckGrp[ grpindex ][0].ConnCheck = true ;
      tmpCheckGrp[ grpindex ][0].PrimaryCheck = true ;
      tmpCheckGrp[ grpindex ][0].LSNCheck = true ;
      tmpCheckGrp[ grpindex ][0].ServiceCheck = true ;
      tmpCheckGrp[ grpindex ][0].DiskCheck = true ;

      // if no primary
      if ( groups[i][0].PrimaryNode == -1 )
      {
         error = true ;
         tmpCheckGrp[ grpindex ][0].PrimaryCheck = false ;
      }
      
      for ( var j = 1; j < groups[i].length; ++j )
      {
         ndindex = j ;
         tmpCheckGrp[grpindex][ndindex] = new Object() ;
         tmpCheckGrp[grpindex][ndindex].HostName = groups[i][j].HostName ;
         tmpCheckGrp[grpindex][ndindex].svcname = groups[i][j].svcname ;
         tmpCheckGrp[grpindex][ndindex].NodeID = groups[i][j].NodeID ;
         tmpCheckGrp[grpindex][ndindex].Connect = true ;
         tmpCheckGrp[grpindex][ndindex].IsPrimay = false ;
         tmpCheckGrp[grpindex][ndindex].LSN = -1 ;
         tmpCheckGrp[grpindex][ndindex].ServiceStatus = true ;
         tmpCheckGrp[grpindex][ndindex].FreeSpace = -1 ;

         var tmpDB ;
         // check connection
         try
         {
            tmpDB = new Sdb( groups[i][j].HostName, groups[i][j].svcname ) ;
         }
         catch ( e )
         {
            error = true ;
            tmpCheckGrp[ grpindex ][0].ConnCheck = false ;
            tmpCheckGrp[grpindex][ndindex].Connect = false ;
            continue ;
         }
         // check primary and lsn
         var tmpSysInfo ;
         try
         {
            tmpSysInfo = tmpDB.snapshot( 6 ).toArray() ;
         }
         catch( e )
         {
            error = true ;
            tmpCheckGrp[ grpindex ][0].ConnCheck = false ;
            tmpCheckGrp[grpindex][ndindex].Connect = false ;
            tmpDB.close() ;
            continue ;
         }
         //delete tmpCheckGrp[grpindex][ndindex].Connect ;
         // check primary
         var tmpSysInfoObj = eval( "(" + tmpSysInfo[0] + ")" ) ;
         if ( tmpSysInfoObj.IsPrimary == true )
         {
            tmpCheckGrp[grpindex][ndindex].IsPrimay = true ;
            if ( groups[i][0].PrimaryNode != groups[i][j].NodeID )
            {
               error = true ;
               tmpCheckGrp[ grpindex ][0].PrimaryCheck = false ;
            }
         }
         else
         {
            if ( groups[i][0].PrimaryNode == groups[i][j].NodeID )
            {
               error = true ;
               tmpCheckGrp[ grpindex ][0].PrimaryCheck = false ;
            }
            //delete tmpCheckGrp[grpindex][ndindex].IsPrimay ;
         }
         // check ServiceStatus
         if ( tmpSysInfoObj.ServiceStatus == false )
         {
            error = true ;
            tmpCheckGrp[ grpindex ][ndindex].ServiceStatus = false ;
            tmpCheckGrp[ grpindex ][0].ServiceCheck = false ;
         }
         // check disk
         tmpCheckGrp[grpindex][ndindex].FreeSpace = tmpSysInfoObj.Disk.FreeSpace ;
         if ( tmpCheckGrp[grpindex][ndindex].FreeSpace < diskThreshold )
         {
            error = true ;
            tmpCheckGrp[ grpindex ][0].DiskCheck = false ;
         }
         // check LSN
         tmpCheckGrp[ grpindex ][ndindex].LSN = tmpSysInfoObj.CurrentLSN.Offset ;
         if ( checkLSN && j > 1 && tmpCheckGrp[ grpindex ][ndindex].LSN != tmpCheckGrp[ grpindex ][1].LSN  )
         {
           // error = true ;
            tmpCheckGrp[ grpindex ][0].LSNCheck = false ;
         }
         
         tmpDB.close() ;
      }
      
      if ( error == true )
      {
         ++grpindex ;
      }
   }
   // no error
   if ( error == false )
   {
      if ( grpindex == 0 )
      {
         tmpCheckGrp = new Array() ;
      }
      else
      {
         delete tmpCheckGrp[grpindex] ;
      }
   }
   return tmpCheckGrp ;
}

/* *****************************************************************************
@discription: check business right function
@author: xiaojun Hu
@return array[][] ex:
        [0]
           [0] {"GroupName":"XXXX", "GroupID":XXXX, "PrimaryNode":XXXX, "ConnCheck":t/f, "PrimaryCheck":t/f, "LSNCheck":t/f, "ServiceCheck":t/f, "DiskCheck":t/f }
           [1] {"HostName":"XXXX", "svcname":"XXXX", "NodeID":XXXX, "Connect":t/f, "IsPrimay":t/f, "LSN":XXXX, "ServiceStatus":t/f, "FreeSpace":XXXX }
           [N] ...
        [N]
           ...
***************************************************************************** */
function commGetInstallPath()
{
   try
   {
      var cmd = new Cmd() ;
      try
      {
         var installFile = cmd.run( "sed -n '3p' /etc/default/sequoiadb" ).split( "=" ) ;
         var installPath = installFile[1].split( "\n" ) ;
         var InstallPath = installPath[0] ;
      }
      catch( e )
      {
         if( 2 == e )
         {
            var local = cmd.run( "pwd" ).split( "\n" ) ;
            var LocalPath = local[0] ;
            var folder = cmd.run( 'ls ' + LocalPath ).split( '\n' ) ;
            var fcnt = 0 ;
            for( var i = 0 ; i < folder.length ; ++i )
            {
               if( "bin" == folder[i] || "SequoiaDB" == folder[i] ||
                   "testcase" == folder[i] )
               {
                  fcnt++ ;
               }
            }
            if( 2 <= fcnt )
               InstallPath = LocalPath ;
            else
               throw "Don'tGetLocalPath" ;
         }
         else
            throw e ;
      }
   }
   catch( e )
   {
      println( "failed to get install path[common]: " + e ) ;
      throw e ;
   }
   return InstallPath ;
}

/* *****************************************************************************
@Description: build exception
@author: wenjing wang
@parameter:
  funname  : funnction name
  e        : js exception
  operate  : current operation
  expectval: expect value
  realval  : real value
@return string:exception msg
***************************************************************************** */
function buildException(funname, e, operate, expectval, realval)
{
   if (undefined != operate &&
       undefined != expectval &&
       undefined != realval)
   {
      var message = "Exec " + operate + " \nExpect result: " + expectval + " \nReal result: " + realval;
      return funname + " throw: " + message;      
   }
   else
   {
      return funname + " unknown error expect: " + e;
   }
}

/* *****************************************************************************
@Description: The number or string is whether in Array
@author: xujianhui
@parameter:
   needle: the compared value, number or string
   hayStack : the array
@return string:true/false
***************************************************************************** */
function commInArray( needle, hayStack )
{
   type = typeof needle ;
   if ( type == 'string' || type == 'number' )
   {
      for ( var i in hayStack )
      {
         if ( hayStack[i] == needle ) return true ;
      }
      return false ;
   }
}

/**********************************************************************
@Description:  generate all kinds of types data randomly
@author:       Ting YU
@usage:        var rd = new commDataGenerator();
               1.var recs = rd.getRecords( 300, "int", ['a','b'] );
               2.var recs = rd.getRecords( 3, ["int", "string"] );
               3.rd.getValue("string");
***********************************************************************/
function commDataGenerator()
{
   this.getRecords =
   function( recNum, dataTypes, fieldNames )
   {
      return getRandomRecords( recNum, dataTypes, fieldNames );
   }
   
   this.getValue = 
   function( dataType )
   {
      return getRandomValue( dataType );
   }
   
   function getRandomRecords( recNum, dataTypes, fieldNames )
   {
      if( fieldNames === undefined ) { fieldNames = getRandomFieldNames(); }
      if( dataTypes.constructor !== Array ) { dataTypes = [ dataTypes ]; }
      
      var recs = [];
      for( var i = 0; i < recNum; i++ )
      {          
         // generate 1 record
         var rec = {};      
         for( var j in fieldNames )
         {  
            // generate 1 filed
            var filedName = fieldNames[j];
   
            var dataType = dataTypes[ parseInt( Math.random() * dataTypes.length ) ];  //randomly get 1 data type
            var filedVal =  getRandomValue( dataType );
            
            if( filedVal !== undefined ) rec[filedName] = filedVal;       
         } 
         
         recs.push( rec );
      }   
      return recs;
   }
   
   function getRandomValue( dataType )
   {
      var value = undefined;
      
      switch( dataType )
      {
         case "int":
            value = getRandomInt( -2147483648, 2147483647 );
            break;
         case "long":
            value = getRandomLong( -922337203685477600, 922337203685477600 );
            break;
         case "float":
            value = getRandomFloat( -999999, 999999 );
            break;
         case "string":
            value = getRandomString( 0, 20 );
            break; 
         case "OID":
            value = ObjectId();
            break;
         case "bool":
            value = getRandomBool();
            break; 
         case "date":
            value = getRandomDate();
            break;
         case "timestamp":
            value = getRandomTimestamp();
            break; 
         case "binary":
            value = getRandomBinary();
            break;
         case "regex":
            value = getRandomRegex();
            break; 
         case "object":
            value = getRandomObject();
            break;
         case "array":
            value = getRandomArray();
            break; 
         case "null":
            value = null;
            break;
         case "non-existed":
            break;                 
      }
      
      return value;
   }
   
   function getRandomFieldNames( minNum, maxNum )
   {
      if( minNum == undefined ) { minNum = 0; }
      if( maxNum == undefined ) { maxNum = 16; }
      
      var fieldNames = [];
      var fieldNum = getRandomInt( minNum, maxNum );
      
      for( var i = 0; i < fieldNum; i++ )
      {
         //get 1 field name
         var fieldName = "";
         var fieldNameLen = getRandomInt( 1, 9 );
         for( var j = 0; j < fieldNameLen; j++ )
         {
            //get 1 char
            var ascii = getRandomInt( 97, 123 ); // 'a'~'z'
            var c = String.fromCharCode( ascii );
            fieldName += c;
         }
         fieldNames.push( fieldName );
      }
      
      return fieldNames;
   }
   
   function getRandomInt( min, max ) // [min, max)
   {
      var range = max - min;
      var value = min + parseInt( Math.random() * range );
      return value;
   }
   
   function getRandomLong( min, max )
   {
      var value = getRandomInt( min, max );
      return NumberLong(value);
   }
   
   function getRandomFloat( min, max )
   {
      var range = max - min;
      var value = min + Math.random() * range;
      return value;
   }
   
   function getRandomString( minLen, maxLen ) //string length value locate in [minLen, maxLen)
   {
      var strLen = getRandomInt( minLen, maxLen );   
      var str = "";
      
      for( var i = 0; i < strLen; i++ )
      {
         var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
         var c = String.fromCharCode( ascii );
         str += c;
      }
      return str;
   }
   
   function getRandomBool()
   {
      var Bools = [ true, false ];
      var index = parseInt( Math.random() * Bools.length );  
      var value = Bools[ index ]; 
      
      return value;
   }
    
   function getRandomDate()
   {
      var sec = getRandomInt( -2208902400, 253402128000 ); //1900-01-02 ~ 9999-12-30
      var d = new Date( sec * 1000 );
      var dateVal = d.getFullYear() + '-' + (d.getMonth()+1) + '-' + d.getDate();
      
      var value = {"$date":dateVal};
      return value;
   }
     
   function getRandomTimestamp()
   {   
      var sec = getRandomInt( -2147397248, 2147397247 ); //1901-12-14-20.45.52 ~ 2038-01-18-03.14.07
      var d = new Date( sec * 1000 );
      
      var ns = getRandomInt( 0, 1000000 ).toString();
      if( ns.length < 6 )
      {
         var addZero = 6 - ns.length;
         for(var i = 0; i < addZero; i++ ) { ns = '0' + ns; }   
      }
      
      var timeVal = d.getFullYear() + '-' + (d.getMonth()+1) + '-' + d.getDate() + '-' +
                    d.getHours() + '.' +  d.getMinutes() + '.' + d.getSeconds() + '.' + ns;
      
      var value = {"$timestamp": timeVal};
      return value;
   }
   
   function getRandomBinary()
   {   
      var str = getRandomString( 1, 50 );

      var cmd = new Cmd();
      var binaryVal = cmd.run( "echo -n '" + str + "' | base64 -w 0" );
      
      var typeVal = getRandomInt( 0, 256 );
      typeVal = typeVal.toString();
      
      var value = {"$binary":binaryVal, "$type":typeVal};
      return value;
   }
   
   function getRandomRegex()
   {
      var opts = [ "i", "m", "x", "s" ];
      var index = parseInt( Math.random() * opts.length );  
      var optVal = opts[ index ]; 
      
      var regexVal = getRandomString( 1, 10 );
      
      var value = {"$regex":regexVal, "$options":optVal};
      return value;
   }
     
   function getRandomObject( n )
   { 
      var obj = {};   
      var dataTypes= [ "int", "long", "float", "string", "OID", "bool", "date", 
                       "timestamp", "binary", "regex", "object", "array", "null" ];
     
      var fieldNames = getRandomFieldNames( 1, 5 );
      
      for( var i in fieldNames )
      {
         var dataType = dataTypes[ parseInt( Math.random() * dataTypes.length ) ];  //randomly get 1 data type 
      
         var filedName = fieldNames[i]
         obj[filedName] = getRandomValue( dataType );
      }
      
      return obj;
   }
   
   function getRandomArray()
   {
      var arr = [];   
      var dataTypes= [ "int", "long", "float", "string", "OID", "bool", "date", 
                       "timestamp", "binary", "regex", "object", "array", "null" ];
                       
      var arrLen = getRandomInt( 1, 5 );  
      for( var i = 0; i < arrLen; i++ )
      {
         var dataType = dataTypes[ parseInt( Math.random() * dataTypes.length ) ];  //randomly get 1 data type 
         
         var elem = getRandomValue( dataType );
         arr.push( elem );
      }
      
      return arr;
   }
}

// common database connection
try
{
   var db = new Sdb(COORDHOSTNAME,COORDSVCNAME );
   var group = commGetGroups( db );
   if ( true == commIsStandalone( db ) || 1 == group.length )   // standalone or one group
   {
      db = new SecureSdb( COORDHOSTNAME, COORDSVCNAME );   // SSL
      println( "SSL connection" ) ;
   }
   var connSucess = false ;
}
catch ( e )
{
   if( -15 != e )
   {
      println( "Connect Failed in Common Function!" );
      connSucess = true ;
      throw e;
   }
   else
   {
      db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      println( "general connection" ) ;
   }
}

//example
//var db = new Sdb(COORDHOSTNAME,COORDSVCNAME ) ;

//var aa = new RSize( 'CSname' );
//println(aa.ReplSize());

