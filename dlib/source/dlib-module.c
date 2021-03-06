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

#include <stdlib.h>
#include <string.h>

#include "dlib.h"
#include "dlib-private.h"

#ifndef WIN32
#include <dlfcn.h>
#else
#include <Windows.h>
#endif

int DLib_Module_IsLoaded( DLib_ModuleRef module )
{
    if( module == NULL )
    {
        return 0;
    }
    
    return module->loaded;
}

char * DLib_Module_GetName( DLib_ModuleRef module )
{
    if( module == NULL )
    {
        return NULL;
    }
    
    return module->name;
}

char * DLib_Module_GetPath( DLib_ModuleRef module )
{
    if( module == NULL )
    {
        return NULL;
    }
    
    return module->path;
}

DLib_ModuleType DLib_Module_GetType( DLib_ModuleRef module )
{
    if( module == NULL )
    {
        return DLib_ModuleType_Unknown;
    }
    
    return module->type;
}

DLib_SymbolRef DLib_Module_GetSymbol( DLib_ModuleRef module, const char * name )
{
    size_t                  i;
    void                  * address;
    struct __DLib_Symbol    symbol;
    DLib_SymbolRef          symbolRef;
    
    if( module == NULL )
    {
        return NULL;
    }
    
    for( i = 0; i < __DLib_Array_GetCount( &( module->symbols ) ); i++ )
    {
        symbolRef = ( DLib_SymbolRef )__DLib_Array_GetValueAtIndex( &( module->symbols ), i );
        
        if( symbolRef == NULL || DLib_Symbol_GetName( symbolRef ) == NULL )
        {
            continue;
        }
        
        if( strcmp( DLib_Symbol_GetName( symbolRef ), name ) == 0 )
        {
            return symbolRef;
        }
    }
    
    if( module->loaded == 0 )
    {
        __DLib_Module_Load( module );
    }
    
    if( module->loaded == 0 )
    {
        return NULL;
    }
    
    #ifndef WIN32
    address = dlsym( module->handle, name );
    #else
    address = GetProcAddress( ( HMODULE )( module->handle ), name );
    #endif

    if( address != NULL )
    {
        memset( &symbol, 0, sizeof( struct __DLib_Symbol ) );
        
        symbol.module   = module;
        symbol.address  = address;

        #ifndef WIN32
        symbol.name     = strdup( name );
        #else
        symbol.name     = _strdup( name );
        #endif
        
        __DLib_Array_InsertCopyOfValue( &( module->symbols ), &symbol );
        
        if( __DLib_Array_GetCount( &( module->symbols ) ) > 0 )
        {
            return ( DLib_SymbolRef )__DLib_Array_GetValueAtIndex( &( module->symbols ), __DLib_Array_GetCount( &( module->symbols ) ) - 1 );
        }
    }
    
    return NULL;
}

void __DLib_Module_Load( DLib_ModuleRef module )
{
    if( module == NULL || module->path == NULL )
    {
        return;
    }
    
    if( module->loaded == 1 )
    {
        return;
    }
    
    #ifndef WIN32
    module->handle = dlopen( module->path, RTLD_LOCAL );
    #else

    {
        wchar_t * ws;

        ws = __DLib_CreateWideStringFromString( module->path );

        if( ws != NULL )
        {
            module->handle = LoadLibrary( ws );
            
            free( ws );
        }
    }

    #endif

    if( module->handle != NULL )
    {
        module->loaded = 1;
    }
}
