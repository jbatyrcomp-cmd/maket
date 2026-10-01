// In-memory queue (Vercel serverless konteyneri xotirasida saqlanadi)
let packetQueue = [];

export default async function handler(req, res) {
  // CORS sarlavhalarini o'rnatish (har qanday domen va qurilmadan kirishga ruxsat)
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type, Authorization');

  // Pre-flight OPTIONS so'rovi kelganda darhol 200 qaytarish
  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  // POST: Yangi paketni navbatga qo'shish (Foydalanuvchi smartfonidan)
  if (req.method === 'POST') {
    try {
      let body = req.body;
      if (typeof body === 'string') {
        try {
          body = JSON.parse(body);
        } catch (e) {
          // Oddiy matn yoki allaqachon parsed
        }
      }

      const { sender, target, color } = body || {};

      if (!sender || !target) {
        return res.status(400).json({
          success: false,
          error: 'Yuboruvchi (sender) va Qabul qiluvchi (target) maydonlari to\'ldirilishi shart.'
        });
      }

      // Yangi paket obyekti
      const newPacket = {
        id: 'PKT-' + Date.now().toString(36).toUpperCase() + '-' + Math.random().toString(36).substring(2, 6).toUpperCase(),
        sender: String(sender).trim(),
        target: String(target).trim(),
        color: color && /^#[0-9A-Fa-f]{6}$/.test(color) ? color : '#00ffcc',
        timestamp: new Date().toISOString(),
        timeStr: new Date().toLocaleTimeString('uz-UZ', { hour12: false })
      };

      // Navbatga qo'shish (oxiriga)
      packetQueue.push(newPacket);

      // Xotira to'lib ketishini oldini olish uchun maksimum 200 ta paket saqlanadi
      if (packetQueue.length > 200) {
        packetQueue.shift();
      }

      return res.status(201).json({
        success: true,
        message: 'Paket navbatga muvaffaqiyatli qo\'shildi',
        packet: newPacket,
        queueLength: packetQueue.length
      });
    } catch (err) {
      console.error('POST /api/packets error:', err);
      return res.status(500).json({
        success: false,
        error: 'Serverda xatolik yuz berdi: ' + err.message
      });
    }
  }

  // GET: Navbatdagi barcha paketlarni o'qish va navbatni tozalash (Admin Bridge paneli uchun)
  if (req.method === 'GET') {
    try {
      // Joriy navbatdagi barcha paketlarni nusxalash
      const packetsToDeliver = [...packetQueue];

      // Navbatni tozalash
      packetQueue = [];

      return res.status(200).json({
        success: true,
        count: packetsToDeliver.length,
        packets: packetsToDeliver,
        serverTime: new Date().toISOString()
      });
    } catch (err) {
      console.error('GET /api/packets error:', err);
      return res.status(500).json({
        success: false,
        error: 'Server xatoligi: ' + err.message
      });
    }
  }

  // Agar boshqa HTTP metod kelsa
  return res.status(405).json({
    success: false,
    error: 'Faqat GET va POST metodlari qo\'llab-quvvatlanadi'
  });
}
