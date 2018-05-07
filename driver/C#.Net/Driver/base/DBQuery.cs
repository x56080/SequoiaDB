/*
 * Copyright 2018 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

using SequoiaDB.Bson;
using System.Collections.Generic;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class DBQuery
     *  \brief Database operation rules
     */
    public class DBQuery
   {
        private long skipRowsCount = 0;
        private long returnRowsCount = -1;
        private int flag = 0;

        /** \memberof FLG_QUERY_FORCE_HINT 0x00000080
         *  \brief Force to use specified hint to query,
         *         if database have no index assigned by the hint, fail to query
         */
	    public const int FLG_QUERY_FORCE_HINT = 0x00000080;

        /** \memberof FLG_QUERY_PARALLED 0x00000100
         *  \brief Enable parallel sub query, 
         *         each sub query will finish scanning diffent part of the data
         */
        public const int FLG_QUERY_PARALLED = 0x00000100;

        /** \memberof FLG_QUERY_WITH_RETURNDATA 0x00000200
         *  \brief In general, query won't return data until cursor gets from database, 
         *         when add this flag, return data in query response, it will be more high-performance
         */
        public const int FLG_QUERY_WITH_RETURNDATA = 0x00000200;
	
	    /** \memberof FLG_QUERY_EXPLAIN 0x00000400
	     *  \brief Query explain
	     */
        internal const int FLG_QUERY_EXPLAIN = 0x00000400;

        /** \memberof FLG_QUERY_MODIFY 0x00001000
	     *  \brief Query and modify
	     */
        internal const int FLG_QUERY_MODIFY = 0x00001000;

        internal static readonly Dictionary<int, int> flagsDir = new Dictionary<int, int>() {
            {FLG_QUERY_FORCE_HINT, FLG_QUERY_FORCE_HINT},
            {FLG_QUERY_PARALLED, FLG_QUERY_PARALLED},
            {FLG_QUERY_WITH_RETURNDATA, FLG_QUERY_WITH_RETURNDATA}
        };

       /** \property Matcher
        *  \brief Matching rule
        */
        public BsonDocument Matcher { get; set; }

        /** \property Selector
         *  \brief selective rule
         */
        public BsonDocument Selector { get; set; }

        /** \property OrderBy
         *  \brief Ordered rule
         */
        public BsonDocument OrderBy { get; set; }

        /** \property Hint
         *  \brief Sepecified access plan
         */
        public BsonDocument Hint { get; set; }

        /** \property Modifier
         *  \brief Modified rule
         */
        public BsonDocument Modifier { get; set; }

        /** \property SkipRowsCount
         *  \brief Documents to skip
         */
        public long SkipRowsCount
        {
            get { return skipRowsCount; }
            set { skipRowsCount = value; }
        }

        /** \property ReturnRowsCount
         *  \brief Documents to return
         */
        public long ReturnRowsCount
        {
            get { return returnRowsCount; }
            set { returnRowsCount = value; }
        }

        /** \property Flag
         *  \brief Query flag
         */
        public int Flag
        {
            get { return flag; }
            set { flag = value; }
        }

	    private static int _Regulate(int newFlags, int originalFlag) 
        {
		    int retFlags = newFlags;
            int tmpFlag = 0;
            if (flagsDir.ContainsKey(originalFlag))
            {
                tmpFlag = flagsDir[originalFlag];
                if (tmpFlag != originalFlag)
                {
                    retFlags &= ~originalFlag;
                    retFlags |= tmpFlag;
                }
            }
		    return retFlags;
	    }

        internal static int RegulateFlag(int flags)
        {
            int newFlags = flags;
            if ((flags & FLG_QUERY_FORCE_HINT) != 0)
            {
                newFlags = _Regulate(newFlags, FLG_QUERY_FORCE_HINT);
            }
            if ((flags & FLG_QUERY_PARALLED) != 0)
            {
                newFlags = _Regulate(newFlags, FLG_QUERY_PARALLED);
            }
            if ((flags & FLG_QUERY_WITH_RETURNDATA) != 0)
            {
                newFlags = _Regulate(newFlags, FLG_QUERY_WITH_RETURNDATA);
            }
            return newFlags;
        }

   }
}
