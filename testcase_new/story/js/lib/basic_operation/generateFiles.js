/*******************************************************************************
@description 生成该目录下除 assert 文件之外的所有包装类（包括 commlib）
            1. 使用 showClass 打印所有内置对象和全局方法
            2. 使用 showClass("内置对象") 打印出内置对象所有的静态方法和成员方法
            3. 使用 addMemPrivateFunc 和 addMemPrivateFunc 为内置类添加隐藏方法（暂时不知道自动获取，只有写死）
            4. 往文件中写声明和封装方法语句（装饰器模式），当内置类被封装，其内所有方法丢失，所以内置类最早封装
               另外，私有方法不进行抛错处理，可能内部会有根据错误码操作
            5. 生成 commlib.js 并提供 Sdb 实例和 Assert 实例
@author  lyy
@time  2020-09-25
*******************************************************************************/
generateFiles();
function generateFiles ()
{
   // 分出内置类和全局方法
   // 内置类：str[0]
   // 全局方法：str[1];
   var str = showClass().split( ":\n" ).map( function( classAndFunc ) { return classAndFunc.split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim() } ); } ).slice( 1 );
   var commFile = getFile( "./commlib.js" );

   // 封装内置类
   var classes = str[0];
   for( var i = 0; i < classes.length; i++ )
   {
      // 获取静态和成员方法
      var staicFunc = showClass( classes[i] ).split( "static functions:\n" )[1].split( "'s member functions" )[0].split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim().slice( 0, -2 ) } );
      var memFunc = showClass( classes[i] ).split( "member functions:\n" )[1].split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim().slice( 0, -2 ) } );

      // 添加私有方法
      staicFunc = addStaPrivateFunc( classes[i], staicFunc );
      memFunc = addMemPrivateFunc( classes[i], memFunc );

      var fileName = classes[i] + ".js";
      var file = getFile( fileName );

      // e.g: var tmpSdb = { dropCS: Sdb.prototype.dropCS, createCS: Sdb.prototype.createCS };
      var varStr = "var tmp" + classes[i] + " = {";
      for( var j = 0; j < memFunc.length; j++ )
      {
         varStr += "\n   " + memFunc[j] + ": " + classes[i] + ".prototype." + memFunc[j];
         if( j != memFunc.length - 1 ) { varStr += ","; } else { varStr += "\n};"; }
      }
      file.write( varStr + "\n" );

      // e.g: var funcSdb = Sdb
      var varStr = "var func" + classes[i] + " = " + classes[i] + ";";
      file.write( varStr + "\n" );

      // e.g: var funcFilechgrp = File.chgrp
      for( var j = 0; j < staicFunc.length; j++ )
      {
         var varStr = "var func" + classes[i] + staicFunc[j] + " = " + classes[i] + "." + staicFunc[j] + ";";
         file.write( varStr + "\n" );
      }

      // e.g:  Sdb = function() { try { return funcSdb.apply( this, arguments ); } catch( e ) { commThrowError(e); } }
      var evalStr = classes[i] + "=function(){try{return func" + classes[i] + ".apply( this, arguments ); } catch( e ) { commThrowError(e) } };";
      file.write( evalStr + "\n" );

      /* e.g:
         File.chgrp = function(){
            try{ return   funcFilechgrp.apply(this,arguments);  }
            catch(e) { commThrowError(e);  }
         }
      */
      for( var j = 0; j < staicFunc.length; j++ )
      {
         var evalStr = classes[i] + "." + staicFunc[j] + " = function(){" +
            "try{ return func" + classes[i] + staicFunc[j] + ".apply( this, arguments ); } catch( e ) { commThrowError(e) } };";
         file.write( evalStr + "\n" );
      }

      /* e.g: 
         Sdb.prototype.dropCS =
            function()
            {
               try { return tmpSdb.dropCS.apply( this, arguments ); }
               catch( e ) {  commThrowError(e);  }
            }
      */
      for( var j = 0; j < memFunc.length; j++ )
      {
         var evalStr = classes[i] + ".prototype." + memFunc[j] + "=function(){try{return tmp" + classes[i] + "." + memFunc[j]
            + ".apply(this,arguments);}catch(e){commThrowError(e);}};";
         file.write( evalStr + "\n" );
      }

      commFile.write( 'import( "./' + fileName + '" );' + "\n" );
      file.close();
   }

   // 封装全局方法
   var file = getFile( "./Global.js" );
   var funcs = str[1].filter( function( s ) { if( s.indexOf( "import" ) == -1 ) { return true; } } ).map( function( s ) { return s.trim().slice( 0, -2 ) } );

   // e.g: var tmpGlobal = { sleep: sleep, setLastErrMsg: setLastErrMsg }
   var varStr = "var tmpGlobal = {";
   for( var i = 0; i < funcs.length; i++ )
   {
      varStr += "\n   " + funcs[i] + ": " + funcs[i];
      if( i != funcs.length - 1 ) { varStr += ","; } else { varStr += "\n};"; }
   }
   file.write( varStr + "\n" );

   /* e.g:
     sleep =
       function()
       {
          try { return tmpGlobal.sleep.apply( this, arguments ); }
          catch( e ) {  throw new Error( e ); } 
       }
   */
   for( var i = 0; i < funcs.length; i++ )
   {
      var evalStr = funcs[i] + "=function(){try{return tmpGlobal." + funcs[i] + ".apply(this,arguments);}"
         + "catch(e){commThrowError(e)}};";
      file.write( evalStr + "\n" );
   }

   commFile.write( 'import( "./Global.js" );' + "\n" );
   commFile.write( 'import( "./assert.js" );' + "\n" );
   commFile.write( 'var assert = new Assert();' + "\n" );
   commFile.write( 'var db = new Sdb(db);' + "\n" );
   file.close();
   commFile.close();
}

