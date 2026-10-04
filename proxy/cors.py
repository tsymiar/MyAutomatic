#!/usr/bin/env python3
"""
静态文件服务器 + API 反向代理。
- /api/* 请求转发到本地 8000 端口的 FastAPI 后端
- 其他路径返回 client/ 目录下的静态文件
- 所有响应均添加 CORS 头
"""
import http.server
import sys
import urllib.request
import json


API_BACKEND = "http://127.0.0.1:8000"


class ProxyHandler(http.server.SimpleHTTPRequestHandler):

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "*")
        self.send_header("Access-Control-Allow-Headers", "*")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path.startswith("/api/"):
            self._proxy("GET")
        else:
            super().do_GET()

    def do_POST(self):
        if self.path.startswith("/api/"):
            self._proxy("POST")
        else:
            self.send_error(405)

    def do_DELETE(self):
        if self.path.startswith("/api/"):
            self._proxy("DELETE")
        else:
            self.send_error(405)

    # ── API 代理 ──────────────────────────────────────────
    def _proxy(self, method: str):
        target_url = f"{API_BACKEND}{self.path}"
        if self.path.find("?") == -1:
            # 保留 query string
            pass

        content_len = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_len) if content_len > 0 else None

        try:
            req = urllib.request.Request(
                target_url,
                data=body,
                method=method,
                headers={
                    "Content-Type": self.headers.get("Content-Type", "application/json"),
                },
            )
            with urllib.request.urlopen(req, timeout=60) as resp:
                status = resp.status
                response_body = resp.read()
                resp_headers = dict(resp.headers)
        except urllib.error.HTTPError as e:
            status = e.code
            response_body = e.read()
            resp_headers = {}
        except Exception as e:
            self.send_error(502, f"Proxy error: {e}")
            return

        self.send_response(status)
        # 复制响应头
        ct = resp_headers.get("content-type") or resp_headers.get("Content-Type")
        if ct:
            self.send_header("Content-Type", ct)
        self.send_header("Content-Length", str(len(response_body)))
        self.end_headers()
        self.wfile.write(response_body)

    def log_message(self, fmt, *args):
        if "/api/" in str(args):
            print(f"[proxy] {args[0]}")
        else:
            super().log_message(fmt, *args)


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    host = sys.argv[2] if len(sys.argv) > 2 else "0.0.0.0"
    print(f"Serving at http://{host}:{port}  (API proxy → {API_BACKEND})")
    httpd = http.server.HTTPServer((host, port), ProxyHandler)
    httpd.serve_forever()
