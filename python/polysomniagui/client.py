import socket, struct, threading, queue, time, logging
import numpy as np

log = logging.getLogger("polysomniagui.client")

FRAME_HEADER_STRUCT = struct.Struct('<QQIIII')  # 8+8+4+4+4+4 = 32? (Header real é 40 bytes) -> ajustado abaixo

HEADER_SIZE = 40
MAGIC = 0x0AD51256AD51256A

class Frame:
    __slots__ = ("timestamp_ns", "channels", "samples_per_channel", "sample_rate", "data")
    def __init__(self, timestamp_ns, channels, samples_per_channel, sample_rate, data):
        self.timestamp_ns = timestamp_ns
        self.channels = channels
        self.samples_per_channel = samples_per_channel
        self.sample_rate = sample_rate
        self.data = data  # shape (samples_per_channel, channels)

class AcquisitionClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self._sock = None
        self._thread = None
        self._q: queue.Queue[Frame] = queue.Queue(maxsize=64)
        self._running = False

    def connect(self):
        self._sock = socket.create_connection((self.host, self.port), timeout=5)
        self._running = True
        self._thread = threading.Thread(target=self._recv_loop, daemon=True)
        self._thread.start()
        log.info("Conectado ao daemon %s:%d", self.host, self.port)

    def frames(self):
        while True:
            yield self._q.get()

    def close(self):
        self._running = False
        if self._sock:
            try: self._sock.close()
            except OSError: pass

    def _read_exact(self, n: int) -> bytes:
        buf = bytearray()
        while len(buf) < n:
            chunk = self._sock.recv(n - len(buf))
            if not chunk:
                raise ConnectionError("Socket fechado")
            buf.extend(chunk)
        return bytes(buf)

    def _recv_loop(self):
        try:
            while self._running:
                header = self._read_exact(HEADER_SIZE)
                magic, ts_ns, channels, samples_per_channel, sample_rate_hz, reserved = struct.unpack('<QQIIII', header)
                if magic != MAGIC:
                    log.error("Magic errado: %x", magic)
                    return
                total_samples = channels * samples_per_channel
                payload = self._read_exact(total_samples * 4)
                data = np.frombuffer(payload, dtype='<i4').reshape(samples_per_channel, channels)
                frame = Frame(ts_ns, channels, samples_per_channel, sample_rate_hz, data)
                try:
                    self._q.put(frame, timeout=0.1)
                except queue.Full:
                    _ = self._q.get_nowait()
                    self._q.put(frame)
        except Exception as e:
            log.error("Loop recepção finalizado: %s", e)
            self.close()
