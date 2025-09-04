import argparse, logging, threading
from .client import AcquisitionClient
from .realtime_plot import PlotWindow
from PySide6 import QtWidgets

def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=5555)
    parser.add_argument('--expected-channels', type=int, default=8)
    args = parser.parse_args(argv)
    logging.basicConfig(level=logging.INFO)

    app = QtWidgets.QApplication([])
    client = AcquisitionClient(args.host, args.port)
    client.connect()

    window = PlotWindow(args.expected_channels)
    window.show()

    def consumer():
        for frame in client.frames():
            # Ajusta canais dinâmicos se necessário
            if frame.channels != window.channels:
                pass
            window.push_frame(frame)

    threading.Thread(target=consumer, daemon=True).start()
    app.exec()

if __name__ == '__main__':
    main()
