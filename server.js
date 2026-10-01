import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import handler from './api/packets.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const PORT = process.env.PORT || 3000;

const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.svg': 'image/svg+xml'
};

const server = http.createServer((req, res) => {
  const parsedUrl = new URL(req.url, `http://${req.headers.host}`);
  const pathname = parsedUrl.pathname;

  // 1. API: /api/packets
  if (pathname === '/api/packets') {
    // Collect body if POST
    if (req.method === 'POST') {
      let body = '';
      req.on('data', chunk => { body += chunk; });
      req.on('end', () => {
        try {
          req.body = body ? JSON.parse(body) : {};
        } catch {
          req.body = body;
        }

        // Mock express/vercel res methods if needed
        res.status = (code) => { res.statusCode = code; return res; };
        res.json = (data) => {
          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify(data));
          return res;
        };

        handler(req, res);
      });
      return;
    } else {
      res.status = (code) => { res.statusCode = code; return res; };
      res.json = (data) => {
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify(data));
        return res;
      };
      handler(req, res);
      return;
    }
  }

  // 2. Static routing
  let filePath = '';
  if (pathname === '/' || pathname === '/index.html') {
    filePath = path.join(__dirname, 'public', 'index.html');
  } else if (pathname === '/admin' || pathname === '/admin.html') {
    filePath = path.join(__dirname, 'public', 'admin.html');
  } else {
    filePath = path.join(__dirname, 'public', pathname);
  }

  fs.stat(filePath, (err, stats) => {
    if (err || !stats.isFile()) {
      res.statusCode = 404;
      res.setHeader('Content-Type', 'text/plain; charset=utf-8');
      res.end('404 Not Found');
      return;
    }

    const ext = path.extname(filePath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';
    res.setHeader('Content-Type', contentType);
    fs.createReadStream(filePath).pipe(res);
  });
});

server.listen(PORT, () => {
  console.log(`\n======================================================`);
  console.log(`⚡ Cisco Packet Tracer Prototype Server Ishga Tushdi!`);
  console.log(`🌐 Klient sahifasi:   http://localhost:${PORT}`);
  console.log(`🛠️  Admin paneli:      http://localhost:${PORT}/admin`);
  console.log(`======================================================\n`);
});
