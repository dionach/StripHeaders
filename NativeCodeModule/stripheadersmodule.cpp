#define _WINSOCKAPI_
#include <httpserv.h>
#include <comdef.h>
#include <string>
#include <vector>

std::vector <std::string> vHeaders;

class StripHeadersModule : public CHttpModule
{

public:
	REQUEST_NOTIFICATION_STATUS OnSendResponse(IN IHttpContext * pHttpContext, IN ISendResponseProvider * pProvider)
	{
		UNREFERENCED_PARAMETER(pProvider);
		HRESULT hr = S_OK;

		// Retrieve a pointer to the response.
		IHttpResponse * pHttpResponse = pHttpContext->GetResponse();

		// Test for an error.
		if (pHttpResponse != NULL)
		{
			for (int i = 0; i != vHeaders.size(); i++)
			{
				// delete header
				hr = pHttpResponse->DeleteHeader(vHeaders[i].c_str());

				// Test for an error.
				if (FAILED(hr))
				{
					// Set the error status.
					pProvider->SetErrorStatus(hr);
					// End additional processing.
					return RQ_NOTIFICATION_FINISH_REQUEST;
				}
			}
		}

		// Return processing to the pipeline.
		return RQ_NOTIFICATION_CONTINUE;
	}
};

// Create the module's class factory.
class StripHeadersModuleFactory : public IHttpModuleFactory
{
public:
	HRESULT GetHttpModule(OUT CHttpModule ** ppModule, IN IModuleAllocator * pAllocator)
	{
		UNREFERENCED_PARAMETER(pAllocator);

		// Create a new instance.
		StripHeadersModule * pModule = new StripHeadersModule;

		// Test for an error.
		if (!pModule)
		{
			// Return an error if the factory cannot create the instance.
			return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
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
HRESULT __stdcall RegisterModule(DWORD dwServerVersion, IHttpModuleRegistrationInfo * pModuleInfo, IHttpServer * pGlobalInfo)
{
	UNREFERENCED_PARAMETER(dwServerVersion);
	IAppHostAdminManager        * pMgr = NULL;
	IAppHostElement             * pStripHeadersElement = NULL;
	IAppHostElementCollection   * pHeaderElements = NULL;
	DWORD						  dwHeaderElementsCount = 0;
	IAppHostElement             * pHeaderElement = NULL;
	IAppHostPropertyCollection  * pHeaderElementProperties = NULL;
	IAppHostProperty            * pHeaderElementNameProperty = NULL;
	VARIANT vtHeaderNameValue;
	VARIANT vtHeaderNameProperty;
	vtHeaderNameProperty.vt = VT_BSTR;
	vtHeaderNameProperty.bstrVal = L"name";

	HRESULT hr = S_OK;

	// Get the configuration object
	pMgr = pGlobalInfo->GetAdminManager();
	if (pMgr == NULL)
	{
		return hr;
	}

	// Get the strip headers section of the configuration
	hr = pMgr->GetAdminSection(L"system.webServer/stripHeaders", L"MACHINE/WEBROOT/APPHOST", &pStripHeadersElement);
	if (FAILED(hr) || (&pStripHeadersElement == NULL))
	{
		return hr;
	}

	// Get the strip headers element collection
	hr = pStripHeadersElement->get_Collection(&pHeaderElements);
	if (FAILED(hr))
	{
		return hr;
	}

	// Get the count of elements
	hr = pHeaderElements->get_Count(&dwHeaderElementsCount);
	if (FAILED(hr) || (&dwHeaderElementsCount == NULL))
	{
		return hr;
	}

	// Loop over header elements
	for (DWORD i = 0; i < dwHeaderElementsCount; i++)
	{
		VARIANT vtLoopIndex;
		vtLoopIndex.vt = VT_I2;
		vtLoopIndex.iVal = i;

		// Get the current header element
		hr = pHeaderElements->get_Item(vtLoopIndex, &pHeaderElement);
		if (FAILED(hr) || (&pHeaderElement == NULL))
		{
			return hr;
		}

		// Get the child elements
		hr = pHeaderElement->get_Properties(&pHeaderElementProperties);
		if (FAILED(hr) || (&pHeaderElementProperties == NULL))
		{
			return hr;
		}

		hr = pHeaderElementProperties->get_Item(vtHeaderNameProperty, &pHeaderElementNameProperty);
		if (FAILED(hr) || (pHeaderElementNameProperty == NULL))
		{
			return hr;
		}

		hr = pHeaderElementNameProperty->get_Value(&vtHeaderNameValue);
		if (FAILED(hr))
		{
			return hr;
		}

		// Add header name to headers vector
		_bstr_t bstrHeader(vtHeaderNameValue.bstrVal);
		vHeaders.push_back((std::string)bstrHeader);

		// loop_cleanup
		if (pHeaderElementNameProperty != NULL)
		{
			pHeaderElementNameProperty->Release();
			pHeaderElementNameProperty = NULL;
		}
		if (pHeaderElementProperties != NULL)
		{
			pHeaderElementProperties->Release();
			pHeaderElementProperties = NULL;
		}
		if (pHeaderElement != NULL)
		{
			pHeaderElement->Release();
			pHeaderElement = NULL;
		}
	}

	// cleanup
	if (pHeaderElements != NULL)
	{
		pHeaderElements->Release();
		pHeaderElements = NULL;
	}
	if (pStripHeadersElement != NULL)
	{
		pStripHeadersElement->Release();
		pStripHeadersElement = NULL;
	}
	if (pMgr != NULL)
	{
		pMgr->Release();
		pMgr = NULL;
	}

	// Set the request notifications
	hr = pModuleInfo->SetRequestNotifications(new StripHeadersModuleFactory, RQ_SEND_RESPONSE, 0);
	if (FAILED(hr))
	{
		return hr;
	}

	// Set the request priority.
	// Note: The priority levels are inverted for RQ_SEND_RESPONSE notifications.
	hr = pModuleInfo->SetPriorityForRequestNotification(RQ_SEND_RESPONSE, PRIORITY_ALIAS_FIRST);
	if (FAILED(hr))
	{
		return hr;
	}

	return S_OK;
}