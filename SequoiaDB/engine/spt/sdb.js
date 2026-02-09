/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/
// Global Constants
const SDB_PAGESIZE_4K              = 4096 ;
const SDB_PAGESIZE_8K              = 8192 ;
const SDB_PAGESIZE_16K             = 16384 ;
const SDB_PAGESIZE_32K             = 32768 ;
const SDB_PAGESIZE_64K             = 65536 ;
const SDB_PAGESIZE_DEFAULT         = SDB_PAGESIZE_64K ;

const SDB_SNAP_CONTEXTS            = 0 ;
const SDB_SNAP_CONTEXTS_CURRENT    = 1 ;
const SDB_SNAP_SESSIONS            = 2 ;
const SDB_SNAP_SESSIONS_CURRENT    = 3 ;
const SDB_SNAP_COLLECTIONS         = 4 ;
const SDB_SNAP_COLLECTIONSPACES    = 5 ;
const SDB_SNAP_DATABASE            = 6 ;
const SDB_SNAP_SYSTEM              = 7 ;
const SDB_SNAP_CATALOG             = 8 ;
const SDB_SNAP_TRANSACTIONS        = 9 ;
const SDB_SNAP_TRANSACTIONS_CURRENT= 10 ;
const SDB_SNAP_ACCESSPLANS         = 11 ;
const SDB_SNAP_HEALTH              = 12 ;
const SDB_SNAP_CONFIGS             = 13 ;
const SDB_SNAP_SVCTASKS            = 14 ;
const SDB_SNAP_SEQUENCES           = 15 ;
const SDB_SNAP_QUERIES             = 18 ;
const SDB_SNAP_LATCHWAITS          = 19 ;
const SDB_SNAP_LOCKWAITS           = 20 ;
const SDB_SNAP_INDEXSTATS          = 21 ;
const SDB_SNAP_TASKS               = 23 ;
// const SDB_SNAP_INDEXES = 24, for internal use only
const SDB_SNAP_TRANSWAITS          = 25 ;
const SDB_SNAP_TRANSDEADLOCK       = 26 ;
const SDB_SNAP_RECYCLEBIN          = 27 ;

const SDB_LIST_CONTEXTS            = 0 ;
const SDB_LIST_CONTEXTS_CURRENT    = 1 ;
const SDB_LIST_SESSIONS            = 2 ;
const SDB_LIST_SESSIONS_CURRENT    = 3 ;
const SDB_LIST_COLLECTIONS         = 4 ;
const SDB_LIST_COLLECTIONSPACES    = 5 ;
const SDB_LIST_STORAGEUNITS        = 6 ;
const SDB_LIST_GROUPS              = 7 ;
const SDB_LIST_STOREPROCEDURES     = 8 ;
const SDB_LIST_DOMAINS             = 9 ;
const SDB_LIST_TASKS               = 10 ;
const SDB_LIST_TRANSACTIONS        = 11 ;
const SDB_LIST_TRANSACTIONS_CURRENT = 12 ;
const SDB_LIST_SVCTASKS            = 14 ;
const SDB_LIST_SEQUENCES           = 15 ;
const SDB_LIST_USERS               = 16 ;
const SDB_LIST_BACKUPS             = 17 ;
const SDB_LIST_DATASOURCES         = 22 ;
// const SDB_LIST_INDEXES = 24, for internal use only
const SDB_LIST_RECYCLEBIN          = 27 ;
const SDB_LIST_GROUPMODES          = 28 ;

const SDB_INSERT_CONTONDUP         = 1 ;
const SDB_INSERT_RETURN_ID         = 0x10000000 ;
const SDB_INSERT_REPLACEONDUP      = 4 ;
const SDB_INSERT_UPDATEONDUP       = 0x00000008 ;
// const SDB_INSERT_HAS_ID_FIELD = 0x00000010, for internal use only
const SDB_INSERT_CONTONDUP_ID      = 0x00000020 ;
const SDB_INSERT_REPLACEONDUP_ID   = 0x00000040 ;

const SDB_TRACE_FLW                = 0 ;
const SDB_TRACE_FMT                = 1 ;

const SDB_COORD_GROUP_NAME         = "SYSCoord" ;
const SDB_CATALOG_GROUP_NAME       = "SYSCatalogGroup" ;
const SDB_SPARE_GROUP_NAME         = "SYSSpare" ;

const SDB_JSON_PARSE               = JSON.parse ;

const CM_PORT        = "CM_PORT" ;
const TMP_PATH       = "TMP_PATH" ;
const TRACE_HOSTNAME = "TRACE_HOSTNAME"  ;

// SdbQuery flags
const SDB_FLG_QUERY_FORCE_HINT        = 0x00000080 ;
const SDB_FLG_QUERY_PARALLED          = 0x00000100 ;
const SDB_FLG_QUERY_WITH_RETURNDATA   = 0x00000200 ;
const SDB_FLG_QUERY_PREPARE_MORE      = 0x00004000 ;
const SDB_FLG_QUERY_FOR_UPDATE        = 0x00010000 ;
const SDB_FLG_QUERY_FOR_SHARE         = 0x00040000 ;

// end Global Constants

// Global functions

// register json function
//JSON.parse JSON.stringify
function SDB_INIT(){

   function isArray( object ){
      return ( object &&
               typeof( object ) === 'object' &&
               typeof( object.length ) === 'number' &&
               typeof( object.splice ) === 'function' &&
               !( object.propertyIsEnumerable( 'length' ) ) ) ;
   }

   function filterInviChart(str) {
      var i = 0, len = str.length;
      var newStr = '';
      var chars, code;
      while (i < len) {
         chars = str.charAt(i);
         code = chars.charCodeAt();
         if (code < 0x20 || code == 0x7F) {
            chars = '?';
         }
         newStr += chars;
         ++i;
      }
      return newStr;
   }

   JSON.parse = function(str, func) {
      var json;
      try {
         json = SDB_JSON_PARSE(str, func);
      } catch (e) {
         try {
            var newStr = filterInviChart(str);
            json = SDB_JSON_PARSE(newStr, func);
         } catch (e) {
            json = eval('(' + str + ')');
         }
      }
      return json;
   } ;

   //var rx_escapable = /[\\\"\u0000-\u001f\u007f-\u009f\u00ad\u0600-\u0604\u070f\u17b4\u17b5\u200c-\u200f\u2028-\u202f\u2060-\u206f\ufeff\ufff0-\uffff]/g;

   function f(n) {
      return n < 10
          ? "0" + n
          : n;
   }

   function this_value() {
      return this.valueOf();
   }

   if (typeof Date.prototype.toJSON !== "function") {

      Date.prototype.toJSON = function () {

         return isFinite(this.valueOf())
             ? this.getUTCFullYear() + "-" +
                     f(this.getUTCMonth() + 1) + "-" +
                     f(this.getUTCDate()) + "T" +
                     f(this.getUTCHours()) + ":" +
                     f(this.getUTCMinutes()) + ":" +
                     f(this.getUTCSeconds()) + "Z"
             : null;
      };

      Boolean.prototype.toJSON = this_value;
      Number.prototype.toJSON = this_value;
      String.prototype.toJSON = this_value;
   }

   var gap;
   var indent;
   var meta;
   var rep;


   function quote(str) {
      var newString = "" ;
      var length = str.length ;
      for( var i = 0; i < length; ++i )
      {
         switch( str.charAt( i ) )
         {
         case "\"":
            newString += "\\\"" ;
            break ;
         case "\\":
            newString += "\\\\" ;
            break ;
         case "\b":
            newString += "\\b" ;
            break ;
         case "\f":
            newString += "\\f" ;
            break ;
         case "\n":
            newString += "\\n" ;
            break ;
         case "\r":
            newString += "\\r" ;
            break ;
         case "\t":
            newString += "\\t" ;
            break ;
         default:
            newString += str.charAt( i ) ;
         }
      }
      return "\"" + newString + "\"" ;
   }


   function str(key, holder) {

      var i;
      var k;
      var v;
      var length;
      var mind = gap;
      var partial;
      var value = holder[key];

      if (value && typeof value === "object" &&
              typeof value.toJSON === "function") {
         value = value.toJSON(key);
      }

      if (typeof rep === "function") {
         value = rep.call(holder, key, value);
      }

      switch (typeof value) {
         case "string":
            return quote(value);

         case "number":
            if (value === Number.POSITIVE_INFINITY) {
               return 'Infinity';
            }
            else if (value === Number.NEGATIVE_INFINITY) {
               return '-Infinity';
            }
            else if (value === Number.NaN) {
               return '0';
            }
            else {
               return String(value);
            }

         case "boolean":
         case "null":
            return String(value);

         case "object":
            if (!value) {
               return "null";
            }
            gap += indent;
            partial = [];

            if( isArray( value ) ) {
               length = value.length;
               for (i = 0; i < length; i += 1) {
                  partial[i] = str(i, value) || "null";
               }

               v = partial.length === 0
                   ? "[]"
                   : gap
                       ? "[\n" + gap + partial.join(",\n" + gap) + "\n" + mind + "]"
                       : "[" + partial.join(",") + "]";
               gap = mind;
               return v;
            }

            if (rep && typeof rep === "object") {
               length = rep.length;
               for (i = 0; i < length; i += 1) {
                  if (typeof rep[i] === "string") {
                     k = rep[i];
                     v = str(k, value);
                     if (v) {
                        partial.push(quote(k) + (
                            gap
                                ? ": "
                                : ":"
                        ) + v);
                     }
                  }
               }
            } else {

               for (k in value) {
                  //if (Object.prototype.hasOwnProperty.call(value, k)) {
                     v = str(k, value);
                     if (v) {
                        partial.push(quote(k) + (
                            gap
                                ? ": "
                                : ":"
                        ) + v);
                     }
                  //}
               }
            }

            v = partial.length === 0
                ? "{}"
                : gap
                    ? "{\n" + gap + partial.join(",\n" + gap) + "\n" + mind + "}"
                    : "{" + partial.join(",") + "}";
            gap = mind;
            return v;
      }
   }

   meta = {
      "\b": "\\b",
      "\t": "\\t",
      "\n": "\\n",
      "\f": "\\f",
      "\r": "\\r",
      "\"": "\\\"",
      "\\": "\\\\"
   };

   JSON.stringify = function (value, replacer, space) {

      var i;
      gap = "";
      indent = "";

      if (typeof space === "number") {
         for (i = 0; i < space; i += 1) {
            indent += " ";
         }

      } else if (typeof space === "string") {
         indent = space;
      }

      rep = replacer;
      if (replacer && typeof replacer !== "function" &&
              (typeof replacer !== "object" ||
              typeof replacer.length !== "number")) {
         throw new Error("JSON.stringify");
      }

      return str("", { "": value });
   };

}
SDB_INIT() ;

function println ( val ) {
   if ( arguments.length > 0 )
      print ( val ) ;
   print ( '\n' ) ;
}
// return a double number between 0 and 1
function rand () {
   return Math.random() ;
}

// return merged json object
function mergeJsonObject(obj1, obj2) {
   var result = {};
   for (var attr in obj1) {
      result[attr] = obj1[attr];
   }
   for (var attr in obj2) {
      result[attr] = obj2[attr];
   }

   return result;
}

function isEmptyObject(obj) {
   for (var name in obj) {
      return false;
   }

   return true;
}

// end Global functions

Object.defineProperty(Object.prototype,'_rawValueOf',{
   enumerable:false,
   value: Object.prototype.valueOf
});

Object.defineProperty(Object.prototype,'_rawToString',{
   enumerable:false,
   value: Object.prototype.toString
});

Object.defineProperty(Object.prototype,'_equality',{
   enumerable:false,
   value: function(rval) {
      if ( this.$numberLong != undefined ) {
         if ( rval.$numberLong != undefined ) {
            return this.valueOf() == rval.valueOf() ;
         }
         return rval == this.valueOf() ;
      }
      if ( rval.$numberLong != undefined ) {
         return this == rval.valueOf() ;
      }

      throw "condition not suitable for the function" ;
   }
});

Object.prototype.valueOf = function() {
   if (this.$numberLong != undefined) {
      if ( typeof(this.$numberLong ) == "string" )
      {
         return parseInt(this.$numberLong) ;
      }
      else if ( typeof(this.$numberLong ) == "number" )
      {
         return this.$numberLong ;
      }
      else
      {
         throw "invalid $numberLong" ;
      }
   }

   return this._rawValueOf() ;
}

Object.prototype.toString = function() {
   if (this.$numberLong != undefined) {
      try
      {
         return JSON.stringify ( this, undefined, 2 ) ;
      }
      catch ( e )
      {
      }
   }
   return this._rawToString() ;
}

// SdbCursor
SdbCursor.prototype.toArray = function() {
   if ( this._arr )
      return this._arr;

   var a = [];
   while ( true ) {
      var bs = this.next();
      if ( ! bs ) break ;
      var json = bs.toJson () ;
      try
      {
         var stf = JSON.stringify(JSON.parse(json), undefined, 2) ;
         a.push ( stf ) ;
      }
      catch ( e )
      {
         a.push ( json ) ;
      }
   }
   this._arr = a ;
   return this._arr ;
}

SdbCursor.prototype.arrayAccess = function( idx ) {
   return this.toArray()[idx] ;
}

