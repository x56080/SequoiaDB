/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = html-show.php

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
<?php

$model   = empty ( $_GET['m'] ) ? "all"   : $_GET['m'] ;
$mapping = empty ( $_GET['p'] ) ? "data" : $_GET['p'] ;

if( $mapping == 'index' )
{
   $mapping = 'data' ;
}

$smarty -> assign( "isConnect", $isConnect ) ;

if ( $model == "all" )
{
	$globalvar_tool_button_cn = array() ;
	$globalvar_tool_parameter_cn = array() ;
	if ( file_exists ( "./php/html_globalvar_".$mapping.".php" ) )
	{
		//获取数据
		require_once ( "./php/html_globalvar_".$mapping.".php" ) ;
	}
	$smarty -> assign( "globalvar_tool_button", $globalvar_tool_button_cn ) ;
	$smarty -> assign( "globalvar_tool_parameter", $globalvar_tool_parameter_cn ) ;


	$smarty -> assign( "headerlist", $mapping ) ;
	$smarty -> display ( 'header.html', 0 ) ;
	if ( file_exists ( './templates/tool/tool-'.$mapping.'.html' ) )
	{
		$smarty -> display ( 'tool/tool-'.$mapping.'.html', 3 ) ;
	}
	else
	{
		$smarty -> display ( 'tool.html', 3 ) ;
	}

	if ( file_exists ( './php/model/m-'.$mapping.'.php' ) )
	{
		require_once ( './php/model/m-'.$mapping.'.php' ) ;
	}

	if ( file_exists ( './templates/'.$mapping.'.html' ) )
	{
		$smarty -> display ( $mapping.'.html', 1 ) ;
	}

	$smarty -> display ( 'foot.html', 2 ) ;
	
}
else if ( $model == "ajax_r" )
{
	sleep( 1 ) ;
	if ( $isConnect )
	{
		if ( !$smarty -> isCached ( './ajax/a-'.$mapping.'_r.html', 1 ) )
		{
			$smarty -> clearCompiledTemplate() ;
		
			if ( file_exists ( './php/ajax/a-'.$mapping.'_r.php' ) )
			{
				require_once ( './php/ajax/a-'.$mapping.'_r.php' ) ;
			}
			
		}
		if ( file_exists ( './templates/ajax/a-'.$mapping.'_r.html' ) )
		{
			$smarty -> display ( './ajax/a-'.$mapping.'_r.html', 1 ) ;
		}
	}
}
else if ( $model == "ajax_w" )
{
	sleep( 1 ) ;
	if ( $isConnect )
	{
		if ( !$smarty -> isCached ( './ajax/a-'.$mapping.'_w.html', 1 ) )
		{
			$smarty -> clearCompiledTemplate() ;
		
			if ( file_exists ( './php/ajax/a-'.$mapping.'_w.php' ) )
			{
				require_once ( './php/ajax/a-'.$mapping.'_w.php' ) ;
			}
			
		}
		if ( file_exists ( './templates/ajax/a-'.$mapping.'_w.html' ) )
		{
			$smarty -> display ( './ajax/a-'.$mapping.'_w.html', 1 ) ;
		}
	}
}
else if ( $model == "ajax_l" )
{
	sleep( 1 ) ;
	if ( $isConnect )
	{
		if ( !$smarty -> isCached ( './ajax/a-'.$mapping.'_l.html', 1 ) )
		{
			$smarty -> clearCompiledTemplate() ;
		
			if ( file_exists ( './php/ajax/a-'.$mapping.'_l.php' ) )
			{
				require_once ( './php/ajax/a-'.$mapping.'_l.php' ) ;
			}
			
		}
		if ( file_exists ( './templates/ajax/a-'.$mapping.'_l.html' ) )
		{
			$smarty -> display ( './ajax/a-'.$mapping.'_l.html', 1 ) ;
		}
	}
}
else if ( $model == "ajax_f" )
{
	sleep( 1 ) ;
	if ( !$smarty -> isCached ( './ajax/a-'.$mapping.'_f.html', 1 ) )
	{
		$smarty -> clearCompiledTemplate() ;
	
		if ( file_exists ( './php/ajax/a-'.$mapping.'_f.php' ) )
		{
			require_once ( './php/ajax/a-'.$mapping.'_f.php' ) ;
		}
		
	}
	/*if ( file_exists ( './templates/ajax/a-'.$mapping.'_f.html' ) )
	{
		$smarty -> display ( './ajax/a-'.$mapping.'_f.html', 1 ) ;
	}*/
}
?>