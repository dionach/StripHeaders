#define _WINSOCKAPI_
#include <httpserv.h>
#include <comdef.h>

class StripHeadersModule : public CHttpModule
{
private:
		IAppHostAdminManager        * pMgr;
		IAppHostElement             * pParentElem;
		IAppHostElementCollection   * pElemColl;
		IAppHostElement             * pElem;
		IAppHostPropertyCollection  * pElemProps;
		IAppHostProperty            * pElemProp;

		HRESULT hr;
		BSTR    bstrConfigCommitPath;
		BSTR    bstrSectionName;
		BSTR    bstrPropertyName;
		DWORD   dwElementCount;
		VARIANT vtValue;
		VARIANT vtPropertyName;

	void cleanup() 
	{
			if ( pElemProp != NULL )
			{
				pElemProp->Release();
				pElemProp = NULL;
			}
			if ( pElemProps != NULL )
			{
				pElemProps->Release(); 
				pElemProps = NULL;
			}
			if ( pElem != NULL )
			{
				pElem->Release();
				pElem = NULL;
			}
			if ( pElemColl != NULL )
			{
				pElemColl->Release(); 
				pElemColl = NULL;
			}
			if ( pParentElem != NULL )
			{
				pParentElem->Release(); 
				pParentElem = NULL;
			}
			if ( pMgr != NULL )
			{
				pMgr->Release(); 
				pMgr = NULL;
			}

			SysFreeString( bstrConfigCommitPath );
			SysFreeString( bstrSectionName );
			SysFreeString( bstrPropertyName );

			CoUninitialize();
	}

public:
	REQUEST_NOTIFICATION_STATUS OnSendResponse( IN IHttpContext * pHttpContext, IN ISendResponseProvider * pProvider )
	{
			UNREFERENCED_PARAMETER( pProvider );

			pMgr        = NULL;
			pParentElem = NULL;
			pElemColl   = NULL;
			pElem       = NULL;
			pElemProps  = NULL;
			pElemProp   = NULL;

			hr                   = S_OK;
			bstrConfigCommitPath = SysAllocString( L"MACHINE/WEBROOT/APPHOST" );
			bstrSectionName      = SysAllocString( L"system.webServer/stripHeaders" );
			bstrPropertyName     = SysAllocString( L"name" );
			dwElementCount       = 0;
			vtPropertyName.vt            = VT_BSTR;
			vtPropertyName.bstrVal       = bstrPropertyName;

			// Initialize
			hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
			if ( FAILED( hr ) )
			{
				// Set the error status.
				pProvider->SetErrorStatus( hr );
				// cleanup
				cleanup();
				// End additional processing.
				return RQ_NOTIFICATION_FINISH_REQUEST;
			}

			// Create
			hr = CoCreateInstance( __uuidof( AppHostAdminManager ), NULL, 
					CLSCTX_INPROC_SERVER,
					__uuidof( IAppHostAdminManager ), (void**) &pMgr );
			if( FAILED( hr ) )
			{
				pProvider->SetErrorStatus( hr );
				cleanup();
				return RQ_NOTIFICATION_FINISH_REQUEST;
			}

			// Get the admin section
			hr = pMgr->GetAdminSection( bstrSectionName, bstrConfigCommitPath, &pParentElem );
			if ( FAILED( hr ) || ( &pParentElem == NULL ) )
			{
				pProvider->SetErrorStatus( hr );
				cleanup();
				return RQ_NOTIFICATION_FINISH_REQUEST;
			}


			// Get the site collection
			hr = pParentElem->get_Collection( &pElemColl );
			if ( FAILED ( hr ) || ( &pElemColl == NULL ) )
			{
				pProvider->SetErrorStatus( hr );
				cleanup();
				return RQ_NOTIFICATION_FINISH_REQUEST;
			}

			// Get the elements
			hr = pElemColl->get_Count( &dwElementCount );
			for ( USHORT i = 0; i < dwElementCount; i++ )
			{
				VARIANT vtItemIndex;
				vtItemIndex.vt = VT_I2;
				vtItemIndex.iVal = i;

				// Add a new section group
				hr = pElemColl->get_Item( vtItemIndex, &pElem );
				if ( FAILED( hr ) || ( &pElem == NULL ) )
				{
					pProvider->SetErrorStatus( hr );
					cleanup();
					return RQ_NOTIFICATION_FINISH_REQUEST;
				}

				// Get the child elements
				hr = pElem->get_Properties( &pElemProps );
				if ( FAILED( hr ) || ( &pElemProps == NULL ) )
				{
					pProvider->SetErrorStatus( hr );
					cleanup();
					return RQ_NOTIFICATION_FINISH_REQUEST;
				}

				hr = pElemProps->get_Item( vtPropertyName, &pElemProp );
				if ( FAILED( hr ) || ( pElemProp == NULL ) )
				{
					pProvider->SetErrorStatus( hr );
					cleanup();
					return RQ_NOTIFICATION_FINISH_REQUEST;
				}

				hr = pElemProp->get_Value( &vtValue );
				if ( FAILED( hr ) )
				{
					pProvider->SetErrorStatus( hr );
					cleanup();
					return RQ_NOTIFICATION_FINISH_REQUEST;
				}

				// Retrieve a pointer to the response.
				IHttpResponse * pHttpResponse = pHttpContext->GetResponse();

				// Test for an error.
				if ( pHttpResponse != NULL )
				{
					// convert bstr to string in order to delete header
					_bstr_t header( vtValue.bstrVal );

					// delete header
					hr = pHttpResponse->DeleteHeader( (char *)header );

					// Test for an error.
					if ( FAILED( hr ) )
					{
						// Set the error status.
						pProvider->SetErrorStatus( hr );
						// cleanup
						cleanup();
						// End additional processing.
						return RQ_NOTIFICATION_FINISH_REQUEST;
					}
				}

				// loop_cleanup
				if ( pElem != NULL )
				{
					pElem->Release(); 
					pElem = NULL;
				}
			}

			cleanup();
			// Return processing to the pipeline.
			return RQ_NOTIFICATION_CONTINUE;
		}
};

// Create the module's class factory.
class StripHeadersModuleFactory : public IHttpModuleFactory
{
public:
	HRESULT GetHttpModule( OUT CHttpModule ** ppModule, IN IModuleAllocator * pAllocator )
	{
			UNREFERENCED_PARAMETER( pAllocator );

			// Create a new instance.
			StripHeadersModule * pModule = new StripHeadersModule;

			// Test for an error.
			if ( !pModule )
			{
				// Return an error if the factory cannot create the instance.
				return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
			}
			else
			{
				// Return a pointer to the module.
				*ppModule = pModule;
				pModule = NULL;
				// Return a success status.
				return S_OK;
			}
		}

	void Terminate()
	{
		// Remove the class from memory.
		delete this;
	}
};

// Create the module's exported registration function.
HRESULT __stdcall RegisterModule( DWORD dwServerVersion, IHttpModuleRegistrationInfo * pModuleInfo, IHttpServer * pGlobalInfo )
{
	UNREFERENCED_PARAMETER( dwServerVersion );
	UNREFERENCED_PARAMETER( pGlobalInfo );

	// Set the request notifications and exit.
	return pModuleInfo->SetRequestNotifications( new StripHeadersModuleFactory, RQ_SEND_RESPONSE, 0 );
}