SdbCursor.prototype.size = function() {
   //return this.toArray().length ;
   var cursor = this ;
   var size = 0 ;
   var record = undefined ;
   while( ( record = cursor.next() ) != undefined )
   {
      size++ ;
   }
   return size ;
}

SdbCursor.prototype.toString = function() {
   //return this.toArray().join('\n') ;
   var csr = this ;
   var record = undefined ;
   var returnRecordNum = 0 ;
   while ( ( record = csr.next() ) != undefined )
   {
      returnRecordNum++ ;
      try
      {
         println ( record ) ;
      }
      catch ( e )
      {
         var json = record.toJson () ;
         println ( json ) ;
      }
   }
   println("Return "+returnRecordNum+" row(s).") ;
   return "" ;
}
// end SdbCursor


// CLCount
CLCount.prototype.toString = function() {
   this._exec() ;
   return this._count ;
}
CLCount.prototype.valueOf = function() {
   this._exec() ;
   return this._count ;
}
CLCount.prototype.hint = function( hint ) {
   this._hint = hint ;
   return this ;
}
CLCount.prototype._exec = function() {
   if ( 1 != this._type )
   {
      this._count = this._collection._count ( this._condition,
                                              this._hint ) ;
   }
   else
   {
      this._count = this._collection._lobCount ( this._condition,
                                                 this._hint ) ;
   }
}
// end CLCount

// SdbCollection

SdbCollection.prototype.count = function( condition ) {
   var count = new CLCount() ;
   count._condition = {} ;
   count._type = 0 ;
   if( undefined != condition )
   {
      count._condition = condition ;
   }
   count._collection = this ;
   count._hint = {} ;
   return count ;
}

SdbCollection.prototype.lobCount = function( condition ) {
   var count = new CLCount() ;
   count._condition = {} ;
   count._type = 1 ;
   if( undefined != condition )
   {
      count._condition = condition ;
   }
   count._collection = this ;
   count._hint = {} ;
   return count ;
}

SdbCollection.prototype.find = function( query, select ) {

   if ( query instanceof SdbQueryOption )
   {
      return this.rawFind( query );
   }

   var queryObj = new SdbQuery();
   queryObj._query = {};
   queryObj._select = {} ;
   if( undefined != query )
   {
      queryObj._query = query ;
   }
   if( undefined != select )
   {
      queryObj._select = select ;
   }
   queryObj._sort = {} ;
   queryObj._hint = {} ;
   queryObj._options = {} ;
   queryObj._collection = this ;
   return queryObj ;
}

SdbCollection.prototype.findOne = function( query, select ) {
   if ( query instanceof SdbQueryOption )
   {
      return this.rawFind( query.limit(1) );
   }

   var queryObj = new SdbQuery() ;
   queryObj._query = {};
   queryObj._select = {} ;
   if( undefined != query )
   {
      queryObj._query = query ;
   }
   if( undefined != select )
   {
      queryObj._select = select ;
   }
   queryObj._sort = {} ;
   queryObj._hint = {} ;
   queryObj._options = {} ;
   queryObj._collection = this ;
   queryObj.limit( 1 ) ;
   return queryObj ;
}

SdbCollection.prototype.getIndex = function( name ) {
   if ( ! name )
   {
      setLastErrMsg( "SdbCollection.getIndex(): the 1st param should be "
                     + "valid string" ) ;
      throw SDB_INVALIDARG ;
   }

   var obj = this._getIndexes(name).next();
   if (undefined == obj)
   {
      setLastError( SDB_IXM_NOTEXIST ) ;
      setLastErrMsg( getErr( SDB_IXM_NOTEXIST ) ) ;
      throw SDB_IXM_NOTEXIST ;
   }

   return obj ;
}

SdbCollection.prototype.listIndexes = function() {
   return this._getIndexes() ;
}

SdbCollection.prototype.toString = function() {
   return this._cs.toString() + "." + this._name;
}

