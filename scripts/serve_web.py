#!/usr/bin/env python3
"""serve_web.py - tiny static server for the rss-port web gallery.

Serves the web/ directory with the right MIME type for .wasm and with
COOP/COEP headers enabled. Most savers run from any static host, but serving
with these headers is the safe default (and required if a saver is ever built
with pthreads / SharedArrayBuffer).

    python3 scripts/serve_web.py [port]      # default 8000
then open http://localhost:8000/
"""
import http.server
import os
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "web")


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
        ".data": "application/octet-stream",
    }

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    os.chdir(ROOT)
    print(f"Serving {ROOT} at http://localhost:{port}/  (Ctrl-C to stop)")
    http.server.HTTPServer(("0.0.0.0", port), Handler).serve_forever()
