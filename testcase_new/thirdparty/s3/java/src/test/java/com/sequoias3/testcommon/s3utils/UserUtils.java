package com.sequoias3.testcommon.s3utils;

import com.sequoias3.testcommon.S3TestBase;
import com.sequoias3.testcommon.TestRest;
import com.sequoias3.testcommon.s3utils.bean.UserCommDefind;
import org.json.JSONObject;
import org.json.XML;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.HttpClientErrorException;

public class UserUtils extends S3TestBase {
    public final static String accessKeyId = "ABCDEFGHIJKLMNOPQRST";
    public final static String accessKeys = "AccessKeys";
    public final static String accessKeyID = "AccessKeyID";
    public final static String secretAccessKey = "SecretAccessKey";

    public static String[] createUser( String name, String type )
            throws HttpClientErrorException {
        JSONObject userJson = createUser( name, type, accessKeyId );
        JSONObject subJson = userJson
                .getJSONObject( UserCommDefind.accessKeys );
        String accessKeyID = subJson.getString( UserCommDefind.accessKeyID );
        String secretAccessKey = subJson
                .getString( UserCommDefind.secretAccessKey );
        return new String[] { accessKeyID, secretAccessKey };
    }

    public static JSONObject createUser( String name, String type,
            String accessKeyId ) throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=CreateUser&UserName="
                        + name + "&Role=" + type )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( UserCommDefind.authorization,
                        UserCommDefind.authValPre + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        String body = response.getBody().toString();
        return XML.toJSONObject( body );
    }

    public static String deleteUser( String name )
            throws HttpClientErrorException {
        return deleteUser( name, accessKeyId, true );
    }

    public static String deleteUser( String name, String accessKeyId )
            throws HttpClientErrorException {
        return deleteUser( name, accessKeyId, true );
    }

    public static String deleteUser( String name, String accessKeyId,
            boolean force ) throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=DeleteUser&UserName=" + name
                        + "&Force=" + force )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( UserCommDefind.authorization,
                        UserCommDefind.authValPre + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        return response.getHeaders().toString();
    }

    public static JSONObject updateUser( String name, String accessKeyId )
            throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=CreateAccessKey&UserName=" + name )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( UserCommDefind.authorization,
                        UserCommDefind.authValPre + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        String body = response.getBody().toString();
        return XML.toJSONObject( body );
    }

    public static JSONObject getUser( String name, String accessKeyId )
            throws HttpClientErrorException {
        TestRest rest = new TestRest();
        ResponseEntity< ? > response = rest
                .setApi( "/users/?Action=GetAccessKey&UserName=" + name )
                .setRequestMethod( HttpMethod.POST )
                .setRequestHeaders( UserCommDefind.authorization,
                        UserCommDefind.authValPre + accessKeyId + "/" )
                .setResponseType( String.class ).exec();
        String body = response.getBody().toString();
        return XML.toJSONObject( body );
    }
}