SdbCollection.prototype.insert = function ( data , arg )
{
   if ( (typeof data) != "object" )
   {
      setLastErrMsg( "SdbCollection.insert(): the 1st param should be "
                     + "obj or array of objs" ) ;
      throw SDB_INVALIDARG ;
   }

   var flag = 0 ;
   if ( arg == undefined )
   {
      flag = 0 ;
   }
   else if ( ( typeof arg ) == "number" ||
             ( ( typeof arg ) == "object" && !( arg instanceof Array ) ) )
   {
      flag = arg ;
   }
   else
   {
      setLastErrMsg( "SdbCollection.insert(): the 2nd param if existed "
                     + "should be a insert flag or insert options" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( data instanceof Array )
   {
      if ( 0 == data.length )
      {
         return ;
      }

      return this._bulkInsert( data, flag ) ;
   }
   else
   {
      return this._insert( data, flag ) ;
   }
}

/*
SdbCollection.prototype.rename = function ( newName ) {
   this._rename ( newName ) ;
   this._name = newName ;
}
*/
// end SdbCollection

// SdbQuery
SdbQuery.prototype._checkExecuted = function() {
   if ( this._cursor )
      throw "query already executed";
}

SdbQuery.prototype._exec = function() {
   if ( ! this._cursor ) {
      this._cursor = this._collection.rawFind( this._query,
                                               this._select,
                                               this._sort,
                                               this._hint,
                                               this._skip,
                                               this._limit,
                                               this._flags,
                                               this._options );
   }
   return this._cursor;
}

SdbQuery.prototype.sort = function( sort ) {
   this._checkExecuted();
   this._sort = sort;
   return this;
}

SdbQuery.prototype.hint = function( hint ) {
   this._checkExecuted();
   if (undefined == this._hint) {
      this._hint = hint;
   } else {
      this._hint = mergeJsonObject(hint, this._hint);
   }
   return this;
}

SdbQuery.prototype.skip = function( skip ) {
   this._checkExecuted();
   this._skip = skip;
   return this;
}

SdbQuery.prototype.limit = function( limit ) {
   this._checkExecuted();
   this._limit = limit;
   return this;
}

SdbQuery.prototype.flags = function( flags ) {
   this._checkExecuted();
   this._flags = flags;
   return this;
}

SdbQuery.prototype.next = function() {
   this._exec();
   return this._cursor.next();
}

SdbQuery.prototype.current = function() {
   this._exec();
   return this._cursor.current();
}

SdbQuery.prototype.close = function() {
   this._exec();
   return this._cursor.close();
}

SdbQuery.prototype.update = function( rule, returnNew, options ) {
   if ((typeof rule) != "object" || isEmptyObject(rule)) {
      setLastErrMsg( "SdbQuery.update(): the 1st param should be "
                     + "non-empty object" ) ;
      throw SDB_INVALIDARG ;
   }
   if (undefined != returnNew && (typeof returnNew) != "boolean") {
      setLastErrMsg( "SdbQuery.update(): the 2nd param should be boolean" ) ;
      throw SDB_INVALIDARG ;
   }
   if (undefined != options && (typeof options) != "object") {
      setLastErrMsg( "SdbQuery.update(): the 3rd param should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   this._checkExecuted();

   if (undefined == this._hint) {
      this._hint = {};
   } else if (undefined != this._hint.$Modify) {
      setLastErrMsg( "SdbQuery.update(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "update";
   modify.Update = rule;
   modify.ReturnNew = (returnNew != undefined) ? returnNew : false;
   this._hint.$Modify = modify;

   if (undefined != options) {
      this._options = options;
   }

   return this;
}

SdbQuery.prototype.remove = function() {
   this._checkExecuted();
   if (undefined == this._hint) {
      this._hint = {};
   } else if (undefined != this._hint.$Modify) {
      setLastErrMsg( "SdbQuery.remove(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "remove";
   modify.Remove = true;
   this._hint.$Modify = modify;

   return this;
}

/*
SdbQuery.prototype.updateCurrent = function ( rule ) {
   this._exec();
   return this._cursor.updateCurrent( rule ) ;
}

SdbQuery.prototype.deleteCurrent = function () {
   this._exec();
   return this._cursor.deleteCurrent();
}
*/
SdbQuery.prototype.toArray = function() {
   this._exec();
   return this._cursor.toArray();
}

SdbQuery.prototype.arrayAccess = function( idx ) {
   return this.toArray()[idx];
}

SdbQuery.prototype.count = function() {
   if (undefined != this._hint && undefined != this._hint.$Modify) {
      setLastErrMsg( "count() cannot be executed with update() or remove()" ) ;
      throw SDB_INVALIDARG ;
   }
   var countObj = this._collection.count( this._query ) ;
   if ( undefined != this._hint ) {
      countObj.hint( this._hint ) ;
   }
   return countObj ;
}

SdbQuery.prototype.explain = function( options ) {
   if( undefined == options )
   {
      options = {} ;
   }
   return this._collection.explain( this._query,
                                    this._select,
                                    this._sort,
                                    this._hint,
                                    this._skip,
                                    this._limit,
                                    this._flags,
                                    options ) ;
}

SdbQuery.prototype.getQueryMeta = function() {
   return this._collection.getQueryMeta( this._query,
                                         this._sort,
                                         this._hint,
                                         this._skip,
                                         this._limit ) ;
}

SdbQuery.prototype.size = function() {
//   return this.toArray().length;
   this._exec();
   return this._cursor.size() ;
}

SdbQuery.prototype.toString = function() {
   this._exec();
   var csr = this._cursor ;
   var record = undefined ;
   var returnRecordNum = 0 ;
   while ( ( record = csr.next() ) != undefined )
   {
      returnRecordNum++ ;
      try
      {
         println ( record ) ;
      }
      catch ( e )
      {
         var json = record.toJson () ;
         println ( json ) ;
      }
   }
   println("Return "+returnRecordNum+" row(s).") ;
   return "" ;
   //return this._cursor.toString();
}
// end SdbQuery

// SdbNode
SdbNode.prototype.toString = function() {
   return this._hostname + ":" +
          this._servicename ;
}

SdbNode.prototype.getHostName = function() {
   return this._hostname ;
}

SdbNode.prototype.getServiceName = function() {
   return this._servicename ;
}

SdbNode.prototype.getNodeDetail = function() {
   return this._nodeid + ":" + this._hostname + ":" +
          this._servicename + "(" +
          this._rg.toString() + ")" ;
}

SdbNode.prototype.getDetailObj = function() {
    var rgInfo = this._rg.getDetailObj().toObj();
    var groupInfo = rgInfo.Group;
    for (var i in groupInfo) {
        if (groupInfo[i].NodeID == this._nodeid) {
            var node = groupInfo[i];
            node.GroupName = rgInfo.GroupName;
            node.GroupID = rgInfo.GroupID;
            return new BSONObj(node);
        }
    }
    setLastErrMsg(getErr(SDB_CLS_NODE_NOT_EXIST));
    throw SDB_CLS_NODE_NOT_EXIST;
}
// end SdbNode

// SdbReplicaGroup
SdbReplicaGroup.prototype.toString = function() {
   return this._name;
}

// getDetail will be remove, suggest using getDetailObj
SdbReplicaGroup.prototype.getDetail = function() {
   return this._conn.list( SDB_LIST_GROUPS,
                           {GroupName: this._name } ) ;
}

SdbReplicaGroup.prototype.getDetailObj = function() {
   var cursor = this._conn.list( SDB_LIST_GROUPS,
                              {GroupName: this._name } ) ;
   if ( undefined == cursor )
   {
      setLastErrMsg( getErr( SDB_CLS_GRP_NOT_EXIST ) ) ;
      throw SDB_CLS_GRP_NOT_EXIST ;
   }
   var obj = cursor.next() ;
   cursor.close() ;
   return obj ;
}
// end SdbReplicaGroup

// SdbCS
SdbCS.prototype.toString = function() {
   return this._conn.toString() + "." + this._name;
}

SdbCS.prototype._resolveCL = function(clName) {
   if( !this.hasOwnProperty( clName) )
   {
      this.getCL( clName ) ;
   }
}

// end SdbCS


// SdbDomain
SdbDomain.prototype.toString = function() {
   return this._domainname ;
}
// end SdbDomain

// SdbDc
SdbDC.prototype.toString = function() {
   return this._name ;
}

SdbDC.prototype.reelect = function( option ) {
   if ( undefined == option || ( typeof option ) != "object" )
   {
      setLastErrMsg( "SdbDC.reelect(): param should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   var hasHostName = undefined != option.HostName ;
   var hasLocation = undefined != option.Location ;

   if ( !hasHostName && !hasLocation )
   {
      setLastErrMsg( "SdbDC.reelect(): must specify HostName or Location" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( hasHostName && hasLocation )
   {
      setLastErrMsg( "SdbDC.reelect(): cannot specify both HostName and Location" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( hasHostName && ( typeof option.HostName ) != "string" )
   {
      setLastErrMsg( "SdbDC.reelect(): HostName should be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( hasLocation && ( typeof option.Location ) != "string" )
   {
      setLastErrMsg( "SdbDC.reelect(): Location should be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != option.Domain )
   {
      var domainType = typeof option.Domain ;
      if ( domainType != "string" && !( option.Domain instanceof Array ) )
      {
         setLastErrMsg( "SdbDC.reelect(): Domain should be string or array of strings" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   var domainGroupNames = undefined ;
   if ( undefined != option.Domain )
   {
      var domains = option.Domain ;
      if ( !( domains instanceof Array ) )
      {
         domains = [ domains ] ;
      }
      
      domainGroupNames = [] ;
      var domainCursor = this._conn.list( SDB_LIST_DOMAINS, { Name: { $in: domains } } ) ;
      if ( undefined != domainCursor )
      {
         var domainRecord = undefined ;
         while ( ( domainRecord = domainCursor.next() ) != undefined )
         {
            var domainObj = domainRecord.toObj() ;
            if ( undefined != domainObj.Groups )
            {
               for ( var i = 0; i < domainObj.Groups.length; i++ )
               {
                  domainGroupNames.push( domainObj.Groups[i].GroupName ) ;
               }
            }
         }
         domainCursor.close() ;
      }
      
      if ( domainGroupNames.length == 0 )
      {
         setLastErrMsg( "SdbDC.reelect(): specified Domain does not exist or has no groups" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   var filter = {} ;
   if ( hasHostName )
   {
      filter["Group.HostName"] = option.HostName ;
   }
   
   if ( hasLocation )
   {
      filter["Group.Location"] = option.Location ;
   }

   var cursor = this._conn.list( SDB_LIST_GROUPS, filter ) ;
   if ( undefined == cursor )
   {
      return ;
   }

   var matchedGroup = false ;
   var groups = [] ;
   var record = undefined ;
   while ( ( record = cursor.next() ) != undefined )
   {
      matchedGroup = true ;
      var groupObj = record.toObj() ;
      var groupName = groupObj.GroupName ;
      
      if ( groupName == SDB_COORD_GROUP_NAME || groupName == SDB_SPARE_GROUP_NAME )
      {
         continue ;
      }
      
      if ( undefined != domainGroupNames )
      {
         if ( domainGroupNames.indexOf( groupName ) == -1 )
         {
            continue ;
         }
      }
      
      groups.push( groupName ) ;
   }
   cursor.close() ;

   if ( !matchedGroup )
   {
      setLastErrMsg( "SdbDC.reelect(): HostName or location does not exist" ) ;
      throw SDB_INVALIDARG ;
   }

   // remove unused options
   if ( undefined != option.Domain )
   {
      delete option.Domain ;
   }
   if ( undefined != option.NodeID )
   {
      delete option.NodeID ;
   }
   if ( undefined != option.ServiceName )
   {
      delete option.ServiceName ;
   }
   if ( undefined != option.NodeIDs )
   {
      delete option.NodeIDs ;
   }

   var matchedNum = groups.length ;
   var succeedNum = 0 ;
   var ignoredNum = 0 ;
   var failedNum = 0 ;
   var failedGroups = [] ;

   for ( var i = 0; i < groups.length; i++ )
   {
      try
      {
         var rg = this._conn.getRG( groups[i] ) ;
         var result = rg.reelect( option ) ;
         
         if ( undefined != result )
         {
            var resultObj = result.toObj() ;
            if ( undefined != resultObj.Changed && resultObj.Changed == false )
            {
               ignoredNum++ ;
            }
            else
            {
               succeedNum++ ;
            }
         }
         else
         {
            succeedNum++ ;
         }
      }
      catch ( e )
      {
         failedNum++ ;
         failedGroups.push( groups[i] ) ;
      }
   }

   var summary = {
      MatchedNum: matchedNum,
      SucceedNum: succeedNum,
      IgnoredNum: ignoredNum,
      FailedNum: failedNum
   } ;

   if ( failedNum > 0 )
   {
      summary.FailedGroups = failedGroups ;
   }

   return new BSONObj( summary ) ;
}

SdbDC.prototype.primarySave = function( option, filename ) {
   if ( null == option )
   {
      option = undefined ;
   }

   if ( undefined != option && ( typeof option ) != "object" )
   {
      setLastErrMsg( "SdbDC.primarySave(): option should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != filename && ( typeof filename ) != "string" )
   {
      setLastErrMsg( "SdbDC.primarySave(): filename should be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != option )
   {
      if ( undefined != option.HostName && ( typeof option.HostName ) != "string" )
      {
         setLastErrMsg( "SdbDC.primarySave(): HostName should be string" ) ;
         throw SDB_INVALIDARG ;
      }

      if ( undefined != option.Domain )
      {
         var domainType = typeof option.Domain ;
         if ( domainType != "string" && !( option.Domain instanceof Array ) )
         {
            setLastErrMsg( "SdbDC.primarySave(): Domain should be string or array of strings" ) ;
            throw SDB_INVALIDARG ;
         }
      }
   }

   var domainGroupNames = undefined ;
   if ( undefined != option && undefined != option.Domain )
   {
      var domains = option.Domain ;
      if ( !( domains instanceof Array ) )
      {
         domains = [ domains ] ;
      }

      domainGroupNames = [] ;
      var domainCursor = this._conn.list( SDB_LIST_DOMAINS, { Name: { $in: domains } } ) ;
      if ( undefined != domainCursor )
      {
         var domainRecord = undefined ;
         while ( ( domainRecord = domainCursor.next() ) != undefined )
         {
            var domainObj = domainRecord.toObj() ;
            if ( undefined != domainObj.Groups )
            {
               for ( var i = 0; i < domainObj.Groups.length; i++ )
               {
                  domainGroupNames.push( domainObj.Groups[i].GroupName ) ;
               }
            }
         }
         domainCursor.close() ;
      }

      if ( domainGroupNames.length == 0 )
      {
         setLastErrMsg( "SdbDC.primarySave(): specified Domain does not exist or has no groups" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   var filter = {} ;
   if ( undefined != option && undefined != option.HostName )
   {
      filter["Group.HostName"] = option.HostName ;
   }

   var cursor = this._conn.list( SDB_LIST_GROUPS, filter ) ;
   if ( undefined == cursor )
   {
      return ;
   }

   var matchedGroup = false ;
   var matchedNum = 0 ;
   var succeedNum = 0 ;
   var ignoredNum = 0 ;
   var failedNum = 0 ;
   var failedGroups = [] ;
   var detail = [] ;

   var record = undefined ;
   while ( ( record = cursor.next() ) != undefined )
   {
      matchedGroup = true ;
      var groupObj = record.toObj() ;
      var groupName = groupObj.GroupName ;

      if ( groupName == SDB_COORD_GROUP_NAME || groupName == SDB_SPARE_GROUP_NAME )
      {
         continue ;
      }

      if ( undefined != domainGroupNames )
      {
         if ( domainGroupNames.indexOf( groupName ) == -1 )
         {
            continue ;
         }
      }

      matchedNum++ ;

      try
      {
         var snapCursor = this._conn.exec( 'select NodeName from $SNAPSHOT_DB where GroupName = "' +
                                          groupName + '" and IsPrimary = true /*+use_option(ShowError,ignore) */' ) ;
         var primaryRecord = snapCursor.next() ;
         snapCursor.close() ;

         if ( undefined == primaryRecord )
         {
            failedNum++ ;
            failedGroups.push( groupName ) ;
            continue ;
         }

         var primaryObj = primaryRecord.toObj() ;
         if ( undefined == primaryObj.NodeName )
         {
            failedNum++ ;
            failedGroups.push( groupName ) ;
            continue ;
         }

         detail.push( {
            GroupName: groupName,
            PrimaryNode: primaryObj.NodeName
         } ) ;
         succeedNum++ ;
      }
      catch ( e )
      {
         failedNum++ ;
         failedGroups.push( groupName ) ;
      }
   }
   cursor.close() ;

   if ( !matchedGroup )
   {
      setLastErrMsg( "SdbDC.primarySave(): specified HostName does not exist" ) ;
      throw SDB_INVALIDARG ;
   }

   var summary = {
      MatchedNum: matchedNum,
      SucceedNum: succeedNum,
      IgnoredNum: ignoredNum,
      FailedNum: failedNum
   } ;

   if ( failedNum > 0 )
   {
      summary.FailedGroups = failedGroups ;
   }

   summary.Detail = detail ;

   if ( undefined != filename )
   {
      var file = new File( filename ) ;
      file.truncate() ;
      file.write( JSON.stringify( summary, null, 2 ) ) ;
      file.close() ;
   }

   return new BSONObj( summary ) ;
}

SdbDC.prototype.primaryRestore = function( planobjOrFile, option ) {
   if ( undefined == planobjOrFile )
   {
      setLastErrMsg( "SdbDC.primaryRestore(): planobjOrFile is required" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( null == option )
   {
      option = undefined ;
   }

   if ( undefined != option && ( typeof option ) != "object" )
   {
      setLastErrMsg( "SdbDC.primaryRestore(): option should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != option )
   {
      if ( undefined != option.HostName && ( typeof option.HostName ) != "string" )
      {
         setLastErrMsg( "SdbDC.primaryRestore(): HostName should be string" ) ;
         throw SDB_INVALIDARG ;
      }

      if ( undefined != option.Domain )
      {
         var domainType = typeof option.Domain ;
         if ( domainType != "string" && !( option.Domain instanceof Array ) )
         {
            setLastErrMsg( "SdbDC.primaryRestore(): Domain should be string or array of strings" ) ;
            throw SDB_INVALIDARG ;
         }
      }
   }

   var saveObj = undefined ;

   if ( ( typeof planobjOrFile ) == "string" )
   {
      var file = new File( planobjOrFile ) ;
      var content = file.read() ;
      file.close() ;
      saveObj = JSON.parse( content ) ;
   }
   else if ( ( typeof planobjOrFile ) == "object" )
   {
      if ( typeof planobjOrFile.toObj == "function" )
      {
         saveObj = planobjOrFile.toObj() ;
      }
      else
      {
         saveObj = planobjOrFile ;
      }
   }
   else
   {
      setLastErrMsg( "SdbDC.primaryRestore(): planobjOrFile should be object, BSONObj or filename" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined == saveObj.Detail || !( saveObj.Detail instanceof Array ) )
   {
      setLastErrMsg( "SdbDC.primaryRestore(): param does not contain valid Detail" ) ;
      throw SDB_INVALIDARG ;
   }

   var domainGroupNames = undefined ;
   if ( undefined != option && undefined != option.Domain )
   {
      var domains = option.Domain ;
      if ( !( domains instanceof Array ) )
      {
         domains = [ domains ] ;
      }

      domainGroupNames = [] ;
      var domainCursor = this._conn.list( SDB_LIST_DOMAINS, { Name: { $in: domains } } ) ;
      if ( undefined != domainCursor )
      {
         var domainRecord = undefined ;
         while ( ( domainRecord = domainCursor.next() ) != undefined )
         {
            var domainObj = domainRecord.toObj() ;
            if ( undefined != domainObj.Groups )
            {
               for ( var j = 0; j < domainObj.Groups.length; j++ )
               {
                  domainGroupNames.push( domainObj.Groups[j].GroupName ) ;
               }
            }
         }
         domainCursor.close() ;
      }

      if ( domainGroupNames.length == 0 )
      {
         setLastErrMsg( "SdbDC.primaryRestore(): specified Domain does not exist or has no groups" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   var hostGroupNames = undefined ;
   if ( undefined != option && undefined != option.HostName )
   {
      var filter = { "Group.HostName": option.HostName } ;
      var hostCursor = this._conn.list( SDB_LIST_GROUPS, filter ) ;
      var matchedGroup = false ;
      hostGroupNames = [] ;
      if ( undefined != hostCursor )
      {
         var hostRecord = undefined ;
         while ( ( hostRecord = hostCursor.next() ) != undefined )
         {
            matchedGroup = true ;
            var hostObj = hostRecord.toObj() ;
            var gName = hostObj.GroupName ;
            if ( gName != SDB_COORD_GROUP_NAME && gName != SDB_SPARE_GROUP_NAME )
            {
               hostGroupNames.push( gName ) ;
            }
         }
         hostCursor.close() ;
      }

      if ( !matchedGroup )
      {
         setLastErrMsg( "SdbDC.primaryRestore(): specified HostName does not exist" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   var details = saveObj.Detail ;
   var matchedNum = 0 ;
   var succeedNum = 0 ;
   var ignoredNum = 0 ;
   var failedNum = 0 ;
   var failedGroups = [] ;

   for ( var i = 0; i < details.length; i++ )
   {
      var groupName = details[i].GroupName ;
      var primaryNode = details[i].PrimaryNode ;

      if ( undefined == groupName || undefined == primaryNode )
      {
         continue ;
      }

      if ( undefined != domainGroupNames )
      {
         if ( domainGroupNames.indexOf( groupName ) == -1 )
         {
            continue ;
         }
      }

      if ( undefined != hostGroupNames )
      {
         if ( hostGroupNames.indexOf( groupName ) == -1 )
         {
            continue ;
         }
      }

      var parts = primaryNode.split( ":" ) ;
      if ( parts.length != 2 )
      {
         failedNum++ ;
         failedGroups.push( groupName ) ;
         continue ;
      }

      matchedNum++ ;

      var rgOption = {
         HostName: parts[0],
         ServiceName: parts[1]
      } ;

      if ( undefined != option )
      {
         if ( undefined != option.Seconds )
         {
            rgOption.Seconds = option.Seconds ;
         }
         if ( undefined != option.Level )
         {
            rgOption.Level = option.Level ;
         }
      }

      try
      {
         var rg = this._conn.getRG( groupName ) ;
         var result = rg.reelect( rgOption ) ;

         if ( undefined != result )
         {
            var resultObj = result.toObj() ;
            if ( undefined != resultObj.Changed && resultObj.Changed == false )
            {
               ignoredNum++ ;
            }
            else
            {
               succeedNum++ ;
            }
         }
         else
         {
            succeedNum++ ;
         }
      }
      catch ( e )
      {
         failedNum++ ;
         failedGroups.push( groupName ) ;
      }
   }

   var summary = {
      MatchedNum: matchedNum,
      SucceedNum: succeedNum,
      IgnoredNum: ignoredNum,
      FailedNum: failedNum
   } ;

   if ( failedNum > 0 )
   {
      summary.FailedGroups = failedGroups ;
   }

   return new BSONObj( summary ) ;
}

SdbDC.prototype.reelectAnalyse = function( option, run ) {
   if ( null == option )
   {
      option = undefined ;
   }

   if ( undefined != option && ( typeof option ) != "object" )
   {
      setLastErrMsg( "SdbDC.reelectAnalyse(): option should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined == run )
   {
      run = false ;
   }

   if ( ( typeof run ) != "boolean" )
   {
      setLastErrMsg( "SdbDC.reelectAnalyse(): run should be boolean" ) ;
      throw SDB_INVALIDARG ;
   }

   // validate option fields
   if ( undefined != option )
   {
      if ( undefined != option.HostName && ( typeof option.HostName ) != "string" )
      {
         setLastErrMsg( "SdbDC.reelectAnalyse(): HostName should be string" ) ;
         throw SDB_INVALIDARG ;
      }

      if ( undefined != option.Domain )
      {
         var domainType = typeof option.Domain ;
         if ( domainType != "string" && !( option.Domain instanceof Array ) )
         {
            setLastErrMsg( "SdbDC.reelectAnalyse(): Domain should be string or array of strings" ) ;
            throw SDB_INVALIDARG ;
         }
      }

      if ( undefined != option.FilterLevel )
      {
         if ( ( typeof option.FilterLevel ) != "string" )
         {
            setLastErrMsg( "SdbDC.reelectAnalyse(): FilterLevel should be string" ) ;
            throw SDB_INVALIDARG ;
         }
         if ( option.FilterLevel != "GroupMode" &&
              option.FilterLevel != "Location" &&
              option.FilterLevel != "Weight" )
         {
            setLastErrMsg( "SdbDC.reelectAnalyse(): FilterLevel should be one of: GroupMode, Location, Weight" ) ;
            throw SDB_INVALIDARG ;
         }
      }

      if ( undefined != option.Rebalance && ( typeof option.Rebalance ) != "boolean" )
      {
         setLastErrMsg( "SdbDC.reelectAnalyse(): Rebalance should be boolean" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   var filterLevel = "Weight" ;
   var rebalance = true ;
   if ( undefined != option )
   {
      if ( undefined != option.FilterLevel )
      {
         filterLevel = option.FilterLevel ;
      }
      if ( undefined != option.Rebalance )
      {
         rebalance = option.Rebalance ;
      }
   }

   // resolve domain groups
   var domainGroupNames = undefined ;
   if ( undefined != option && undefined != option.Domain )
   {
      var domains = option.Domain ;
      if ( !( domains instanceof Array ) )
      {
         domains = [ domains ] ;
      }

      domainGroupNames = [] ;
      var domainCursor = this._conn.list( SDB_LIST_DOMAINS, { Name: { $in: domains } } ) ;
      if ( undefined != domainCursor )
      {
         var domainRecord = undefined ;
         while ( ( domainRecord = domainCursor.next() ) != undefined )
         {
            var domainObj = domainRecord.toObj() ;
            if ( undefined != domainObj.Groups )
            {
               for ( var i = 0; i < domainObj.Groups.length; i++ )
               {
                  domainGroupNames.push( domainObj.Groups[i].GroupName ) ;
               }
            }
         }
         domainCursor.close() ;
      }

      if ( domainGroupNames.length == 0 )
      {
         setLastErrMsg( "SdbDC.reelectAnalyse(): specified Domain does not exist or has no groups" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   // resolve host groups
   var filter = {} ;
   if ( undefined != option && undefined != option.HostName )
   {
      filter["Group.HostName"] = option.HostName ;
   }

   var cursor = this._conn.list( SDB_LIST_GROUPS, filter ) ;
   if ( undefined == cursor )
   {
      return ;
   }

   var matchedGroup = false ;
   var groups = [] ;
   var record = undefined ;
   while ( ( record = cursor.next() ) != undefined )
   {
      matchedGroup = true ;
      var groupObj = record.toObj() ;
      var groupName = groupObj.GroupName ;

      if ( groupName == SDB_COORD_GROUP_NAME || groupName == SDB_SPARE_GROUP_NAME )
      {
         continue ;
      }

      if ( undefined != domainGroupNames )
      {
         if ( domainGroupNames.indexOf( groupName ) == -1 )
         {
            continue ;
         }
      }

      groups.push( groupName ) ;
   }
   cursor.close() ;

   if ( undefined != option && undefined != option.HostName && !matchedGroup )
   {
      setLastErrMsg( "SdbDC.reelectAnalyse(): specified HostName does not exist" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( groups.length == 0 )
   {
      var summary = {
         MatchedNum: 0,
         SucceedNum: 0,
         IgnoredNum: 0,
         FailedNum: 0,
         Detail: []
      } ;
      return new BSONObj( summary ) ;
   }

   // build FilterLevel allowed desps
   var allowedDesps = [] ;
   if ( filterLevel == "GroupMode" )
   {
      allowedDesps = [ "Critical", "Maintenance" ] ;
   }
   else if ( filterLevel == "Location" )
   {
      allowedDesps = [ "Critical", "Maintenance", "ActiveLocation", "AffinitiveLocation" ] ;
   }
   // Weight: all desps allowed, no filtering

   // query snapshot configs
   // For each group, only keep: bestCandidates (highest weight nodes), primaryNode
   // Results are ordered by GroupName, RunStatusWeight desc, IsPrimary desc, NodeName
   // groupInfo[groupName] = { bestWeight, bestCandidates: [{NodeName, RunStatusWeightDesp}], primaryNode: {NodeName, RunStatusWeightDesp} }
   var groupInfo = {} ;
   var allGroups = ( undefined == option || ( undefined == option.HostName && undefined == option.Domain ) ) ;

   // helper: parse snapshot cursor into groupInfo
   function _parseSnapCursor( snapCursor )
   {
      if ( undefined == snapCursor )
      {
         return ;
      }
      var snapRecord = undefined ;
      while ( ( snapRecord = snapCursor.next() ) != undefined )
      {
         var snapObj = snapRecord.toObj() ;
         var gn = snapObj.GroupName ;
         if ( undefined == gn || gn == "" || gn == null ||
              gn == SDB_COORD_GROUP_NAME || gn == SDB_SPARE_GROUP_NAME )
         {
            continue ;
         }
         var desp = ( undefined != snapObj.RunStatusWeightDesp ) ? snapObj.RunStatusWeightDesp : "" ;
         var node = {
            NodeName: snapObj.NodeName,
            RunStatusWeightDesp: desp
         } ;

         if ( undefined == groupInfo[gn] )
         {
            // first record of this group = highest weight
            groupInfo[gn] = {
               bestWeight: snapObj.RunStatusWeight,
               bestCandidates: [ node ],
               primaryNode: undefined
            } ;
            if ( snapObj.IsPrimary == true )
            {
               groupInfo[gn].primaryNode = node ;
            }
         }
         else
         {
            var info = groupInfo[gn] ;

            if ( snapObj.IsPrimary == true )
            {
               info.primaryNode = node ;
            }

            // keep nodes with same best weight as candidates
            if ( snapObj.RunStatusWeight == info.bestWeight &&
                 snapObj.IsPrimary != true )
            {
               info.bestCandidates.push( node ) ;
            }
         }
      }
      snapCursor.close() ;
   }

   if ( allGroups )
   {
      // no filter, query all groups in one SQL without WHERE clause
      var sql = 'select NodeName, RunStatusWeight, RunStatusWeightDesp, IsPrimary, GroupName from $SNAPSHOT_CONFIGS ' +
                'order by GroupName, RunStatusWeight desc, IsPrimary desc, NodeName ' +
                '/*+use_option(IgnoreDefault,true) use_option(Expand,false) use_option(ShowRunStatus,true) use_option(ShowError,ignore) */' ;

      try
      {
         _parseSnapCursor( this._conn.exec( sql ) ) ;
      }
      catch ( e )
      {
         // ignore errors, groups will be treated as failed
      }
   }
   else
   {
      // query in batches of 50
      var batchSize = 50 ;
      for ( var batchStart = 0; batchStart < groups.length; batchStart += batchSize )
      {
         var batchEnd = batchStart + batchSize ;
         if ( batchEnd > groups.length )
         {
            batchEnd = groups.length ;
         }

         var groupList = "" ;
         for ( var i = batchStart; i < batchEnd; i++ )
         {
            if ( i > batchStart )
            {
               groupList += "," ;
            }
            groupList += '"' + groups[i] + '"' ;
         }

         var sql = 'select NodeName, RunStatusWeight, RunStatusWeightDesp, IsPrimary, GroupName from $SNAPSHOT_CONFIGS where GroupName in (' +
                   groupList + ') order by GroupName, RunStatusWeight desc, IsPrimary desc, NodeName ' +
                   '/*+use_option(IgnoreDefault,true) use_option(Expand,false) use_option(ShowRunStatus,true) use_option(ShowError,ignore) */' ;

         try
         {
            _parseSnapCursor( this._conn.exec( sql ) ) ;
         }
         catch ( e )
         {
            // ignore batch errors, groups will be treated as failed
         }
      }
   }

   // helper: get host from NodeName
   function _getHost( nodeName )
   {
      var parts = nodeName.split( ":" ) ;
      return ( parts.length == 2 ) ? parts[0] : undefined ;
   }

   // helper: get unique candidate hosts for a candidates array
   function _getCandidateHosts( candidates )
   {
      var hosts = [] ;
      for ( var j = 0; j < candidates.length; j++ )
      {
         var h = _getHost( candidates[j].NodeName ) ;
         if ( undefined != h && hosts.indexOf( h ) == -1 )
         {
            hosts.push( h ) ;
         }
      }
      hosts.sort() ;
      return hosts ;
   }

   // Pass 1: classify groups
   // status: "stable", "needSwitch", "maySwitch", "failed"
   // needSwitch: current primary not in bestCandidates, must switch
   // maySwitch: current primary in bestCandidates, rebalance=true, multiple candidate hosts
   // stable: no switch needed
   var matchedNum = groups.length ;
   var succeedNum = 0 ;
   var ignoredNum = 0 ;
   var failedNum = 0 ;
   var failedGroups = [] ;
   var detail = [] ;

   var groupStatus = [] ;
   var groupCandidates = [] ;
   var groupCandidateHosts = [] ;

   for ( var i = 0; i < groups.length; i++ )
   {
      var groupName = groups[i] ;
      var info = groupInfo[groupName] ;

      if ( undefined == info || undefined == info.primaryNode )
      {
         groupStatus.push( "failed" ) ;
         groupCandidates.push( [] ) ;
         groupCandidateHosts.push( [] ) ;
         continue ;
      }

      var currentPrimary = info.primaryNode ;
      var bestCandidates = info.bestCandidates ;

      // filter bestCandidates by FilterLevel
      // if current primary is in Maintenance, skip filtering
      var candidates = [] ;
      if ( filterLevel == "Weight" ||
           currentPrimary.RunStatusWeightDesp == "Maintenance" )
      {
         candidates = bestCandidates ;
      }
      else
      {
         for ( var j = 0; j < bestCandidates.length; j++ )
         {
            if ( allowedDesps.indexOf( bestCandidates[j].RunStatusWeightDesp ) != -1 )
            {
               candidates.push( bestCandidates[j] ) ;
            }
         }
      }

      // check if current primary is among the best candidates
      var currentIsBest = false ;
      for ( var j = 0; j < bestCandidates.length; j++ )
      {
         if ( bestCandidates[j].NodeName == currentPrimary.NodeName )
         {
            currentIsBest = true ;
            break ;
         }
      }

      var candHosts = _getCandidateHosts( candidates ) ;
      groupCandidates.push( candidates ) ;
      groupCandidateHosts.push( candHosts ) ;

      if ( !currentIsBest && candidates.length == 0 )
      {
         // no candidates pass filter, current primary is fine
         groupStatus.push( "stable" ) ;
      }
      else if ( !currentIsBest )
      {
         groupStatus.push( "needSwitch" ) ;
      }
      else if ( rebalance && candHosts.length > 1 )
      {
         // current primary is optimal, but rebalance may improve distribution
         groupStatus.push( "maySwitch" ) ;
      }
      else
      {
         groupStatus.push( "stable" ) ;
      }
   }

   // Initialize hostPrimaryCount from stable groups only
   var hostPrimaryCount = {} ;
   for ( var i = 0; i < groups.length; i++ )
   {
      if ( groupStatus[i] != "stable" )
      {
         continue ;
      }
      var info = groupInfo[groups[i]] ;
      if ( undefined != info && undefined != info.primaryNode )
      {
         var host = _getHost( info.primaryNode.NodeName ) ;
         if ( undefined != host )
         {
            if ( undefined == hostPrimaryCount[host] )
            {
               hostPrimaryCount[host] = 0 ;
            }
            hostPrimaryCount[host]++ ;
         }
      }
   }

   // Compute quota per candidate-host-set
   // hostSetQuota[key] = { hosts: [...], quota: N }
   // key = sorted hosts joined by ","
   var hostSetQuota = {} ;
   if ( rebalance )
   {
      // count total groups (needSwitch + maySwitch + stable with same host set) per host set
      var hostSetGroupCount = {} ; // key -> total group count
      var hostSetHosts = {} ; // key -> hosts array

      for ( var i = 0; i < groups.length; i++ )
      {
         if ( groupStatus[i] == "failed" )
         {
            continue ;
         }

         var candHosts = groupCandidateHosts[i] ;
         // for stable groups, derive candidate hosts from the primary's host
         // (they belong to the same host set as groups with matching candidate hosts)
         if ( groupStatus[i] == "stable" )
         {
            // stable groups don't have meaningful candHosts, skip host set counting
            // their primaries are already counted in hostPrimaryCount
            continue ;
         }

         var key = candHosts.join( "," ) ;
         if ( undefined == hostSetGroupCount[key] )
         {
            hostSetGroupCount[key] = 0 ;
            hostSetHosts[key] = candHosts ;
         }
         hostSetGroupCount[key]++ ;
      }

      // compute quota: (needSwitch + maySwitch count + existing primaries on these hosts) / host count
      for ( var key in hostSetGroupCount )
      {
         var hosts = hostSetHosts[key] ;
         var totalGroups = hostSetGroupCount[key] ;

         // add stable primaries on these hosts
         for ( var j = 0; j < hosts.length; j++ )
         {
            if ( undefined != hostPrimaryCount[hosts[j]] )
            {
               totalGroups += hostPrimaryCount[hosts[j]] ;
            }
         }

         var quota = Math.ceil( totalGroups / hosts.length ) ;
         hostSetQuota[key] = { hosts: hosts, quota: quota } ;
      }
   }

   // Pass 2: process maySwitch groups first (primary already in candidates)
   // If primary's host is within quota, keep it (ignore). Otherwise mark as needSwitch.
   for ( var i = 0; i < groups.length; i++ )
   {
      if ( groupStatus[i] != "maySwitch" )
      {
         continue ;
      }

      var info = groupInfo[groups[i]] ;
      var currentPrimary = info.primaryNode ;
      var primaryHost = _getHost( currentPrimary.NodeName ) ;

      if ( undefined == primaryHost )
      {
         groupStatus[i] = "needSwitch" ;
         continue ;
      }

      var key = groupCandidateHosts[i].join( "," ) ;
      var quota = ( undefined != hostSetQuota[key] ) ? hostSetQuota[key].quota : 0 ;
      var currentCount = ( undefined != hostPrimaryCount[primaryHost] ) ? hostPrimaryCount[primaryHost] : 0 ;

      if ( currentCount < quota )
      {
         // within quota, keep current primary
         groupStatus[i] = "stable" ;
         if ( undefined == hostPrimaryCount[primaryHost] )
         {
            hostPrimaryCount[primaryHost] = 0 ;
         }
         hostPrimaryCount[primaryHost]++ ;
      }
      else
      {
         // over quota, need to switch
         groupStatus[i] = "needSwitch" ;
      }
   }

   // Pass 3: process all groups in order
   for ( var i = 0; i < groups.length; i++ )
   {
      var groupName = groups[i] ;

      if ( groupStatus[i] == "failed" )
      {
         failedNum++ ;
         failedGroups.push( groupName ) ;
         continue ;
      }

      if ( groupStatus[i] == "stable" )
      {
         ignoredNum++ ;
         continue ;
      }

      // needSwitch
      var info = groupInfo[groupName] ;
      var currentPrimary = info.primaryNode ;
      var candidates = groupCandidates[i] ;

      // check if current primary was originally among best (maySwitch -> needSwitch)
      var currentIsBest = false ;
      for ( var j = 0; j < info.bestCandidates.length; j++ )
      {
         if ( info.bestCandidates[j].NodeName == currentPrimary.NodeName )
         {
            currentIsBest = true ;
            break ;
         }
      }

      // select target node
      var targetNode = candidates[0] ;

      if ( rebalance && candidates.length > 1 )
      {
         // pick the first candidate host (sorted order) that is under quota
         // if all at quota, fall back to the host with fewest primaries
         var candHosts = groupCandidateHosts[i] ;
         var key = candHosts.join( "," ) ;
         var quota = ( undefined != hostSetQuota[key] ) ? hostSetQuota[key].quota : 0 ;

         var bestHost = undefined ;
         var minCount = undefined ;

         // first pass: find first host under quota (sorted order)
         for ( var j = 0; j < candHosts.length; j++ )
         {
            var h = candHosts[j] ;
            var count = ( undefined != hostPrimaryCount[h] ) ? hostPrimaryCount[h] : 0 ;
            if ( count < quota )
            {
               bestHost = h ;
               break ;
            }
         }

         // fallback: all at quota, pick host with fewest primaries
         if ( undefined == bestHost )
         {
            for ( var j = 0; j < candHosts.length; j++ )
            {
               var h = candHosts[j] ;
               var count = ( undefined != hostPrimaryCount[h] ) ? hostPrimaryCount[h] : 0 ;
               if ( undefined == minCount || count < minCount )
               {
                  minCount = count ;
                  bestHost = h ;
               }
            }
         }

         // pick the first candidate on bestHost
         if ( undefined != bestHost )
         {
            for ( var j = 0; j < candidates.length; j++ )
            {
               if ( _getHost( candidates[j].NodeName ) == bestHost )
               {
                  targetNode = candidates[j] ;
                  break ;
               }
            }
         }
      }

      // if target is same as current primary, no switch needed
      if ( targetNode.NodeName == currentPrimary.NodeName )
      {
         ignoredNum++ ;
         var host = _getHost( currentPrimary.NodeName ) ;
         if ( undefined != host )
         {
            if ( undefined == hostPrimaryCount[host] )
            {
               hostPrimaryCount[host] = 0 ;
            }
            hostPrimaryCount[host]++ ;
         }
         continue ;
      }

      // determine CausedBy
      var causedBy = "" ;
      if ( currentIsBest )
      {
         causedBy = "Rebalance" ;
      }
      else
      {
         if ( currentPrimary.RunStatusWeightDesp == "Maintenance" )
         {
            causedBy = "Maintenance" ;
         }
         else
         {
            causedBy = targetNode.RunStatusWeightDesp ;
            if ( causedBy == "" )
            {
               causedBy = "Weight" ;
            }
         }
      }

      if ( run )
      {
         // execute reelect
         var parts = targetNode.NodeName.split( ":" ) ;
         if ( parts.length != 2 )
         {
            failedNum++ ;
            failedGroups.push( groupName ) ;
            continue ;
         }

         var rgOption = {
            HostName: parts[0],
            ServiceName: parts[1]
         } ;

         if ( undefined != option )
         {
            if ( undefined != option.Seconds )
            {
               rgOption.Seconds = option.Seconds ;
            }
            if ( undefined != option.Level )
            {
               rgOption.Level = option.Level ;
            }
         }

         try
         {
            var rg = this._conn.getRG( groupName ) ;
            rg.reelect( rgOption ) ;

            succeedNum++ ;
            detail.push( {
               GroupName: groupName,
               OldPrimary: currentPrimary.NodeName,
               NewPrimary: targetNode.NodeName,
               CausedBy: causedBy
            } ) ;

            // update hostPrimaryCount
            var newHost = _getHost( targetNode.NodeName ) ;
            if ( undefined != newHost )
            {
               if ( undefined == hostPrimaryCount[newHost] )
               {
                  hostPrimaryCount[newHost] = 0 ;
               }
               hostPrimaryCount[newHost]++ ;
            }
         }
         catch ( e )
         {
            failedNum++ ;
            failedGroups.push( groupName ) ;
         }
      }
      else
      {
         // analyse only
         succeedNum++ ;
         detail.push( {
            GroupName: groupName,
            OldPrimary: currentPrimary.NodeName,
            NewPrimary: targetNode.NodeName,
            CausedBy: causedBy
         } ) ;

         // update hostPrimaryCount for subsequent groups
         var newHost = _getHost( targetNode.NodeName ) ;
         if ( undefined != newHost )
         {
            if ( undefined == hostPrimaryCount[newHost] )
            {
               hostPrimaryCount[newHost] = 0 ;
            }
            hostPrimaryCount[newHost]++ ;
         }
      }
   }

   var summary = {
      MatchedNum: matchedNum,
      SucceedNum: succeedNum,
      IgnoredNum: ignoredNum,
      FailedNum: failedNum
   } ;

   if ( failedNum > 0 )
   {
      summary.FailedGroups = failedGroups ;
   }

   summary.Detail = detail ;

   return new BSONObj( summary ) ;
}

SdbDC.prototype.locationAnalyse = function( option, fileName ) {
   if ( null == option )
   {
      option = undefined ;
   }

   if ( undefined != option && ( typeof option ) != "object" )
   {
      setLastErrMsg( "SdbDC.locationAnalyse(): option should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( null == fileName )
   {
      fileName = undefined ;
   }

   if ( undefined != fileName && ( typeof fileName ) != "string" )
   {
      setLastErrMsg( "SdbDC.locationAnalyse(): fileName should be string" ) ;
      throw SDB_INVALIDARG ;
   }

   // validate option fields
   if ( undefined != option )
   {
      if ( undefined != option.HostName && ( typeof option.HostName ) != "string" )
      {
         setLastErrMsg( "SdbDC.locationAnalyse(): HostName should be string" ) ;
         throw SDB_INVALIDARG ;
      }

      if ( undefined != option.Domain )
      {
         var domainType = typeof option.Domain ;
         if ( domainType != "string" && !( option.Domain instanceof Array ) )
         {
            setLastErrMsg( "SdbDC.locationAnalyse(): Domain should be string or array of strings" ) ;
            throw SDB_INVALIDARG ;
         }
      }
   }

   // resolve domain groups
   var domainGroupNames = undefined ;
   if ( undefined != option && undefined != option.Domain )
   {
      var domains = option.Domain ;
      if ( !( domains instanceof Array ) )
      {
         domains = [ domains ] ;
      }

      domainGroupNames = [] ;
      var domainCursor = this._conn.list( SDB_LIST_DOMAINS, { Name: { $in: domains } } ) ;
      if ( undefined != domainCursor )
      {
         var domainRecord = undefined ;
         while ( ( domainRecord = domainCursor.next() ) != undefined )
         {
            var domainObj = domainRecord.toObj() ;
            if ( undefined != domainObj.Groups )
            {
               for ( var i = 0; i < domainObj.Groups.length; i++ )
               {
                  var dgn = domainObj.Groups[i].GroupName ;
                  if ( domainGroupNames.indexOf( dgn ) == -1 )
                  {
                     domainGroupNames.push( dgn ) ;
                  }
               }
            }
         }
         domainCursor.close() ;
      }

      if ( domainGroupNames.length == 0 )
      {
         setLastErrMsg( "SdbDC.locationAnalyse(): specified Domain does not exist or has no groups" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   // resolve host filter and list groups
   var filter = {} ;
   if ( undefined != option && undefined != option.HostName )
   {
      filter["Group.HostName"] = option.HostName ;
   }

   var cursor = this._conn.list( SDB_LIST_GROUPS, filter, {},
                                 { GroupName: 1 } ) ;
   if ( undefined == cursor )
   {
      return ;
   }

   // Phase 1: parse SDB_LIST_GROUPS
   // Build: groupNames[], groupNodes{groupName -> [{HostName, Location, NodeName, NodeID}]},
   //        groupActiveLocation{groupName -> string}
   var matchedGroup = false ;
   var groupNames = [] ;
   var groupNodes = {} ;
   var groupActiveLocation = {} ;
   var isCoordGroup = {} ;   // groupName -> true for coord group
   var allHosts = {} ;       // hostName -> true
   var totalNodeNum = 0 ;

   var record = undefined ;
   while ( ( record = cursor.next() ) != undefined )
   {
      matchedGroup = true ;
      var groupObj = record.toObj() ;
      var groupName = groupObj.GroupName ;

      if ( groupName == SDB_SPARE_GROUP_NAME )
      {
         continue ;
      }

      if ( undefined != domainGroupNames )
      {
         if ( domainGroupNames.indexOf( groupName ) == -1 )
         {
            continue ;
         }
      }

      groupNames.push( groupName ) ;

      if ( groupName == SDB_COORD_GROUP_NAME )
      {
         isCoordGroup[groupName] = true ;
      }

      // Coord group does not participate in ActiveLocation analysis
      if ( undefined == isCoordGroup[groupName] &&
           undefined != groupObj.ActiveLocation && groupObj.ActiveLocation != "" )
      {
         groupActiveLocation[groupName] = groupObj.ActiveLocation ;
      }

      var nodes = [] ;
      if ( undefined != groupObj.Group )
      {
         for ( var j = 0; j < groupObj.Group.length; j++ )
         {
            var nodeObj = groupObj.Group[j] ;
            var hostName = nodeObj.HostName ;
            var location = ( undefined != nodeObj.Location && nodeObj.Location != "" ) ? nodeObj.Location : "" ;
            var svcName = "" ;
            if ( undefined != nodeObj.Service )
            {
               for ( var k = 0; k < nodeObj.Service.length; k++ )
               {
                  if ( nodeObj.Service[k].Type == 0 )
                  {
                     svcName = nodeObj.Service[k].Name ;
                     break ;
                  }
               }
            }
            var nodeName = hostName + ":" + svcName ;
            nodes.push( {
               HostName: hostName,
               Location: location,
               NodeName: nodeName,
               NodeID: nodeObj.NodeID
            } ) ;
            allHosts[hostName] = true ;
            totalNodeNum++ ;
         }
      }
      groupNodes[groupName] = nodes ;
   }
   cursor.close() ;

   if ( undefined != option && undefined != option.HostName && !matchedGroup )
   {
      setLastErrMsg( "SdbDC.locationAnalyse(): specified HostName does not exist" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( groupNames.length == 0 )
   {
      var result = {
         MatchedHostNum: 0,
         MatchedGroupNum: 0,
         MatchedNodeNum: 0,
         ActiveLocation: "",
         LocationInfo: []
      } ;
      if ( undefined != fileName )
      {
         var file = new File( fileName ) ;
         file.write( JSON.stringify( result, null, 2 ) ) ;
         file.close() ;
      }
      return new BSONObj( result ) ;
   }

   // Phase 2: parse SDB_LIST_GROUPMODES
   // groupModeInfo{groupName -> {mode: "critical"/"maintenance"/"", properties: [...]}}
   var groupModeInfo = {} ;
   var modeCursor = this._conn.list( SDB_LIST_GROUPMODES ) ;
   if ( undefined != modeCursor )
   {
      var modeRecord = undefined ;
      while ( ( modeRecord = modeCursor.next() ) != undefined )
      {
         var modeObj = modeRecord.toObj() ;
         var gn = modeObj.GroupName ;
         if ( undefined == gn || undefined == groupNodes[gn] )
         {
            continue ;
         }
         var mode = ( undefined != modeObj.GroupMode && modeObj.GroupMode != "" ) ? modeObj.GroupMode : "" ;
         var props = ( undefined != modeObj.Properties ) ? modeObj.Properties : [] ;
         groupModeInfo[gn] = { mode: mode, properties: props } ;
      }
      modeCursor.close() ;
   }

   // Phase 3: build analysis structures
   // For each location, track:
   //   - which groups it appears in
   //   - which hosts it appears on (and which nodes per host)
   //   - active status per group
   //   - groupMode status per group

   // locationGroups{locName -> [groupName, ...]}
   // locationHostNodes{locName -> {hostName -> [nodeName, ...]}}
   var locationGroups = {} ;
   var locationHostNodes = {} ;
   var hasAnyLocation = false ;

   // hostLocations{hostName -> {locName -> true}}
   // hostNoLocNodes{hostName -> [nodeName, ...]}
   var hostLocations = {} ;
   var hostNoLocNodes = {} ;

   // groupLocations{groupName -> {locName -> true}}
   // groupNoLocCount{groupName -> number}
   // groupTotalCount{groupName -> number}
   var groupLocations = {} ;
   var groupNoLocCount = {} ;
   var groupTotalCount = {} ;

   // nodeIDToLocation{nodeID -> locName}  (for groupMode analysis)
   var nodeIDToLocation = {} ;

   // hostTotalNodes{hostName -> number}  (total matched nodes per host)
   var hostTotalNodes = {} ;

   for ( var i = 0; i < groupNames.length; i++ )
   {
      var gn = groupNames[i] ;
      var nodes = groupNodes[gn] ;
      groupLocations[gn] = {} ;
      groupNoLocCount[gn] = 0 ;
      groupTotalCount[gn] = nodes.length ;

      for ( var j = 0; j < nodes.length; j++ )
      {
         var nd = nodes[j] ;
         var loc = nd.Location ;
         var host = nd.HostName ;

         nodeIDToLocation[nd.NodeID] = loc ;

         if ( undefined == hostTotalNodes[host] )
         {
            hostTotalNodes[host] = 0 ;
         }
         hostTotalNodes[host]++ ;

         if ( undefined == hostLocations[host] )
         {
            hostLocations[host] = {} ;
            hostNoLocNodes[host] = [] ;
         }

         if ( loc != "" )
         {
            hasAnyLocation = true ;
            hostLocations[host][loc] = true ;
            groupLocations[gn][loc] = true ;

            // locationGroups
            if ( undefined == locationGroups[loc] )
            {
               locationGroups[loc] = [] ;
               locationHostNodes[loc] = {} ;
            }
            if ( locationGroups[loc].indexOf( gn ) == -1 )
            {
               locationGroups[loc].push( gn ) ;
            }

            // locationHostNodes
            if ( undefined == locationHostNodes[loc][host] )
            {
               locationHostNodes[loc][host] = [] ;
            }
            locationHostNodes[loc][host].push( nd.NodeName ) ;
         }
         else
         {
            hostNoLocNodes[host].push( nd.NodeName ) ;
            groupNoLocCount[gn]++ ;
         }
      }
   }

   // Phase 4: compute ActiveLocation
   var activeLocSet = {} ;
   var activeLocCount = 0 ;
   for ( var gn in groupActiveLocation )
   {
      var al = groupActiveLocation[gn] ;
      if ( undefined == activeLocSet[al] )
      {
         activeLocSet[al] = true ;
         activeLocCount++ ;
      }
   }
   var activeLocationResult ;
   if ( activeLocCount == 0 )
   {
      activeLocationResult = "" ;
   }
   else if ( activeLocCount == 1 )
   {
      for ( var al in activeLocSet )
      {
         activeLocationResult = al ;
      }
   }
   else
   {
      activeLocationResult = [] ;
      for ( var al in activeLocSet )
      {
         activeLocationResult.push( al ) ;
      }
      activeLocationResult.sort() ;
   }

   // Phase 5: build LocationInfo
   var locationInfo = [] ;
   for ( var loc in locationGroups )
   {
      var grps = locationGroups[loc] ;

      // ActiveStatus (exclude coord groups)
      var activeAll = 0 ;
      var activeNone = 0 ;
      for ( var g = 0; g < grps.length; g++ )
      {
         if ( undefined != isCoordGroup[grps[g]] )
         {
            continue ;
         }
         if ( groupActiveLocation[grps[g]] == loc )
         {
            activeAll++ ;
         }
         else
         {
            activeNone++ ;
         }
      }
      var activeStatus ;
      if ( activeAll == 0 && activeNone == 0 )
      {
         // only coord groups in this location, no data groups
         activeStatus = "None" ;
      }
      else if ( activeNone == 0 )
      {
         activeStatus = "All" ;
      }
      else if ( activeAll == 0 )
      {
         activeStatus = "None" ;
      }
      else
      {
         activeStatus = "Partical" ;
      }

      // GroupStatus (exclude coord groups)
      // For each data group this location appears in, determine if it's Critical or Maintenance
      var criticalCount = 0 ;
      var maintenanceCount = 0 ;
      var dataGroupCount = 0 ;
      for ( var g = 0; g < grps.length; g++ )
      {
         var gn = grps[g] ;
         if ( undefined != isCoordGroup[gn] )
         {
            continue ;
         }
         dataGroupCount++ ;
         var modeInfo = groupModeInfo[gn] ;
         if ( undefined == modeInfo || modeInfo.mode == "" )
         {
            continue ;
         }

         if ( modeInfo.mode == "critical" )
         {
            // check if this location is the critical target
            var isCriticalForLoc = false ;
            for ( var p = 0; p < modeInfo.properties.length; p++ )
            {
               var prop = modeInfo.properties[p] ;
               // by Location
               if ( undefined != prop.Location && prop.Location == loc )
               {
                  isCriticalForLoc = true ;
                  break ;
               }
               // by NodeID: check if node belongs to this location
               if ( undefined != prop.NodeID && 0 != prop.NodeID )
               {
                  var nodeLoc = nodeIDToLocation[prop.NodeID] ;
                  if ( nodeLoc == loc )
                  {
                     isCriticalForLoc = true ;
                     break ;
                  }
               }
            }
            if ( isCriticalForLoc )
            {
               criticalCount++ ;
            }
         }
         else if ( modeInfo.mode == "maintenance" )
         {
            // check if any maintenance node belongs to this location
            var isMaintenanceForLoc = false ;
            for ( var p = 0; p < modeInfo.properties.length; p++ )
            {
               var prop = modeInfo.properties[p] ;
               if ( undefined != prop.NodeID && 0 != prop.NodeID )
               {
                  var nodeLoc = nodeIDToLocation[prop.NodeID] ;
                  if ( nodeLoc == loc )
                  {
                     isMaintenanceForLoc = true ;
                     break ;
                  }
               }
            }
            if ( isMaintenanceForLoc )
            {
               maintenanceCount++ ;
            }
         }
      }

      var groupStatus ;
      var hasCritical = criticalCount > 0 ;
      var hasMaintenance = maintenanceCount > 0 ;
      var allCritical = ( dataGroupCount > 0 && criticalCount == dataGroupCount ) ;
      var allMaintenance = ( dataGroupCount > 0 && maintenanceCount == dataGroupCount ) ;

      if ( !hasCritical && !hasMaintenance )
      {
         groupStatus = "" ;
      }
      else if ( hasCritical && !hasMaintenance )
      {
         groupStatus = allCritical ? "Critical" : "ParticalCritical" ;
      }
      else if ( !hasCritical && hasMaintenance )
      {
         groupStatus = allMaintenance ? "Maintenance" : "ParticalMaintenance" ;
      }
      else
      {
         // both exist, since same group is mutually exclusive,
         // criticalCount + maintenanceCount <= grps.length
         if ( criticalCount + maintenanceCount == dataGroupCount )
         {
            groupStatus = "Critical-Maintenance" ;
         }
         else
         {
            groupStatus = "Partical-Critical-Maintenance" ;
         }
      }

      // WholeHost and ParticalHost
      var wholeHost = [] ;
      var particalHost = [] ;
      var hostNodes = locationHostNodes[loc] ;
      for ( var h in hostNodes )
      {
         var totalOnHost = ( undefined != hostTotalNodes[h] ) ? hostTotalNodes[h] : 0 ;

         if ( hostNodes[h].length == totalOnHost )
         {
            wholeHost.push( h ) ;
         }
         else
         {
            particalHost.push( {
               HostName: h,
               Node: hostNodes[h]
            } ) ;
         }
      }
      wholeHost.sort() ;
      particalHost.sort( function( a, b ) {
         return a.HostName < b.HostName ? -1 : ( a.HostName > b.HostName ? 1 : 0 ) ;
      } ) ;
      for ( var pi = 0; pi < particalHost.length; pi++ )
      {
         particalHost[pi].Node.sort() ;
      }

      var locItem = {
         LocationName: loc,
         ActiveStatus: activeStatus,
         GroupStatus: groupStatus,
         WholeHost: wholeHost
      } ;
      if ( particalHost.length > 0 )
      {
         locItem.ParticalHost = particalHost ;
      }
      locationInfo.push( locItem ) ;
   }

   locationInfo.sort( function( a, b ) {
      return a.LocationName < b.LocationName ? -1 : ( a.LocationName > b.LocationName ? 1 : 0 ) ;
   } ) ;

   // Phase 6: ExceptionHostInfo and ExceptionGroupInfo
   var noLocationHost = [] ;
   var particalLocationHost = [] ;
   var multyLocationHost = [] ;
   var noLocationGroup = [] ;
   var particalLocationGroup = [] ;
   var oneLocationGroup = [] ;

   if ( hasAnyLocation )
   {
      // host analysis
      for ( var h in allHosts )
      {
         var locs = hostLocations[h] ;
         var noLocs = hostNoLocNodes[h] ;
         var locCount = 0 ;
         for ( var l in locs )
         {
            locCount++ ;
         }
         var hasNoLoc = ( undefined != noLocs && noLocs.length > 0 ) ;

         if ( locCount == 0 )
         {
            noLocationHost.push( h ) ;
         }
         else
         {
            if ( hasNoLoc )
            {
               particalLocationHost.push( h ) ;
            }
            if ( locCount > 1 )
            {
               multyLocationHost.push( h ) ;
            }
         }
      }
      noLocationHost.sort() ;
      particalLocationHost.sort() ;
      multyLocationHost.sort() ;

      // group analysis
      for ( var i = 0; i < groupNames.length; i++ )
      {
         var gn = groupNames[i] ;
         var total = groupTotalCount[gn] ;
         var noLoc = groupNoLocCount[gn] ;
         var locs = groupLocations[gn] ;
         var locCount = 0 ;
         for ( var l in locs )
         {
            locCount++ ;
         }

         if ( noLoc == total )
         {
            noLocationGroup.push( gn ) ;
         }
         else
         {
            if ( noLoc > 0 )
            {
               particalLocationGroup.push( gn ) ;
            }
            if ( locCount == 1 )
            {
               oneLocationGroup.push( gn ) ;
            }
         }
      }
   }

   // Phase 7: build result
   var hostCount = 0 ;
   for ( var h in allHosts )
   {
      hostCount++ ;
   }

   var result = {
      MatchedHostNum: hostCount,
      MatchedGroupNum: groupNames.length,
      MatchedNodeNum: totalNodeNum,
      ActiveLocation: activeLocationResult,
      LocationInfo: locationInfo
   } ;

   if ( noLocationHost.length > 0 ||
        particalLocationHost.length > 0 ||
        multyLocationHost.length > 0 )
   {
      result.ExceptionHostInfo = {
         NoLocationHost: noLocationHost,
         ParticalLocationHost: particalLocationHost,
         MultyLocationHost: multyLocationHost
      } ;
   }

   if ( noLocationGroup.length > 0 ||
        particalLocationGroup.length > 0 ||
        oneLocationGroup.length > 0 )
   {
      result.ExceptionGroupInfo = {
         NoLocationGroup: noLocationGroup,
         ParticalLocationGroup: particalLocationGroup,
         OneLocationGroup: oneLocationGroup
      } ;
   }

   if ( undefined != fileName )
   {
      var file = new File( fileName ) ;
      file.write( JSON.stringify( result, null, 2 ) ) ;
      file.close() ;
   }

   return new BSONObj( result ) ;
}

// end SdbDc

// SdbSequence
SdbSequence.prototype.toString = function() {
   return this._conn.toString() + "." + this._name;
}
// end SdbSequence

// Sdb
Sdb.prototype.toString = function() {
   return this._host + ":" + this._port;
}

Sdb.prototype.listCollectionSpaces = function() {
   return this.list( SDB_LIST_COLLECTIONSPACES ) ;
}

Sdb.prototype.listCollections = function() {
   return this.list( SDB_LIST_COLLECTIONS ) ;
}

Sdb.prototype.listSequences = function() {
   return this.list( SDB_LIST_SEQUENCES ) ;
}

Sdb.prototype.listReplicaGroups = function() {
   return this.list( SDB_LIST_GROUPS ) ;
}

Sdb.prototype.getTask = function( id ) {
   if ( typeof( id ) == 'undefined' )
   {
      setLastErrMsg( "Task id must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( typeof( id ) != 'number' )
   {
      setLastErrMsg( "Task id must be number" ) ;
      throw SDB_INVALIDARG ;
   }

   var obj = this.listTasks( { TaskID: id } ).next() ;
   if (undefined == obj)
   {
      setLastError( SDB_CAT_TASK_NOTFOUND ) ;
      setLastErrMsg( getErr( SDB_CAT_TASK_NOTFOUND ) ) ;
      throw SDB_CAT_TASK_NOTFOUND ;
   }
   return obj ;
}

Sdb.prototype._resolveCS = function(csName) {
   if( !this.hasOwnProperty( csName ) )
   {
      return this.getCS( csName ) ;
   }
}

// getCatalogRG will be remove, suggest using getCataRG
Sdb.prototype.getCatalogRG = function() {
   return this.getCataRG() ;
}

Sdb.prototype.getCataRG = function() {
   return this.getRG( SDB_CATALOG_GROUP_NAME ) ;
}

// removeCatalogRG will be remove, suggest using removeCataRG
Sdb.prototype.removeCatalogRG = function() {
   return this.removeCataRG() ;
}

Sdb.prototype.removeCataRG = function() {
   return this.removeRG( SDB_CATALOG_GROUP_NAME ) ;
}

Sdb.prototype.createCoordRG = function() {
   return this.createRG( SDB_COORD_GROUP_NAME ) ;
}

Sdb.prototype.removeCoordRG = function() {
   return this.removeRG( SDB_COORD_GROUP_NAME ) ;
}

Sdb.prototype.getCoordRG = function() {
   return this.getRG( SDB_COORD_GROUP_NAME ) ;
}

Sdb.prototype.createSpareRG = function() {
   return this.createRG(SDB_SPARE_GROUP_NAME) ;
}

Sdb.prototype.getSpareRG = function() {
   return this.getRG(SDB_SPARE_GROUP_NAME) ;
}

Sdb.prototype.removeSpareRG  = function() {
   return this.removeRG( SDB_SPARE_GROUP_NAME ) ;
}

Sdb.prototype.stopRG = function() {
   for( var index in arguments )
   {
      var rgName = arguments[ index ] ;
      if ( "string" != typeof( rgName ) )
      {
         setLastErrMsg( "Sdb.stopRG(): wrong arguments" ) ;
         throw SDB_INVALIDARG ;
      }
      try
      {
         this.getRG( rgName ).stop() ;
      }
      catch( e )
      {
         setLastErrMsg( rgName + ": " + getLastErrMsg() ) ;
         throw e ;
      }
   }
}

Sdb.prototype._getTraceInfo = function()
{
   var path          = "" ;
   var cmPort        = "" ;
   var localIP       = "" ;
   var traceInfo     = {} ;
   var traceHostname = "" ;
   var localHostname = "" ;

   try
   {
      var retObj = this.snapshot( SDB_SNAP_CONFIGS, { "Global": false },
                                  { "NodeName": 1,
                                    "tmppath": 1 } ).next().toObj() ;
   }
   catch( e )
   {
      setLastErrMsg( getLastErrMsg() + " Failed to get trace info" ) ;
      throw e ;
   }

   traceHostname = retObj.NodeName.split( ":" )[0] ;
   localHostname = System.getHostName() ;
   cmPort        = Oma.getAOmaSvcName( traceHostname ) ;
   localIP       = System.getAHostMap( localHostname ) ;

   // The format of the NodeName:
   // 1. u1604-fngjiabin:50000
   // 2. 192.168.20.71:50000
   // 3. 127.0.0.1:50000
   // 4. localhost:50000
   if( traceHostname != localHostname && traceHostname != localIP &&
       traceHostname != "127.0.0.1" && traceHostname != "localhost" )
   {
      path = retObj.tmppath + "tmp.dump" ;
   }

   traceInfo[CM_PORT]        = cmPort ;
   traceInfo[TMP_PATH]       = path ;
   traceInfo[TRACE_HOSTNAME] = traceHostname ;

   return traceInfo ;
}

Sdb.prototype.traceOff = function()
{
   var path          = "" ;
   var cmPort        = "" ;
   var traceHostname = "" ;
   var argumentsSize = arguments.length ;
   var traceInfo ;

   if ( 0 == argumentsSize )
   {
      setLastErrMsg( "FileName must be configured" ) ;
      throw SDB_INVALIDARG ;
   }

   if( argumentsSize >= 1 )
   {
      if( "string" != typeof( arguments[0] ) )
      {
         setLastErrMsg( "FileName must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }

   if( argumentsSize >= 2 )
   {
      if( "boolean" != typeof( arguments[1] ) )
      {
         setLastErrMsg( "The second parameter must be bool" ) ;
         throw SDB_INVALIDARG ;
      }

      if( arguments[1] && arguments[0].length > 0 )
      {
         traceInfo     = this._getTraceInfo() ;
         path          = traceInfo.TMP_PATH ;
         cmPort        = traceInfo.CM_PORT ;
         traceHostname = traceInfo.TRACE_HOSTNAME ;
      }
   }

   if ( argumentsSize > 3 )
   {
      setLastErrMsg( "traceOff support only two parameters" ) ;
	  throw SDB_INVALIDARG ;
   }

   if( "" != path )
   {
      try
      {
         var remote = new Remote( traceHostname, cmPort ) ;
      }
      catch( e )
      {
         setLastErrMsg( getLastErrMsg() +
                        ". You can check if there is a cm process " +
                        "on the host[" + traceHostname + ":" + cmPort + "]. "
                        + "\n" + "Or check whether the network is normal" ) ;
         throw e ;
      }

      if( File.exist( arguments[0] ) )
      {
         if( File.getSize( arguments[0] ) < 2 )
         {
            setLastErrMsg( "The file[" + arguments[0] +
                           "] exists. But it isn't trace file" ) ;
            throw SDB_FE ;
         }

         var file = new File( arguments[0] ) ;
         var eyeCatcher = file.read( 2 ) ;
         if( "TB" != eyeCatcher )
         {
            setLastErrMsg( "The file[" + arguments[0] +
                           "] is exist. But it isn't trace file" ) ;
            throw SDB_PD_TRACE_FILE_INVALID ;
         }
      }

      this._traceOff( path ) ;

      try
      {
         var src = traceHostname + ":" + cmPort + "@" + path ;
         var des = arguments[0] ;
         File.scp( src, des, true, 0640 ) ;
      }
      catch( e )
      {
         setLastErrMsg( getLastErrMsg() +
                        " Failed to scp. The tmp trace file is in " +
                        traceHostname + ":" + path ) ;
         throw e ;
      }

      try
      {
         var remote = new Remote( traceHostname, cmPort ) ;
         var remoteFile = remote.getFile() ;
         remoteFile.remove( path ) ;
      }
      catch( e )
      {
         setLastErrMsg( getLastErrMsg() +
                        " Failed to remove tmp trace file" ) ;
         throw e ;
      }
   }
   else
   {
      if ( argumentsSize > 0 )
      {
         this._traceOff( arguments[0] ) ;
      }
      else
      {
         this._traceOff() ;
      }
   }
}

SecureSdb.prototype._resolveCS = function(csName) {
   if( !this.hasOwnProperty( csName ) )
   {
      return this.getCS( csName ) ;
   }
}
// end Sdb

function printCallStack()
{
   try
   {
      throw new Error( "print ErrStack" ) ;
   }
   catch ( e )
   {
      print( e.stack ) ;
   }
}

function assert( condition )
{
   if ( !condition )
   {
      printCallStack() ;
   }
}

// ObjectId
if ( !ObjectId.prototype )
   ObjectId.prototype = {}

ObjectId.prototype.toString = function() {
   return "ObjectId(\"" + this._str + "\")" ;
}
// end ObjectId

// BinData
if ( !BinData.prototype )
   BinData.prototype = {}

BinData.prototype.toString = function() {
   return "BinData(\"" + this._data + "\", \"" + this._type + "\")"  ;
}


// end BinData

// Timestamp
if ( !Timestamp.prototype )
   Timestamp.prototype = {}

Timestamp.prototype.toString = function() {
   return "Timestamp(\"" + this._t + "\")" ;
}
// end Timestamp

// Regex
if ( !Regex.prototype )
   Regex.prototype = {}

Regex.prototype.toString = function () {
   return "Regex(\"" + this._regex + "\", \"" + this._option + "\")" ;
}
// end Regex

// MinKey
if ( !MinKey.prototype )
   MinKey.prototype = {}

MinKey.prototype.toString = function() {
   return "MinKey()" ;
}
// end MinKey

// MaxKey
if ( !MaxKey.prototype )
   MaxKey.prototype = {}

MaxKey.prototype.toString = function() {
   return "MaxKey()" ;
}
// end MaxKey

// NumberLong
if ( !NumberLong.prototype )
   NumberLong.prototype = {}

NumberLong.prototype.toString = function() {
   if ( typeof(this._v ) == "string" )
   {
      return "NumberLong(\"" + this._v + "\")" ;
   }
   return "NumberLong(" + this._v + ")" ;
}

NumberLong.prototype.valueOf = function() {
   if ( typeof(this._v ) == "string" )
   {
      return parseInt(this._v) ;
   }
   return this._v ;
}

// end NumberLong

// NumberDecimal
if ( !NumberDecimal.prototype )
   NumberDecimal.prototype = {}

NumberDecimal.prototype.valueOf = function() {
   var decimalNumber = this._decimal ;

   if ( typeof( this._decimal ) == "string" )
   {
      if( isNaN( this._decimal) )
      {
         throw "Decimal data must be number or numeric string" ;
      }
      decimalNumber = parseInt(this._decimal) ;
   }

   return decimalNumber ;
}

NumberDecimal.prototype.toString = function() {
   var decimalNumber = this._decimal ;
   var precision = "";

   if( typeof( this._precision ) != "undefined" )
   {
      if( 0 == this._precision.length )
      {
         precision = "" ;
      }
      else
      {
         precision = ", [" + this._precision +"]" ;
      }
   }

   return "NumberDecimal( " + decimalNumber + precision + " )" ;
}

// end NumberDecimal

// SdbDate
if ( !SdbDate.prototype )
   SdbDate.prototype = {}

SdbDate.prototype.toString = function() {
   return "SdbDate(\"" + this._d + "\")" ;
}
// end SdbDate

// SdbOptionBase

SdbOptionBase.prototype.cond = function(cond) {
   this._cond = BSONObj(cond) ;
   return this ;
}

SdbOptionBase.prototype.sel = function(sel) {
   this._sel = BSONObj(sel) ;
   return this ;
}

SdbOptionBase.prototype.sort = function(sort) {
   this._sort = BSONObj(sort) ;
   return this ;
}

SdbOptionBase.prototype.hint = function(hint) {
   this._hint = BSONObj(hint) ;
   return this ;
}

SdbOptionBase.prototype.skip = function(skip) {
   if ( typeof( skip ) == "number" ) {
      this._skip = skip ;
   } else {
      setLastErrMsg( "SdbOptionBase.skip() param must be Number" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

SdbOptionBase.prototype.limit = function(limit) {
   if ( typeof( limit ) == "number" ) {
      this._limit = limit ;
   } else {
      setLastErrMsg( "SdbOptionBase.limit() param must be Number" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

SdbOptionBase.prototype.flags = function(flags) {
   if ( typeof ( flags ) == "number" ) {
      this._flags = flags ;
   } else {
      setLastErrMsg( "SdbOptionBase.flags() param must be Number" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

SdbOptionBase.prototype.toString = function() {
   return this.__className__ + "(" + "\"cond\": " + this._cond.toJson() +
          ", \"sel\": " + this._sel.toJson() +
          ", \"sort\": " + this._sort.toJson() +
          ", \"hint\": " + this._hint.toJson() +
          ", \"skip\": " + this._skip +
          ", \"limit\": " + this._limit +
          ", \"flags\": " + this._flags + ")" ;
}

// end SdbOptionBase

// SdbSnapshotOption

SdbSnapshotOption.prototype.options = function(options) {
   if (undefined != options && (typeof options) != "object") {
      setLastErrMsg( "SdbSnapshotOption.options(): param should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   this._hint = BSONObj({$Options:BSONObj(options)}) ;
   return this ;
}

// end SdbSnapshotOption

// SdbQueryOption

SdbQueryOption.prototype.update = function( rule, returnNew, options ) {
   if ((typeof rule) != "object" || isEmptyObject(rule)) {
      setLastErrMsg( "SdbQueryOption.update(): the 1st param should be "
                     + "non-empty object" ) ;
      throw SDB_INVALIDARG ;
   }
   if (undefined != returnNew && (typeof returnNew) != "boolean") {
      setLastErrMsg( "SdbQueryOption.update(): the 2nd param "
                     + "should be boolean" ) ;
      throw SDB_INVALIDARG;
   }
   if (undefined != options && (typeof options) != "object") {
      setLastErrMsg( "SdbQueryOption.update(): the 3rd param "
                     + "should be object" ) ;
      throw SDB_INVALIDARG ;
   }

   var hintObj = eval('(' + this._hint.toString() + ')');

   if (undefined == this._hint) {
      this._hint = {};
   } else if ( undefined != hintObj.$Modify ) {
      setLastErrMsg( "SdbQueryOption.update(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "update";
   modify.Update = rule;
   modify.ReturnNew = (returnNew != undefined) ? returnNew : false ;
   hintObj["$Modify"] = modify ;
   this._hint = BSONObj( hintObj );

   if (undefined != options) {
      this._options = BSONObj( options ) ;
   }

   return this;
}

SdbQueryOption.prototype.remove = function() {

   var hintObj = eval('(' + this._hint.toString() + ')');

   if (undefined == this._hint) {
      this._hint = {};
   } else if ( undefined != hintObj.$Modify ) {
      setLastErrMsg( "SdbQueryOption.remove(): duplicate modification" ) ;
      throw SDB_INVALIDARG ;
   }

   var modify = {};
   modify.OP = "remove";
   modify.Remove = true;
   hintObj["$Modify"] = modify ;
   this._hint = BSONObj( hintObj );

   return this;
}

// end SdbQueryOption

// SdbTraceOption

SdbTraceOption.prototype.components = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      // the format that user specifies component parameter is like
      // .components( [ "dms", "rtn" ] )
      if( arguments[0] instanceof Array )
      {
         // if the type of the first parameter is array,
         // the method only needs one parameter
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid components' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         // the format that user specifies component parameter is like
         // .components( [] )
         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "Components can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            // the format that user specifies component parameter is like
            // .components( [ "", "" ] )
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "Component can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            // the format that user specifies component parameter is like
            // .components( [ 123, 456 ] )
            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "Component must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._components ) == "undefined" )
         {
            this._components = arguments[0] ;
         }
         // After we call the component method,
         // we can call the component method again
         // eg:
         // > var option = new SdbTraceOption().component( [ "rtn", "dms" ] )
         // now the component is [ "rtn", "dms" ]
         // > option.component( [ "oss", "mth" ] )
         // now the component is [ "rtn", "dms", "oss", "mth" ]
         else
         {
            this._components = this._components.concat( arguments[0] ) ;
         }
      }

      // the format that user specifies component parameter is like
      // .components( "dms", "rtn" )
      else
      {
         if( typeof( this._components ) == "undefined" )
         {
            this._components = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            // the format that user specifies component parameter is like
            // .components( "", "" )
            if ( "" == arguments[i] )
            {
               setLastErrMsg( "Component can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            // the format that user specifies component parameter is like
            // .components( 123, 456 )
            if ( typeof( arguments[i] ) != "string" )
            {
               setLastErrMsg( "Component must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }

            this._components.push( arguments[i] );
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.breakPoints = function( breakPoints )
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid breakPoints' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "Breakpoints can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "Breakpoint can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "Breakpoint must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._breakPoints ) == "undefined" )
         {
            this._breakPoints = arguments[0] ;
         }
         else
         {
            this._breakPoints = this._breakPoints.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._breakPoints ) == "undefined" )
         {
            this._breakPoints = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if ( "" == arguments[i] )
            {
               setLastErrMsg( "Breakpoint can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[i] ) != "string" )
            {
               setLastErrMsg( "Breakpoint must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }

            this._breakPoints.push( arguments[i] );
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.tids = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid tids' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "Tids can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( typeof( arguments[0][i] ) != "number" )
            {
               setLastErrMsg( "Tid must be int or int array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._tids ) == "undefined" )
         {
            this._tids = arguments[0] ;
         }
         else
         {
            this._tids = this._tids.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._tids ) == "undefined" )
         {
            this._tids = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if ( typeof( arguments[i] ) != "number" )
            {
               setLastErrMsg( "Tid must be int or int array" ) ;
               throw SDB_INVALIDARG ;
            }

            this._tids.push( arguments[i] );
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.functionNames = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid functionNames' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "FunctionNames can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "FunctionName can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "FunctionName must be string or string array" ) ;
               throw SDB_INVALIDARG ;
            }
         }

         if( typeof( this._functionNames ) == "undefined" )
         {
            this._functionNames = arguments[0] ;
         }
         else
         {
            this._functionNames = this._functionNames.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._functionNames ) == "undefined" )
         {
            this._functionNames = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if( arguments[i] instanceof Array )
            {
               setLastErrMsg( "Invalid functionNames' parameters" ) ;
               throw SDB_INVALIDARG ;
            }
            else
            {
               if ( "" == arguments[i] )
               {
                  setLastErrMsg( "FunctionName can't be empty" ) ;
                  throw SDB_INVALIDARG ;
               }

               if ( typeof( arguments[i] ) != "string" )
               {
                  setLastErrMsg( "FunctionName must be string or string array" ) ;
                  throw SDB_INVALIDARG ;
               }

               this._functionNames.push( arguments[i] );
            }
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.threadTypes = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if( arguments[0] instanceof Array )
      {
         if( argumentsSize > 1 )
         {
            setLastErrMsg( "Invalid threadTypes' parameters" ) ;
            throw SDB_INVALIDARG ;
         }

         if ( 0 == arguments[0].length )
         {
            setLastErrMsg( "ThreadTypes can't be empty" ) ;
            throw SDB_INVALIDARG ;
         }

         for ( var i = 0; i < arguments[0].length; i++ )
         {
            if ( "" == arguments[0][i] )
            {
               setLastErrMsg( "ThreadType can't be empty" ) ;
               throw SDB_INVALIDARG ;
            }

            if ( typeof( arguments[0][i] ) != "string" )
            {
               setLastErrMsg( "ThreadType must be string or string array" ) ;
               throw SDB_INVALIDARG
            }
         }

         if( typeof( this._threadTypes ) == "undefined" )
         {
            this._threadTypes = arguments[0] ;
         }
         else
         {
            this._threadTypes = this._threadTypes.concat( arguments[0] ) ;
         }
      }
      else
      {
         if( typeof( this._threadTypes ) == "undefined" )
         {
            this._threadTypes = [] ;
         }

         for ( var i = 0; i < argumentsSize; i++ )
         {
            if( arguments[i] instanceof Array )
            {
               setLastErrMsg( "Invalid threadTypes' parameters" ) ;
               throw SDB_INVALIDARG ;
            }
            else
            {
               if ( "" == arguments[i] )
               {
                  setLastErrMsg( "ThreadType can't be empty" ) ;
                  throw SDB_INVALIDARG ;
               }

               if ( typeof( arguments[i] ) != "string" )
               {
                  setLastErrMsg( "ThreadType must be string or string array" ) ;
                  throw SDB_INVALIDARG ;
               }

               this._threadTypes.push( arguments[i] );
            }
         }
      }
   }

   return this ;
}

SdbTraceOption.prototype.toString = function()
{
   var componentsStr ;
   var breakPointsStr ;
   var tidsStr ;
   var funcNamesStr ;
   var threadTypesStr ;

   // User doesn't specify component parameter
   if( typeof( this._components ) == "undefined" )
   {
      componentsStr = "[]" ;
   }
   else
   {
      // the value that shell or engine returns
      if( this._components[0] != "[" )
      {
         componentsStr = "[ "  + "\"" + this._components.join("\", \"") +
                         "\"" + " ]" ;
      }
      // the value that fmp returns
      else
      {
         componentsStr = this._components ;
      }
   }

   if( typeof( this._breakPoints ) == "undefined" )
   {
      breakPointsStr = "[]" ;
   }
   else
   {
      if( this._breakPoints[0] != "[" )
      {
         breakPointsStr = "[ "  + "\"" + this._breakPoints.join("\", \"") +
                          "\"" + " ]" ;
      }
      else
      {
         breakPointsStr = this._breakPoints ;
      }
   }

   if( typeof( this._tids ) == "undefined" )
   {
      tidsStr = "[]" ;
   }
   else
   {
      if( this._tids[0] != "[" )
      {
         tidsStr = "[ "  + this._tids.join(", ") + " ]" ;
      }
      else
      {
         tidsStr = this._tids ;
      }
   }

   if( typeof( this._functionNames ) == "undefined" )
   {
      funcNamesStr = "[]" ;
   }
   else
   {
      if( this._functionNames[0] != "[" )
      {
         funcNamesStr = "[ "  + "\"" + this._functionNames.join("\", \"") +
                        "\"" + " ]" ;
      }
      else
      {
         funcNamesStr = this._functionNames ;
      }
   }

   if( typeof( this._threadTypes ) == "undefined" )
   {
      threadTypesStr = "[]" ;
   }
   else
   {
      if( this._threadTypes[0] != "[" )
      {
         threadTypesStr = "[ "  + "\"" + this._threadTypes.join("\", \"") +
                          "\"" + " ]" ;
      }
      else
      {
         threadTypesStr = this._threadTypes ;
      }
   }

   return this.__className__ +
          "( " +
          "\"components\": " + componentsStr +
          ", \"breakPoints\": " + breakPointsStr +
          ", \"tids\": " + tidsStr +
          ", \"functionNames\": " + funcNamesStr +
          ", \"threadTypes\": " + threadTypesStr +
          " )" ;
}

// end SdbTraceOption

// User
User.prototype.promptPassword = function()
{
   if ( "function" != typeof ( this._promptPassword ) )
   {
      throw "Fmp can't use promptPassword function" ;
   }
   this._promptPassword() ;
   return this ;
}

User.prototype.getUsername = function() {
   return this._user ;
}

User.prototype.toString = function() {
   return this._user ;
}

// end User

// CipherUser
CipherUser.prototype.token = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if ( "string" == typeof( arguments[0] ) )
      {
         this._setToken( arguments[0] ) ;
      }
      else
      {
         setLastErrMsg( "Token must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   else
   {
      setLastErrMsg( "You must input token" ) ;
      throw SDB_INVALIDARG ;
   }

   return this ;
}

CipherUser.prototype.clusterName = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if ( "string" == typeof( arguments[0] ) )
      {
         this._clusterName = arguments[0] ;
      }
      else
      {
         setLastErrMsg( "Cluster name must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   else
   {
      setLastErrMsg( "You must input cluster name" ) ;
      throw SDB_INVALIDARG ;
   }

   return this ;
}

CipherUser.prototype.cipherFile = function()
{
   var argumentsSize = arguments.length ;
   if( argumentsSize > 0 )
   {
      if ( "string" == typeof( arguments[0] ) )
      {
         this._cipherFile = arguments[0] ;
      }
      else
      {
         setLastErrMsg( "Cipher file must be string" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   else
   {
      setLastErrMsg( "You must input cipher file" ) ;
      throw SDB_INVALIDARG ;
   }

   return this ;
}

CipherUser.prototype.getUsername = function()
{
   return this._user ;
}

CipherUser.prototype.getClusterName = function()
{
   return this._clusterName ;
}

CipherUser.prototype.getCipherFile = function()
{
   return this._cipherFile ;
}

CipherUser.prototype.toString = function()
{
   var output = this._user ;
   if ( "undefined" != typeof ( this._clusterName ) && "" != this._clusterName )
   {
      output = ( output + "@" + this._clusterName ) ;
   }
   if ( "undefined" != typeof ( this._cipherFile ) && "" != this._cipherFile )
   {
      output = ( output + ":" + this._cipherFile ) ;
   }
   return output ;
}

// end CipherUser

SdbDataSource.prototype.help = function()
{
    println() ;
    println( '   --Instance methods for class "SdbDataSource"' ) ;
    println( '   alter(<options>)         - Alter data source options' ) ;
}
