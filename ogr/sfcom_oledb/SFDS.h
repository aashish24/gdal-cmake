/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  CSFSource declaration.
 * Author:   Ken Shih, kshih@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Les Technologies SoftMap Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************
 *
 * $Log$
 * Revision 1.6  1999/07/20 17:11:11  kshih
 * Use OGR code
 *
 * Revision 1.5  1999/06/22 16:17:31  warmerda
 * added debug statement
 *
 * Revision 1.4  1999/06/22 15:52:56  kshih
 * Added Initialize error return for invalid data set.
 *
 * Revision 1.3  1999/06/12 17:15:42  kshih
 * Make use of datasource property
 * Add schema rowsets
 *
 * Revision 1.2  1999/06/04 15:18:32  warmerda
 * Added copyright header.
 *
 */

#ifndef __CSFSource_H_
#define __CSFSource_H_
#include "resource.h"       // main symbols
#include "SFRS.h"
#include "sfutil.h"

// IDBInitializeImpl
template <class T>
class ATL_NO_VTABLE MyIDBInitializeImpl : public IDBInitializeImpl<T>
{
public:
	
	STDMETHOD(Initialize)(void)
	{
		HRESULT hr;
		hr =IDBInitializeImpl<T>::Initialize();
	
		if (SUCCEEDED(hr))
		{
			char *pszDataSource;
			IUnknown *pIU;
			QueryInterface(IID_IUnknown, (void **) &pIU);		
			
			RegisterOGRShape();

			pszDataSource = SFGetInitDataSource(pIU);

			OGRDataSource *poDS;
			poDS = OGRSFDriverRegistrar::Open( pszDataSource, FALSE );


			if (poDS)
			{
				hr = S_OK;
				QueryInterface(IID_IUnknown, (void **) &pIU);
				SFSetOGRDataSource(pIU,poDS);
			}
			else
			{
				hr = SFReportError(E_FAIL,IID_IDBInitialize,0,pszDataSource);
			}
			
			free(pszDataSource);
		}
	
		return hr;
		
	}
};


class ATL_NO_VTABLE CDataSourceISupportErrorInfoImpl : public ISupportErrorInfo
{
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		if (IID_IDBInitialize == riid)
			return S_OK;

		return S_FALSE;
	}
};
/////////////////////////////////////////////////////////////////////////////
// CDataSource
class ATL_NO_VTABLE CSFSource : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSFSource, &CLSID_SF>,
	public IDBCreateSessionImpl<CSFSource, CSFSession>,
	public MyIDBInitializeImpl<CSFSource>,
	public IDBPropertiesImpl<CSFSource>,
	public IPersistImpl<CSFSource>,
	public IInternalConnectionImpl<CSFSource>,
	public CDataSourceISupportErrorInfoImpl
	{
public:

	

	HRESULT FinalConstruct()
	{
		

		// verify the 
		return FInit();
	}
DECLARE_REGISTRY_RESOURCEID(IDR_SF)
BEGIN_PROPSET_MAP(CSFSource)
	BEGIN_PROPERTY_SET(DBPROPSET_DATASOURCEINFO)
		PROPERTY_INFO_ENTRY(ACTIVESESSIONS)
		PROPERTY_INFO_ENTRY(DATASOURCEREADONLY)
		PROPERTY_INFO_ENTRY(BYREFACCESSORS)
		PROPERTY_INFO_ENTRY(OUTPUTPARAMETERAVAILABILITY)
		PROPERTY_INFO_ENTRY(PROVIDEROLEDBVER)
		PROPERTY_INFO_ENTRY(DSOTHREADMODEL)
		PROPERTY_INFO_ENTRY(SUPPORTEDTXNISOLEVELS)
		PROPERTY_INFO_ENTRY(USERNAME)
	END_PROPERTY_SET(DBPROPSET_DATASOURCEINFO)
	BEGIN_PROPERTY_SET(DBPROPSET_DBINIT)
		PROPERTY_INFO_ENTRY(AUTH_PASSWORD)
		PROPERTY_INFO_ENTRY(AUTH_PERSIST_SENSITIVE_AUTHINFO)
		PROPERTY_INFO_ENTRY(AUTH_USERID)
		PROPERTY_INFO_ENTRY(INIT_DATASOURCE)
		PROPERTY_INFO_ENTRY(INIT_HWND)
		PROPERTY_INFO_ENTRY(INIT_LCID)
		PROPERTY_INFO_ENTRY(INIT_LOCATION)
		PROPERTY_INFO_ENTRY(INIT_MODE)
		PROPERTY_INFO_ENTRY(INIT_PROMPT)
		PROPERTY_INFO_ENTRY(INIT_PROVIDERSTRING)
		PROPERTY_INFO_ENTRY(INIT_TIMEOUT)
	END_PROPERTY_SET(DBPROPSET_DBINIT)
	CHAIN_PROPERTY_SET(CSFCommand)
END_PROPSET_MAP()
BEGIN_COM_MAP(CSFSource)
	COM_INTERFACE_ENTRY(IDBCreateSession)
	COM_INTERFACE_ENTRY(IDBInitialize)
	COM_INTERFACE_ENTRY(IDBProperties)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IInternalConnection)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
public:




};
#endif //__CSFSource_H_
