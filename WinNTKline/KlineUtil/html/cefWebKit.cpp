#include "cefWebKit.h"

void CEFWebKit::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    m_Browser = browser;
}

ClientAppRenderer::ClientAppRenderer() : m_v8Handler(new ClientV8ExtensionHandler) {}

void ClientAppRenderer::OnContextCreated(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    CefRefPtr<CefV8Context> context)
{
    OutputDebugString("ClientAppRenderer::OnContextCreated, create window binding\r\n");

    // Retrieve the context's window object.
    CefRefPtr<CefV8Value> object = context->GetGlobal();


    // Create the "NativeLogin" function.
    CefRefPtr<CefV8Value> func = CefV8Value::CreateFunction("NativeLogin", m_v8Handler);

    // Add the "NativeLogin" function to the "window" object.
    object->SetValue("NativeLogin", func, V8_PROPERTY_ATTRIBUTE_NONE);
}

void ClientAppRenderer::OnWebKitInitialized()
{
    std::string app_code =
        "var app;"
        "if (!app)"
        "    app = {};"
        "(function() {"
        "    app.ChangeTextInJS = function(text) {"
        "        native function ChangeTextInJS();"
        "        return ChangeTextInJS(text);"
        "    };"
        "})();;";

    CefRegisterExtension("v8/app", app_code, m_v8Handler);
}

bool ClientV8ExtensionHandler::Execute(const CefString& name,
    CefRefPtr<CefV8Value> object,
    const CefV8ValueList& arguments,
    CefRefPtr<CefV8Value>& retval,
    CefString& exception)
{
    if (name == "ChangeTextInJS") {
        if ((arguments.size() == 1) && arguments[0]->IsString()) {
            CefString           text = arguments[0]->GetStringValue();
            CefRefPtr<CefFrame> frame = CefV8Context::GetCurrentContext()->GetBrowser()->GetMainFrame();
            std::string         jscall = "ChangeText('";
            jscall += text;
            jscall += "');";
            frame->ExecuteJavaScript(jscall, frame->GetURL(), 0);
            /*
            * If you want your method to return a value, just use retval, like this:
            * retval = CefV8Value::CreateString("Hello World!");
            * you can use any CefV8Value, what means you can return arrays, objects or whatever you can create with CefV8Value::Create* methods
            */
            return true;
        }
    }
    return false;
}

ClientV8ExtensionHandler::ClientV8ExtensionHandler(CefRefPtr<CefApp> app)
{
    this->app = app;
}
