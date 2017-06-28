#pragma once
#include <include/cef_v8.h>
#include <include/cef_app.h>
#include <include/cef_client.h>

class CEFWebKit :
	public CefClient, 
	public CefLifeSpanHandler
{
protected:
	CefRefPtr<CefBrowser> m_Browser;
public:
	CEFWebKit() {};
	virtual ~CEFWebKit() {};
	CefRefPtr<CefBrowser> GetBrowser() { return m_Browser; }

	virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE
	{
		return this;
	}

	virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;

	// 添加CEF的SP虚函数
	IMPLEMENT_REFCOUNTING(CEFWebKit);
//	IMPLEMENT_LOCKING(CEFWebKit);
};

struct ClientV8ExtensionHandler : public CefV8Handler {
	ClientV8ExtensionHandler() {};
	ClientV8ExtensionHandler(CefRefPtr<CefApp> app);
	bool Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, CefRefPtr<CefV8Value> &retval, CefString &exception) OVERRIDE;

private:
	CefRefPtr<CefApp> app;

	IMPLEMENT_REFCOUNTING(ClientV8ExtensionHandler);
};

class ClientAppRenderer : public CefApp,
	public CefRenderProcessHandler
{
public:
	ClientAppRenderer();

	CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() OVERRIDE
	{
		return this;
	}

	void OnContextCreated(
		CefRefPtr<CefBrowser> browser,
		CefRefPtr<CefFrame> frame,
		CefRefPtr<CefV8Context> context);

	void OnWebKitInitialized() OVERRIDE;

private:
	CefRefPtr<ClientV8ExtensionHandler> m_v8Handler;

	IMPLEMENT_REFCOUNTING(ClientAppRenderer);
};
