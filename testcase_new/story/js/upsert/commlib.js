/*******************************************************************************
*@Description: JavaScript common function library
*@Modify list:
*   2014-4-21 wenjing wang Init
*******************************************************************************/

//构造JSON字符串
function BuildObjStr( obj )
{
   var objstr; 
   if( '[object Array]' === toString.apply( obj ) )
   {
      objstr = "["; 
      var first = true; 
      for( item in obj )
      {
         if( !first )
         {
            objstr += ", "; 
         }
         else
         {
            first = false; 
         }
         
         if( "object" == typeof( obj[item] ) )
         {
            objstr += BuildObjStr( obj[item] ); 
         }
         else if( "string" == typeof( obj[item] ) )
         {
            objstr +=( "'" + obj[item] + "'" ); 
         }
         else
         {
            objstr += obj[item]; 
         }
      }
      objstr += "]"; 
   }
   else
   {
      objstr = "{"; 
      var first = true; 
      for( item in obj )
      {
         if( !first )
         {
            objstr += ", "; 
         }
         else
         {
            first = false; 
         }
         if( "object" == typeof( obj[item] ) )
         {
            objstr = objstr + item + ":"; 
            objstr += BuildObjStr( obj[item] ); 
         }
         else if( "string" == typeof( obj[item] ) )
         {
            objstr +=( item + ":'" + obj[item] + "'" ); 
         }
         else
         {
            objstr +=( item + ":" + obj[item] ); 
         }
      }
      objstr += "}"; 
   }
   return objstr; 
}

function compareObj( lobj, robj, needObjectID )
{
   if( typeof( lobj )!== "object" ||
   typeof( robj )!== "object" )
   {
      return lobj === robj; 
   }
   
   if( lobj === null || robj === null )
   {
      return lobj === robj; 
   }
   
   if( undefined === needObjectID )
   {
      var needObjectID = false; 
   }
   
   var lkeys = Object.getOwnPropertyNames( lobj ); 
   var rkeys = Object.getOwnPropertyNames( robj ); 
   if( ( needObjectID && lkeys.length !== rkeys.length )||
( !needObjectID && Math.abs( lkeys.length - rkeys.length )!== 1 ) )
   {
      return false; 
   }
   
   for( k in lobj )
   {
      if( needObjectID === false && k === "_id" )
      {
         continue; 
      }
      
      if( robj[k] === undefined )
      {
         println( k + " not exist " ); 
         return false; 
      }
      
      if( typeof( lobj[k] )=== "object" &&
      typeof( robj[k] )=== "object" )
      {
         if( !compareObj( lobj[k], robj[k], true ) )
         {
            println( JSON.stringify( lobj[k] )+ " not equal " + JSON.stringify( robj[k] ) ); 
            return false; 
         }
      }
      else if( typeof( lobj[k] )=== "object" ||
      typeof( robj[k] )=== "object" )
      {
         println( typeof( lobj[k] )+ "not equal" + typeof( robj[k] ) ); 
         return false; 
      }
      else if( lobj[k] !== robj[k] )
      {
         println( lobj[k] + " not equal " + robj[k] ); 
         return false; 
      }
   }
   return true; 
}

function checkResult( cl, cond, resultSet )
{
   if( "undefined" === typeof( cl ) )
   {
      return true; 
   }
   if( "undefined" === typeof( cond ) )
   {
      var cond = {}; 
   }
   
   if( "undefined" === typeof( resultSet ) )
   {
      var resultSet = []; 
   }
   
   var tmp = resultSet.concat(); 
   var realRes = []; 
   var docNum = 0; 
   var check = true; 
   var cursor = cl.find( cond ); 
   while( cursor.next() )
   {
      var doc = cursor.current().toObj(); 
      var i = 0; 
      var find = false; 
      if( check )
      {
         for(; i < tmp.length; ++i )
         {
            if( compareObj( doc, tmp[i], false ) )
            {
               tmp.splice( i, 1 ); 
               find = true; 
               break; 
            }
         }
      }
      
      if( !find )
      {
         check = false; 
      }
      realRes.push( doc ); 
   }
   
   if( !check || realRes.length != resultSet.length )
   {
      throw "expect:" + JSON.stringify( resultSet )+ "real:" + JSON.stringify( realRes ); 
   }
   
}
