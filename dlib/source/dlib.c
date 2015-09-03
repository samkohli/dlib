/*******************************************************************************
 * Copyright (c) 2015, Jean-David Gadina - www.xs-labs.com
 * Distributed under the Boost Software License, Version 1.0.
 * 
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "dlib.h"
#include "dlib-private.h"

#ifdef WIN32
#include <Windows.h>
#endif

__DLib_ModuleArray __modules    = { NULL, 0, 0, 0 };
__DLib_StringArray __paths      = { NULL, 0, 0, 0 };

void DLib_AddSearchPath( const char * path )
{
    char * newPath;
    
    if( path == NULL )
    {
        return;
    }
    
    if( __paths.data == NULL )
    {
        __DLib_Array_Init( &__paths, sizeof( char * ) );
    }
    
    #ifndef WIN32
    newPath = strdup( path );
    #else
    newPath = _strdup( path );
    #endif
    
    if( __paths.data != NULL && newPath != NULL )
    {
        __DLib_Array_InsertCopyOfValue( &__paths, &newPath );

        #ifdef WIN32

        {
            wchar_t * ws;

            ws = __DLib_CreateWideStringFromString( newPath );

            if( ws != NULL )
            {
                /*
				 * AddDllDirectory doesn't exist on Windows XP
				 * AddDllDirectory( ws );
				 */ 
				SetDllDirectory( ws );
                free( ws );
            }
        }

        #endif
    }
    
    /*
     * Only used during Clang analysis part. Turns off the warning about
     * a memory leak, as the leak is deliberate.
     */
    #ifdef __clang_analyzer__
    free( newPath );
    #endif
}

DLib_ModuleRef DLib_GetModule( const char * name )
{
    size_t                  i;
    size_t                  n;
    DLib_ModuleRef          moduleRef;
    struct __DLib_Module    module;
    char                 ** path;
    FILE                  * fh;
    char                  * lib;
    
    if( __modules.data == NULL )
    {
        __DLib_Array_Init( &__modules, sizeof( struct __DLib_Module ) );
    }
    
    for( i = 0; i < __DLib_Array_GetCount( &__modules ); i++ )
    {
        moduleRef = ( DLib_ModuleRef )__DLib_Array_GetValueAtIndex( &__modules, i );
        
        if( DLib_Module_GetName( moduleRef ) == NULL )
        {
            continue;
        }
        
        if( strcmp( DLib_Module_GetName( moduleRef ), name ) == 0 )
        {
            return moduleRef;
        }
    }
    
    for( i = 0; i < __DLib_Array_GetCount( &__paths ); i++ )
    {
        path = __DLib_Array_GetValueAtIndex( &__paths, i );
        
        if( path == NULL )
        {
            continue;
        }
        
        n   = strlen( *( path ) ) + ( strlen( name ) * 2 ) + 20;
        lib = ( char * )calloc( 1, n );
        
        if( lib == NULL )
        {
            break;
        }
        
        memset( &module, 0, sizeof( struct __DLib_Module ) );
        
        #ifdef WIN32

        strcpy_s( lib, n, *( path ) );
        strcat_s( lib, n, "/" );
        strcat_s( lib, n, name );
        strcat_s( lib, n, ".dll" );
        
        fh = NULL;

        if( fopen_s( &fh, lib, "r" ) == 0 && fh != NULL )
        {
            fclose( fh );
            
            module.type = DLib_ModuleType_DLL;
            module.name = _strdup( name );
            module.path = _strdup( lib );
        }

        #else
        
        strlcpy( lib, *( path ), n );
        strlcat( lib, "/", n );
        strlcat( lib, name, n );
        strlcat( lib, ".dylib", n );
        
        if( ( fh = fopen( lib, "r" ) ) )
        {
            fclose( fh );
            
            module.type = DLib_ModuleType_DyLib;
            module.name = strdup( name );
            module.path = strdup( lib );
        }
        
        memset( lib, 0, n );
        strlcpy( lib, *( path ), n );
        strlcat( lib, "/", n );
        strlcat( lib, name, n );
        strlcat( lib, ".framework", n );
        strlcat( lib, "/", n );
        strlcat( lib, name, n );
        
        if( ( fh = fopen( lib, "r" ) ) )
        {
            fclose( fh );
            
            module.type = DLib_ModuleType_Framework;
            module.name = strdup( name );
            module.path = strdup( lib );
        }

        #endif
        
        if( module.type != DLib_ModuleType_Unknown )
        {
            __DLib_Array_Init( &( module.symbols ), sizeof( struct __DLib_Symbol ) );
            __DLib_Array_InsertCopyOfValue( &__modules, &module );
            
            if( __DLib_Array_GetCount( &__modules ) > 0 )
            {
                return ( DLib_ModuleRef )__DLib_Array_GetValueAtIndex( &__modules, __DLib_Array_GetCount( &__modules ) - 1 );
            }
            
            return NULL;
        }
        
        free( lib );
    }
    
    return NULL;
}

#ifdef WIN32

#include <Windows.h>

wchar_t * __DLib_CreateWideStringFromString( char * s )
{
    wchar_t * ws;
    int       n;
    
    n  = MultiByteToWideChar( CP_UTF8, 0, s, -1, NULL, 0 );
    ws = ( wchar_t * )malloc( ( ( size_t )n * sizeof( wchar_t ) ) + sizeof( wchar_t ) );
    
    if( ws == NULL )
    {
        return NULL;
    }

    MultiByteToWideChar( CP_UTF8, 0, s, -1, ws, n );

    return ws;
}

#endif

void DLib_PrintDebugInfo( void )
{
    char          ** str;
    DLib_ModuleRef   mod;
    DLib_SymbolRef   sym;
    size_t           i;
    size_t           j;
    
    printf( "DLib - Debug Info\n" );
    printf( "-----------------\n" );
    printf( "\n" );
    printf( "Search paths:  " );
    
    if( __DLib_Array_GetCount( &__paths ) == 0 )
    {
        printf( "None\n" );
    }
    else
    {
        for( i = 0; i < __DLib_Array_GetCount( &__paths ); i++ )
        {
            str = __DLib_Array_GetValueAtIndex( &__paths, i );
            
            if( str == NULL )
            {
                continue;
            }
            
            if( i > 0 )
            {
                printf( "               " );
            }
            
            printf( "%s\n", *( str ) );
        }
    }
    
    printf( "\n" );
    printf( "Modules:       " );
    
    if( __DLib_Array_GetCount( &__modules ) == 0 )
    {
        printf( "None\n" );
    }
    else
    {
        for( i = 0; i < __DLib_Array_GetCount( &__modules ); i++ )
        {
            mod = ( DLib_ModuleRef )__DLib_Array_GetValueAtIndex( &__modules, i );
            
            if( mod == NULL )
            {
                continue;
            }
            
            if( i > 0 )
            {
                printf( "               " );
            }
            
            printf( "%s", DLib_Module_GetName( mod ) );
            
            if( DLib_Module_IsLoaded( mod ) )
            {
                printf( " (loaded: %s)\n", DLib_Module_GetPath( mod ) );
            }
            else
            {
                printf( " (not loaded)\n" );
            }
            
            for( j = 0; j < __DLib_Array_GetCount( &( mod->symbols ) ); j++ )
            {
                sym = ( DLib_SymbolRef )__DLib_Array_GetValueAtIndex( &( mod->symbols ), j );
                
                if( sym == NULL )
                {
                    continue;
                }
                
                printf( "                   - %s: 0x%p\n", DLib_Symbol_GetName( sym ), DLib_Symbol_GetAddress( sym ) );
            }
            
            printf( "\n" );
        }
    }
}
