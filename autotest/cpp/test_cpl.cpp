///////////////////////////////////////////////////////////////////////////////
// $Id$
//
// Project:  C++ Test Suite for GDAL/OGR
// Purpose:  Test general CPL features.
// Author:   Mateusz Loskot <mateusz@loskot.net>
// 
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2006, Mateusz Loskot <mateusz@loskot.net>
//  
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
// 
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.
///////////////////////////////////////////////////////////////////////////////
//
//  $Log$
//
///////////////////////////////////////////////////////////////////////////////
#include <tut.h>
#include <string>
#include "cpl_list.h"

namespace tut
{

    // Common fixture with test data
    struct test_cpl_data
    {
        test_cpl_data()
        {
        }
    };

    // Register test group
    typedef test_group<test_cpl_data> group;
    typedef group::object object;
    group test_cpl_group("CPL");

    // Test cpl_list API
    template<>
    template<>
    void object::test<1>()
    {
        CPLList* list;
    
        list = CPLListInsert(NULL, (void*)0, 0);
        ensure(CPLListCount(list) == 1);
        list = CPLListRemove(list, 2);
        ensure(CPLListCount(list) == 1);
        list = CPLListRemove(list, 1);
        ensure(CPLListCount(list) == 1);
        list = CPLListRemove(list, 0);
        ensure(CPLListCount(list) == 0);
        list = NULL;
        
        list = CPLListInsert(NULL, (void*)0, 2);
        ensure(CPLListCount(list) == 3);
        list = CPLListRemove(list, 2);
        ensure(CPLListCount(list) == 2);
        list = CPLListRemove(list, 1);
        ensure(CPLListCount(list) == 1);
        list = CPLListRemove(list, 0);
        ensure(CPLListCount(list) == 0);
        list = NULL;
    
        list = CPLListAppend(list, (void*)1);
        ensure(CPLListGet(list,0) == list);
        ensure(CPLListGet(list,1) == NULL);
        list = CPLListAppend(list, (void*)2);
        list = CPLListInsert(list, (void*)3, 2);
        ensure(CPLListCount(list) == 3);
        CPLListDestroy(list);
        list = NULL;
    
        list = CPLListAppend(list, (void*)1);
        list = CPLListAppend(list, (void*)2);
        list = CPLListInsert(list, (void*)4, 3);
        CPLListGet(list,2)->pData = (void*)3;
        ensure(CPLListCount(list) == 4);
        ensure(CPLListGet(list,0)->pData == (void*)1);
        ensure(CPLListGet(list,1)->pData == (void*)2);
        ensure(CPLListGet(list,2)->pData == (void*)3);
        ensure(CPLListGet(list,3)->pData == (void*)4);
        CPLListDestroy(list);
        list = NULL;
    
        list = CPLListInsert(list, (void*)4, 1);
        CPLListGet(list,0)->pData = (void*)2;
        list = CPLListInsert(list, (void*)1, 0);
        list = CPLListInsert(list, (void*)3, 2);
        ensure(CPLListCount(list) == 4);
        ensure(CPLListGet(list,0)->pData == (void*)1);
        ensure(CPLListGet(list,1)->pData == (void*)2);
        ensure(CPLListGet(list,2)->pData == (void*)3);
        ensure(CPLListGet(list,3)->pData == (void*)4);
        list = CPLListRemove(list, 1);
        list = CPLListRemove(list, 1);
        list = CPLListRemove(list, 0);
        list = CPLListRemove(list, 0);
        ensure(list == NULL);
    }

} // namespace tut
