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

using System;

/** \namespace SequoiaDB
 *  \brief SequoiaDB Driver for C#.Net
 *  \author Hetiu Lin
 */
namespace SequoiaDB
{
    /** \class BaseException
     *  \brief Database operation exception
     */
    public class BaseException : Exception
    {
        private const long serialVersionUID = -6115487863398926195L;

        private string message;
        private string errorType;
        private int errorCode;

        /// <summary>
        /// Expection throw by sequoiadb.
        /// </summary>
        /// <param name="errorCode">The error code return by engine</param>
        /// <param name="detail">The error Detail</param>
        internal BaseException(int errorCode, string detail)
        {
            try
            {
                if (detail != null && detail != "")
                {
                    this.message = SDBErrorLookup.GetErrorDescriptionByCode(errorCode) +
                        ", " + detail;
                }
                else
                {
                    this.message = SDBErrorLookup.GetErrorDescriptionByCode(errorCode);
                }
                this.errorType = SDBErrorLookup.GetErrorTypeByCode(errorCode);
                this.errorCode = errorCode;
            }
            catch (Exception)
            {
                this.message = SequoiadbConstants.UNKONWN_DESC;
                this.errorType = SequoiadbConstants.UNKNOWN_TYPE;
                this.errorCode = SequoiadbConstants.UNKNOWN_CODE;
            }
        }

        internal BaseException(int errorCode):this(errorCode, "")
        {
        }

        internal BaseException(string errorType)
        {
            try
            {
                this.message = SDBErrorLookup.GetErrorDescriptionByType(errorType);
                this.errorCode = SDBErrorLookup.GetErrorCodeByType(errorType);
                this.errorType = errorType;
            }
            catch (Exception)
            {
                this.message = SequoiadbConstants.UNKONWN_DESC;
                this.errorType = SequoiadbConstants.UNKNOWN_TYPE;
                this.errorCode = SequoiadbConstants.UNKNOWN_CODE;
            }
        }

        /** \property Message
         *  \brief Get the error description of exception
         */
        public override string Message
        {
            get
            {
                return this.message;
            }
        }

        /** \property ErrorType
         *  \brief Get the error type of exception
         */
        public string ErrorType
        {
            get 
            {
                return this.errorType;
            }
        }

        /** \property ErrorCode
         *  \brief Get the error code of exception
         */
        public int ErrorCode
        {
            get
            {
                return this.errorCode;
            }
        }
    }
}
