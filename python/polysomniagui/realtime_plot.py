from PySide6 import QtWidgets, QtCore
import pyqtgraph as pg
import numpy as np
import time

class PlotWindow(QtWidgets.QWidget):
    def __init__(self, channels: int):
        super().__init__()
        self.channels = channels
        self.setWindowTitle("Polysomnografia - Streaming")
        layout = QtWidgets.QVBoxLayout(self)
        self.plot = pg.PlotWidget()
        layout.addWidget(self.plot)
        self.curves = []
        self.buffer_seconds = 10
        self.sample_rate = 1000
        self.max_samples = self.buffer_seconds * self.sample_rate
        self.data = np.zeros((self.max_samples, channels), dtype=np.int32)
        self.index = 0
        colors = [pg.intColor(i, hues=channels) for i in range(channels)]
        for c in range(channels):
            curve = self.plot.plot(pen=colors[c])
            self.curves.append(curve)
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self._refresh)
        self.timer.start(50)

    def push_frame(self, frame):
        samples = frame.data
        n = samples.shape[0]
        if self.index + n <= self.max_samples:
            self.data[self.index:self.index+n] = samples
            self.index += n
        else:
            shift = (self.index + n) - self.max_samples
            if shift < self.max_samples:
                self.data = np.roll(self.data, -shift, axis=0)
                self.index = self.max_samples - n
                self.data[self.index:self.index+n] = samples
                self.index += n
            else:
                self.data[:] = samples[-self.max_samples:]
                self.index = self.max_samples
        self.sample_rate = frame.sample_rate

    def _refresh(self):
        if self.index == 0:
            return
        window = self.data[:self.index]
        t = np.arange(window.shape[0]) / self.sample_rate
        for c, curve in enumerate(self.curves):
            curve.setData(t, window[:, c] + c*1_000_000)  # offset simples para separar canais
