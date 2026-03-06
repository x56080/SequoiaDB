package com.sequoiadb.sdbschedule.utils;

import io.restassured.RestAssured;
import io.restassured.response.Response;

import java.util.HashMap;
import java.util.Map;

public class HttpClient {

    private static Map< String, Object > emptyParams() {
        return new HashMap< String, Object >();
    }

    public static Response get( String baseUrl, String path ) {
        return RestAssured.given().baseUri( baseUrl ).when().get( path );
    }

    public static Response get( String baseUrl, String path,
            Map< String, Object > params ) {
        return RestAssured.given().baseUri( baseUrl )
                .queryParams( params == null ? emptyParams() : params ).when()
                .get( path );
    }

    public static Response post( String baseUrl, String path, Object body ) {
        return RestAssured.given().baseUri( baseUrl )
                .contentType( "application/json" ).body( body ).when()
                .post( path );
    }

    public static Response post( String baseUrl, String path,
            Map< String, Object > params ) {
        return RestAssured.given().baseUri( baseUrl )
                .queryParams( params == null ? emptyParams() : params ).when()
                .post( path );
    }

    public static Response put( String baseUrl, String path,
            Map< String, Object > params ) {
        return RestAssured.given().baseUri( baseUrl )
                .queryParams( params == null ? emptyParams() : params ).when()
                .put( path );
    }

    public static Response put( String baseUrl, String path, Object body ) {
        return RestAssured.given().baseUri( baseUrl )
                .contentType( "application/json" ).body( body ).when()
                .put( path );
    }

    public static Response delete( String baseUrl, String path ) {
        return RestAssured.given().baseUri( baseUrl ).when().delete( path );
    }
}
