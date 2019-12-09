/*****************************************************************
@description:   sort object in array, rg:
                array.sort(compare("key1", compare("key2", compare("key3"))));       
@input:         key
                o: object1's key
                p: object2's key					 
******************************************************************/
function compare ( name, minor )
{
   try
   {
      return function( o, p )
      {
         var a, b;
         if( o && p && typeof o === 'object' && typeof p === 'object' )
         {
            a = o[name];
            b = p[name];
            if( a === b ) 
            {
               return typeof minor === 'function' ? minor( o, p ) : 0;
            }
            if( typeof a === typeof b ) 
            {
               return a < b ? -1 : 1;
            }
            return typeof a < typeof b ? -1 : 1;
         }
         else 
         {
            throw "ERROR";

         }
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 比较是否为json对象
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function isJson ( object ) 
{
   try
   {
      var isJson = object && typeof ( object ) == 'object' && Object.prototype.toString.call( object ).toLowerCase() == "[object object]";
      return isJson;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 比较2个对象是否相等
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function isObjEqual ( obj1, obj2 )
{
   try
   {
      if( typeof ( obj1 ) !== typeof ( obj2 ) ) return false;
      if( isJson( obj1 ) && isJson( obj2 ) )
      {
         var props1 = Object.getOwnPropertyNames( obj1 );
         var props2 = Object.getOwnPropertyNames( obj2 );
         if( props1.length !== props2.length ) return false;
         for( var i = 0; i < props1.length; i++ )
         {
            if( props1[i] !== props2[i] ) return false;
         }
      }

      for( var key in obj1 )
      {
         var oA = obj1[key];
         var oB = obj2[key];
         if( typeof ( oA ) !== typeof ( oB ) ) return false;
         if( isJson( oA ) && isJson( oB ) ) return isObjEqual( oA, oB );
         if( oA.hasOwnProperty( '$numberLong' ) && oB.hasOwnProperty( '$numberLong' ) && oA.$numberLong !== oB.$numberLong ) return false;
         if( oA.hasOwnProperty( '$decimal' ) && oB.hasOwnProperty( '$decimal' ) && oA.$decimal !== oB.$decimal ) return false;
         if( !oA.hasOwnProperty( '$numberLong' ) && !oB.hasOwnProperty( '$numberLong' )
            && !oA.hasOwnProperty( '$decimal' ) && !oB.hasOwnProperty( '$decimal' )
            && oA !== oB ) return false;
      }
      return true;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : json对象按照属性值排序
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function sortJsonKeys ( obj )
{
   try
   {
      var tmp = {};
      Object.keys( obj ).sort().forEach( function( k ) { tmp[k] = obj[k] } );
      return tmp;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 比较查询返回的结果（游标）与预期结果(数组)是否一致
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function checkRec ( rc, expRecs )
{
   try
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
         throw "expect lenth is " + expRecs.length + ", but act length is " + actRecs.length;
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
               println( "\nactual recs in cl= " + JSON.stringify( actRec ) + "\n\nexpect recs= " + JSON.stringify( expRec ) );
               throw "rec ERROR";
            }
         }
      }

      //check every records every fields,actRecs as compare source
      for( var i in actRecs )
      {
         var actRec = actRecs[i];
         var expRec = expRecs[i];

         for( var f in actRec )
         {
            if( f == "_id" )
            {
               continue;
            }
            if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
            {
               println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
               println( "\nactual record= " + JSON.stringify( actRec ) + "\n\nexpect record= " + JSON.stringify( expRec ) );
               throw "rec ERROR";
            }
         }
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 获取coord组内所有节点名
@return: array,例如：["localhost:11810","localhost:11820"]
@Modify list : 2018-10-15 zhaoyu init
*******************************************************************************/
function getCoordNodeNames ()
{
   var nodeNames = new Array();
   try
   {
      var rg = db.getCoordRG();
   } catch( e )
   {
      if( e == -159 )
      {
         return nodeNames;
      }
   }
   try
   {
      var details = rg.getDetail();
      while( details.next() )
      {
         var groups = details.current().toObj().Group;
         for( var i = 0; i < groups.length; i++ )
         {
            var hostName = groups[i].HostName;
            var service = groups[i].Service;
            for( var j = 0; j < service.length; j++ )
            {
               if( service[j].Type === 0 )
               {
                  var serviceName = service[j].Name;
                  break;
               }
            }
            nodeNames.push( hostName + ":" + serviceName );
         }
      }
      return nodeNames;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 获取分区组数
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function getDataGroupNames ()
{
   var dataGroupNames = new Array();
   try
   {
      var tmpInfo = db.listReplicaGroups().toArray();
   } catch( e )
   {
      if( e == -159 )
      {
         return dataGroupNames;
      }
   }

   try
   {
      for( var i = 0; i < tmpInfo.length; i++ )
      {
         var tmpObj = eval( "(" + tmpInfo[i] + ")" );
         if( tmpObj.GroupName === "SYSCatalogGroup" || tmpObj.GroupName === "SYSCoord" )
         {
            continue;
         }
         var dataGroupName = tmpObj.GroupName;
         dataGroupNames.push( dataGroupName );
      }
      return dataGroupNames;
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 获取集合的全局ID
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function getCLID ( csName, clName )
{
   try
   {
      var uniqueID = db.snapshot( 8, { Name: csName + "." + clName } ).next().toObj().UniqueID;
      return uniqueID;
   }
   catch( e )
   {
      throw new Error( e );
   }
}


/*******************************************************************************
@Description : 校验集合自增字段属性
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function checkAutoIncrementonCL ( csName, clName, expArr )
{
   try
   {
      for( var i = 0; i < expArr.length; i++ )
      {
         if( expArr[i].Generated == undefined ) { expArr[i].Generated = "default"; }
      }

      var autoIncrementArr = db.snapshot( 8, { Name: csName + "." + clName } ).next().toObj().AutoIncrement;
      if( autoIncrementArr.length !== expArr.length )
      {
         println( "act num:" + autoIncrementArr.length + "\nexp num:" + expArr.length );
         throw "check_autoIncrement_num_err";
      }
      autoIncrementArr.sort( compare( "Field" ) );
      expArr.sort( compare( "Field" ) );
      for( var i = 0; i < autoIncrementArr.length; i++ )
      {
         var tmpActObj = sortJsonKeys( autoIncrementArr[i] );
         delete tmpActObj.SequenceID;
         var tmpExpObj = sortJsonKeys( expArr[i] );
         if( !isObjEqual( tmpActObj, tmpExpObj ) )
         {
            println( "autoIncrementObj:" + JSON.stringify( tmpActObj ) + "\nexpObj:" + JSON.stringify( tmpExpObj ) );
            throw "check_autoIncrement_err";
         }
      }
   } catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 校验集合sequence属性
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function checkSequence ( sequenceName, expObj )
{
   try
   {
      if( expObj.Increment == undefined ) { expObj.Increment = 1; }
      if( expObj.StartValue == undefined ) { expObj.StartValue = 1; }
      if( expObj.MinValue == undefined ) { expObj.MinValue = 1; }
      if( expObj.MaxValue == undefined ) { expObj.MaxValue = { $numberLong: "9223372036854775807" }; }
      if( expObj.CacheSize == undefined ) { expObj.CacheSize = 1000; }
      if( expObj.AcquireSize == undefined ) { expObj.AcquireSize = 1000; }
      if( expObj.Cycled == undefined ) { expObj.Cycled = false; }
      if( expObj.CurrentValue == undefined ) { expObj.CurrentValue = 1; }

      var sequenceObj = db.snapshot( SDB_SNAP_SEQUENCES, { Name: sequenceName } ).next().toObj();
      delete sequenceObj._id;
      delete sequenceObj.Version;
      delete sequenceObj.Initial;
      delete sequenceObj.Internal;
      delete sequenceObj.Name;
      delete sequenceObj.SequenceID
      delete sequenceObj.ID

      var tmpActObj = sortJsonKeys( sequenceObj );
      var tmpExpObj = sortJsonKeys( expObj );

      if( !isObjEqual( tmpActObj, tmpExpObj ) )
      {
         println( "tmpActObj:" + JSON.stringify( tmpActObj ) + "\ntmpExpObj:" + JSON.stringify( tmpExpObj ) );
         throw "check_sequence_err";
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 校验从节点返回记录数
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function checkCountFromNode ( groupName, csName, clName, expCount )
{
   try
   {
      var rg = db.getRG( groupName );
      var data = rg.getMaster().connect();
      var dataCL = data.getCS( csName ).getCL( clName );
      var count = dataCL.count();
      if( parseInt( count ) !== expCount )
      {
         println( "expect count: " + expCount + "\nactual count:" + parseInt( count ) );
         throw "data count err";
      }
   }
   catch( e )
   {
      throw new Error( e );
   }
}

/*******************************************************************************
@Description : 插入其他不支持类型的记录
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function insertOtherTypeDatas ( dbcl, arr )
{
   for( var i = 0; i < arr.length; i++ )
   {
      try
      {
         dbcl.insert( arr[i] );
         throw "NEED_INSERT_ERR";
      } catch( e )
      {
         if( e !== -6 )
         {
            println( "err occor the " + i + "th record, record is :" + JSON.stringify( arr[i] ) );
            throw new Error( e );
         }
      }
   }
}

/*******************************************************************************
@Description : 指定options参数为不支持类型创建自增字段
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function create ( dbcl, options, isLegal )
{
   try
   {
      dbcl.createAutoIncrement( options );
      if( !isLegal )
      {
         throw "NEED_ERROR";
      }
   }
   catch( e )
   {
      if( !isLegal )
      {
         if( e !== -6 )
         {
            throw new Error( e );
         }
      }
      else
      {
         throw new Error( e );
      }
   }
}

/*******************************************************************************
@Description : 插入数据并返回预期结果
@return: 
@Modify list : 2018-10-17 zhaoyu init
*******************************************************************************/
function insertAndGetExpList ( cl, increment_1, increment_2, currentValue_1, currentValue_2, expList, insertCount )
{
   for( var i = 0; i < 3; i++ )
   {
      cl.insert( { a: i } );
      currentValue_1 = currentValue_1 + increment_1;
      currentValue_2 = currentValue_2 + increment_2;
      expList.push( { a: i, id1: currentValue_1, id2: currentValue_2 } );
      insertCount.count++;
   }
   expList.sort( compare( "id1" ) );
   return expList;
}

