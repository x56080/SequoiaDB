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

   Source File Name = pdComponents.cpp

   Descriptive Name = Problem Determination

   When/how to use: this program may be used on binary and text-formatted
   versions of PD component. This file contains functions for components

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "pdTrace.hpp"

// Whenever we add a new component, we have to modify
// 1) pdTrace.hpp ( the component list )
// 2) pdComponents.cpp ( component list string )
// 3) pdTrace.hpp ( _pdTraceComponentNum )
// 4) add a new directory with EXACT same component name
// 5) create <comp>Trace.hpp which will include <comp>Trace.h
// There will be <comp>Trace.h automatically created in include directory
// Note there are max 32 components can be added into list
const CHAR *_pdTraceComponentDir[] = {
   "auth",   // PD_TRACE_COMPONENT_AUTH
   "bps",    // PD_TRACE_COMPONENT_BPS
   "cat",    // PD_TRACE_COMPONENT_CAT
   "cls",    // PD_TRACE_COMPONENT_CLS
   "dps",    // PD_TRACE_COMPONENT_DPS
   "mig",    // PD_TRACE_COMPONENT_MIG
   "msg",    // PD_TRACE_COMPONENT_MSG
   "net",    // PD_TRACE_COMPONENT_NET
   "oss",    // PD_TRACE_COMPONENT_OSS
   "pd",     // PD_TRACE_COMPONENT_PD
   "rtn",    // PD_TRACE_COMPONENT_RTN
   "sql",    // PD_TRACE_COMPONENT_SQL
   "tools",  // PD_TRACE_COMPONENT_TOOL
   "bar",    // PD_TRACE_COMPONENT_BAR
   "client", // PD_TRACE_COMPONENT_CLIENT
   "coord",  // PD_TRACE_COMPONENT_COORD
   "dms",    // PD_TRACE_COMPONENT_DMS
   "ixm",    // PD_TRACE_COMPONENT_IXM
   "mon",    // PD_TRACE_COMPONENT_MON
   "mth",    // PD_TRACE_COMPONENT_MTH
   "opt",    // PD_TRACE_COMPONENT_OPT
   "pmd",    // PD_TRACE_COMPONENT_PMD
   "rest",   // PD_TRACE_COMPONENT_REST
   "spt",    // PD_TRACE_COMPONENT_SPT
   "util",   // PD_TRACE_COMPONENT_UTIL
   "aggr",   // PD_TRACE_COMPONENT_AGGR
   "spd",    // PD_TRACE_COMPONENT_SPD
   "qgm"     // PD_TRACE_COMPONENT_QGM
} ;

const CHAR *pdGetTraceComponent ( UINT32 id )
{
   if ( (INT32)id >= _pdTraceComponentNum )
   {
      return NULL ;
   }
   return _pdTraceComponentDir[id] ;
}
