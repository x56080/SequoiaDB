<?php

function getOSInfo()
{
   return (DIRECTORY_SEPARATOR == '\\') ? 'windows' : 'linux' ;
}

function removeDir( $path )
{
   if( is_dir( $path ) == false )
   {
      return false ;
   }
   $handle = @opendir( $path ) ;
   while( ( $file = @readdir( $handle ) ) != false )
   {
      if( $file != '.' && $file != '..' )
      {
         $newPath = getOSInfo() == 'windows' ? ( $path.'\\'.$file ) : ( $path.'/'.$file ) ;
         is_dir( $newPath ) ? removeDir( $newPath ) : @unlink( $newPath ) ;
      }
   }
   closedir( $handle ) ;
   return rmdir( $path ) ;
}

//格式化doxygen，因为ie必须没有dom和必须\r\n换行才能正常使用（嵌套的时候）
function formatApiDoc( $path )
{
   if( is_dir( $path ) == false )
   {
      return false ;
   }
   $handle = @opendir( $path ) ;
   while( ( $file = @readdir( $handle ) ) != false )
   {
      if( $file != '.' && $file != '..' )
      {
         $newPath = getOSInfo() == 'windows' ? ( $path.'\\'.$file ) : ( $path.'/'.$file ) ;
         if( is_dir( $newPath ) )
         {
            if( formatApiDoc( $newPath ) === false )
            {
               return false ;
            }
         }
         else
         {
            if( pathinfo( $file )['extension'] == 'html' || pathinfo( $file )['extension'] == 'htm' )
            {
               $contents = file_get_contents( $newPath ) ;
               if ( $contents === false )
               {
                  return false ;
               }
               $contents = str_replace( "\n", "\r\n", $contents ) ;
               file_put_contents( $newPath, $contents ) ;
            }
         }
      }
   }
   return true ;
}

function copyDir( $src, $dst )
{
   if( is_dir( $src ) == false )
   {
      return ;
   }
   $handle = @opendir( $src ) ;
   @mkdir( $dst, 0777, true ) ;
   chmod( $dst, 0777 ) ;
   while( ( $file = @readdir( $handle ) ) != false )
   {
      if( $file != '.' && $file != '..' )
      {
         if( is_dir( "$src/$file" ) )
         {
            copyDir( "$src/$file", "$dst/$file" ) ;
         }
         else
         {
            copy( "$src/$file", "$dst/$file" ) ;
            chmod( "$dst/$file", 0777 ) ;
         }
      }
   }
   closedir( $handle ) ;
}

function clearDir( $path )
{
   if( removeDir( $path ) )
   {
      $rc = mkdir( $path, 0777, true ) ;
      chmod( $path, 0777 ) ;
      return $rc ;
   }
   return false ;
}

function printLog( $errMsg, $type = "Error" )
{
   echo 'PHP '.$type.': '.$errMsg."\n" ;
}

function execCmd( $cmd, $isPrint = false )
{
   $output = array() ;
   $return_val = 0 ;
   exec( $cmd, $output, $return_val ) ;
   if( $return_val != 0 || $isPrint == true )
   {
      $errMsg = "" ;
      foreach( $output as $index => $line )
      {
         $errMsg = $errMsg."   ".$line."\n" ;
      }
      printLog( $errMsg, $return_val == 0 ? 'Event' : 'Error' ) ;
   }
   return $return_val ;
}

function iterDoxygenConfig( $path )
{
   $list = array() ;
   $res = dir( $path ) ;
   while( $file = $res->read() )
   {
      if( $file != '.' && $file != '..' )
      {
         if( is_dir( getOSInfo() == 'linux' ? "$path/$file" : "$path\\$file" ) == false )
         {
            $parts = pathinfo( $file ) ;
            if( $parts['extension'] == 'conf' )
            {
               array_push( $list, "config/doxygen/$file" ) ;
            }
         }
      }
   }
   return $list ;
}
