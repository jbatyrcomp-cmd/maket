# Cisco Packet Tracer Physical Prototype & Web Controller

Interaktiv jismoniy **ESP32 maketi (Cisco Packet Tracer topologiyasi)** uchun **Vercel** bulutli platformasida ishlaydigan Full-Stack veb-ilova.

---

## 📌 Loyiha Arxitekturasi

```
[Foydalanuvchi Smartfoni] (Istalgan Internet: 4G/LTE/Wi-Fi)
            │
            ▼ POST /api/packets
[Vercel Serverless Queue] (Global navbat xotirasi)
            │
            ▲ GET /api/packets (Har 1 soniyada Polling)
[Admin Noutbuk / Telefon] (ESP32 Wi-Fi tarmog'iga ulangan)
            │
            ▼ HTTP GET /sendWan?target=...&color=...
[Jismoniy ESP32 Maketi] (192.168.4.1) ➔ WS2812B Optik Nur Animatsiyasi
```

1. **Klientlar (`/` yoki `index.html`)**: O'z smartfonlaridan ism, qabul qiluvchi qurilma (PC1, PC2, Server0) va rangni tanlab, "Optik Toladan Yuborish" tugmasini bosishadi.
2. **Serverless API (`/api/packets`)**: Vercel funksiyasi paketlarni qabul qiladi va global navbatga qo'shadi.
3. **Admin Bridge (`/admin` yoki `admin.html`)**: Maket yonida turgan admin ESP32 ning Wi-Fi tarmog'iga (`Cisco_Maket_AP`) ulanadi. Admin paneli Vercel navbatidagi yangi paketlarni o'qiydi va bir zumda lokal ESP32 mikrokontrolleriga (`http://192.168.4.1`) yo'naltiradi.
4. **Hardware Sozlamalari**: Admin sahifasidan ESP32 ning GPIO pini, lenta yorqinligi va barcha segment oraliqlarini (WAN, Router-Switch, PC1, PC2, Server) NVS xotirasiga to'g'ridan-to'g'ri o'qish va saqlash mumkin.

---

## 📂 Fayllar Strukturasi

```
├── api/
│   └── packets.js                   # Vercel Serverless Function (POST navbatga qo'shadi, GET navbatni o'qiydi/tozalaydi)
├── public/
│   ├── index.html                   # Foydalanuvchilar (klientlar) uchun mobil Dark Cyberpunk sahifa
│   └── admin.html                   # Admin paneli (Jonli Bridge + Cisco CLI terminal + Hardware Setup)
├── esp32/
│   └── cisco_esp32_firmware.ino     # ESP32 uchun to'liq tayyor Arduino sketch (Wi-Fi AP, WebServer, NVS)
├── vercel.json                      # Vercel marshrutlash (routing & rewrites) konfiguratsiyasi
└── README.md                        # Loyiha hujjatlari va yo'riqnoma
```

---

## 🚀 1. Vercel-ga Deploy Qilish (1 Daqiqada)

Ushbu loyiha hech qanday qo'shimcha `npm install` yoki murakkab build talab qilmaydi. To'g'ridan-to'g'ri `git push` orqali ishlaydi.

### Usul 1: GitHub orqali (Eng oson)
1. Ushbu papkani GitHub-dagi yangi repozitoriyga yuklang:
   ```bash
   git init
   git add .
   git commit -m "Initial Cisco Packet Tracer Web Controller"
   git branch -M main
   git remote add origin https://github.com/USERNAME/REPO_NAME.git
   git push -u origin main
   ```
2. [Vercel.com](https://vercel.com) ga kiring va **"Add New Project"** tugmasini bosing.
3. Repozitoriyingizni import qiling va **"Deploy"** tugmasini bosing.
4. Bir necha soniyada Vercel sizga `https://sizning-loyihangiz.vercel.app` domenini beradi!

### Usul 2: Vercel CLI orqali
```bash
npx vercel
```

---

## ⚡ 2. ESP32 Mikrokontrollerini Sozlash

1. **Arduino IDE** dasturida quyidagi kutubxonalarni o'rnating:
   - `Adafruit NeoPixel` (Library Manager orqali)
   - `Preferences` (ESP32 core tarkibida mavjud)
   - `WebServer` (ESP32 core tarkibida mavjud)
2. `esp32/cisco_esp32_firmware.ino` faylini oching.
3. ESP32 platangizni kompyuterga ulang va **Upload** tugmasini bosing.
4. ESP32 ishga tushgach, avtomatik ravishda quyidagi Wi-Fi tarmoqni ochadi:
   - **SSID:** `Cisco_Maket_AP`
   - **Parol:** `12345678`
   - **ESP32 IP manzili:** `192.168.4.1`

---

## 🎮 3. Amaliy Ishlatish Ketma-ketligi

1. **Admin Qadami:**
   - Maket yonidagi noutbuk yoki telefon orqali ESP32 ning Wi-Fi tarmog'iga (`Cisco_Maket_AP`) ulaning.
   - Brauzeringizda Vercel admin panelini oching: `https://sizning-loyihangiz.vercel.app/admin`
   - Sahifada **"Jonli Ko'prik (Bridge) Holati: FAOL"** yozuvi turadi. ESP32 manzilini tekshiring (`http://192.168.4.1`) va kerak bo'lsa **"Ping"** tugmasi orqali tekshiring.
   - Zarur bo'lsa o'ng paneldan **"Yuklash"** tugmasini bosib, LED pini va segment oraliqlarini sozlang.

2. **Foydalanuvchi Qadami:**
   - Foydalanuvchilar o'z telefonlarida (istalgan mobil internetdan) saytga kirishadi: `https://sizning-loyihangiz.vercel.app`
   - Ismini yozib, qabul qiluvchi qurilmani (PC1, PC2 yoki Server0) hamda nur rangini tanlab **"Optik Toladan Yuborish ➔"** tugmasini bosishadi.

3. **Maketdagi Natija:**
   - Admin sahifasi 1 soniya ichida yangi paketni Vercel serveridan oladi va ESP32 ga yetkazadi.
   - ESP32 maketidagi WS2812B LED lentasida optik nur WAN kabelidan boshlanib, Router va Switch orqali tanlangan kompyuterga qarab yuguradi hamda monitor LEDlari miltillab paket qabul qilinganini ko'rsatadi!

---

## 🛠️ ESP32 HTTP API Ma'lumotnomasi

| Endpoint | Metod | Tavsif |
|---|---|---|
| `/sendWan?target=PC1&color=%2300ffcc` | GET | Paketni WAN orqali uzatish va animatsiyani ishga tushirish |
| `/clear` | GET | Lentadagi barcha LEDlarni tozalash |
| `/ping` | GET | Aloqani tekshirish (`PONG` qaytaradi) |
| `/getConfig` | GET | ESP32 xotirasidagi joriy pin, yorqinlik va segmentlar JSON-i |
| `/saveConfig?pin=4&...` | GET | Yangi parametrlarni ESP32 NVS xotirasiga doimiy saqlash |