function getFile ( fileName )
{
   var file = new File( fileName );
   File.chmod( fileName, 0644, false );
   file.truncate();
   return file;
}

// JS_ADD_MEMBER_FUNC_WITHATTR
function addMemPrivateFunc ( className, memFunc )
{
   var privateFunc = {
      // __runCommand 前面就是两个下划线
      "Remote": ["__runCommand"],
      "System": ["_getInfo"],
      "IniFile": ["_setValue", "_getValue", "_setSectionComment", "_getSectionComment", "_setComment", "_getComment", "_setLastComment", "_getLastComment", "_enableItem", "_disableItem", "_disableAllItem", "_toString", "_toObj", "_save", "_getFileName", "_getFlags", "_convertComment", "_comment2String"],
      "Oma": ["_runCommand"],
      "File": ["_read", "_write", "_truncate", "_readLine", "_readContent", "_writeContent", "_close", "_seek", "_getInfo", "_toString"],
      "Cmd": ["_getLastRet", "_getLastOut", "_run", "_start", "_getCommand", "_getInfo"],
      "Filter": ["_match"]
   }
   for( var key in privateFunc )
   {
      if( key == className )
      {
         // 不知道为啥 concat 不行..
         var value = privateFunc[key];
         for( var i = 0; i < value.length; i++ )
         {
            memFunc.push( value[i] );
         }
      }
   }
   return memFunc;
}

// JS_ADD_STATIC_FUNC_WITHATTR
function addStaPrivateFunc ( className, staicFunc )
{
   var privateFunc = {
      "System": ["_listProcess", "_listLoginUsers", "_listAllUsers", "_listGroups", "_createSshKey", "_getHomePath"],
      "File": ["_getFileObj", "_readFile", "_getPathType", "_getUmask", "_list", "_find"]
   }
   for( var key in privateFunc )
   {
      if( key == className )
      {
         // 不知道为啥 concat 不行..
         var value = privateFunc[key];
         for( var i = 0; i < value.length; i++ )
         {
            staicFunc.push( value[i] );
         }
      }
   }
   return staicFunc;
}
