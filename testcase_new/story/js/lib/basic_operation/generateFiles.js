/*******************************************************************************
@description 生成该目录下除 assert.js 和 Error.js 文件之外的所有包装类（包括 commlib）
            1. 使用 showClassfull 得到所有内置对象和全局方法
            2. 使用 showClassfull("内置对象") 打印出内置对象所有的静态方法和成员方法
            3. 往文件中写声明和封装方法语句（装饰器模式），当内置类被封装，其内所有方法丢失，所以内置类最早封装
            4. 生成 commlib.js 并提供 Sdb 实例和 Assert 实例
            5. 当前方法内嵌进 all_prepare.js，调用 runtest.sh 会执行一次，也可通过在该文件同级目录直接使用 sdb -f generateFiles.js 生成文件
@author  lyy
@time  2020-09-25
*******************************************************************************/
if( typeof ( DIRPATH ) == "undefined" ) { DIRPATH = "./"; }

generateFiles();

function generateFiles ()
{
   // 分出内置类和全局方法
   // 内置类：str[0]
   // 全局方法：str[1];
   var str = showClassfull().split( ":\n" ).map( function( classAndFunc ) { return classAndFunc.split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim() } ); } ).slice( 1 );
   var commFile = getFile( "commlib.js" );
   commFile.write( 'import( "./Error.js" );' + "\n" );

   // 封装内置类
   var classes = str[0];
   for( var i = 0; i < classes.length; i++ )
   {
      // 获取静态方法和成员方法
      var staicFunc = showClassfull( classes[i] ).split( "static functions:\n" )[1].split( "'s member functions" )[0].split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim().slice( 0, -2 ) } );
      var memFunc = showClassfull( classes[i] ).split( "member functions:\n" )[1].split( "\n" ).slice( 0, -1 ).map( function( s ) { return s.trim().slice( 0, -2 ) } );

      var fileName = classes[i] + ".js";
      var file = getFile( fileName );

      /* e.g: 
          var tmpSdb = { 
             dropCS: Sdb.prototype.dropCS,
             createCS: Sdb.prototype.createCS
          };
      */
      var varStr = "var tmp" + classes[i] + " = {";
      for( var j = 0; j < memFunc.length; j++ )
      {
         varStr += "\n   " + memFunc[j] + ": " + classes[i] + ".prototype." + memFunc[j];
         if( j != memFunc.length - 1 ) { varStr += ","; } else { varStr += "\n};"; }
      }
      file.write( varStr + "\n" );

      /* e.g: 
          var funcSdb = Sdb
      */
      var varStr = "var func" + classes[i] + " = " + classes[i] + ";";
      file.write( varStr + "\n" );

      /* e.g: 
          var funcSdbhelp = Sdb.help;
      */
      for( var j = 0; j < staicFunc.length; j++ )
      {
         var varStr = "var func" + classes[i] + staicFunc[j] + " = " + classes[i] + "." + staicFunc[j] + ";";
         file.write( varStr + "\n" );
      }

      /* e.g:  
          Sdb=function(){try{return funcSdb.apply( this, arguments ); } catch( e ) {  throw new Error(e) } };
      */
      var evalStr = classes[i] + "=function(){try{return func" + classes[i] + ".apply( this, arguments ); } catch( e ) { throw new Error(e) } };";
      file.write( evalStr + "\n" );

      /* e.g:
         Sdb.help = function(){try{ return funcSdbhelp.apply( this, arguments ); } catch( e ) { throw new Error(e) } };
      */
      for( var j = 0; j < staicFunc.length; j++ )
      {
         var evalStr = classes[i] + "." + staicFunc[j] + " = function(){" +
            "try{ return func" + classes[i] + staicFunc[j] + ".apply( this, arguments ); } catch( e ) { throw new Error(e) } };";
         file.write( evalStr + "\n" );
      }

      /* e.g: 
        Sdb.prototype.close=function(){try{return tmpSdb.close.apply(this,arguments);}catch(e){ throw new Error(e);}};
      */
      for( var j = 0; j < memFunc.length; j++ )
      {
         var evalStr = classes[i] + ".prototype." + memFunc[j] + "=function(){try{return tmp" + classes[i] + "." + memFunc[j]
            + ".apply(this,arguments);}catch(e){throw new Error(e);}};";
         file.write( evalStr + "\n" );
      }

      commFile.write( 'import( "./' + fileName + '" );' + "\n" );
      file.close();
   }

   // 封装全局方法
   var file = getFile( "Global.js" );
   var funcs = str[1].filter( function( s ) { if( s.indexOf( "import" ) == -1 ) { return true; } } ).map( function( s ) { return s.trim().slice( 0, -2 ) } );

   /* e.g: 
       var tmpGlobal = {
          catPath: catPath,
          displayManual: displayManual,
       }
   */
   var varStr = "var tmpGlobal = {";
   for( var i = 0; i < funcs.length; i++ )
   {
      varStr += "\n   " + funcs[i] + ": " + funcs[i];
      if( i != funcs.length - 1 ) { varStr += ","; } else { varStr += "\n};"; }
   }
   file.write( varStr + "\n" );

   /* e.g:
     sleep=function(){try{return tmpGlobal.sleep.apply(this,arguments);}catch(e){ throw new Error(e)}};
   */
   for( var i = 0; i < funcs.length; i++ )
   {
      var evalStr = funcs[i] + "=function(){try{return tmpGlobal." + funcs[i] + ".apply(this,arguments);}"
         + "catch(e){ throw new Error(e)}};";
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
   var filePath = DIRPATH + fileName;
   println( "生成文件" + filePath );
   var file = new File( filePath );
   File.chmod( filePath, 0644, false );
   file.truncate();
   return file;
}
