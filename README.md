# Projeto de Aquisição de Polissonografia

Aquisição de sinais multi‑canal (ADC ADS1256 em Raspberry Pi) a ~1000 Hz por canal usando núcleo em C++ (daemon) e interface interativa em Python (PyQtGraph).

## Visão Geral da Arquitetura

Componentes:

1. `adc_daemon` (C++):
   - Inicializa driver ADS1256 (ou simulador se hardware ausente)
   - Faz amostragem contínua e agrega lotes (batches) de N amostras por canal
   - Publica frames binários via TCP para clientes (UI) em uma porta configurável
2. Protocolo Binário:
   - Cabeçalho fixo + payload (int32 little-endian) das amostras (24 bits expandidos)
3. UI Python:
   - Conecta ao daemon
   - Decodifica frames em thread assíncrona
   - Mantém buffer rolling e plota em tempo real (PyQtGraph)

## Estrutura

```
cpp/
  CMakeLists.txt
  include/
    ads1256_driver.h
    ring_buffer.h
    frame_protocol.h
  src/
    ads1256_driver.cpp
    data_server.cpp
    main.cpp
python/
  pyproject.toml
  polysomniagui/
    __init__.py
    client.py
    realtime_plot.py
    main.py
```

## Build (Raspberry Pi / Linux)

Pré-requisitos (Pi):

```
sudo apt update
sudo apt install -y build-essential cmake git
```

Compilar:

```
mkdir -p build
cd build
cmake ../cpp -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

Executar daemon (exemplo 8 canais @1000 Hz, lote 50 amostras):

```
./adc_daemon --channels 8 --rate 1000 --batch 50 --port 5555
```

UI Python (em outro terminal):

```
cd python
pip install -e .
python -m polysomniagui.main --host 127.0.0.1 --port 5555
```

## Protocolo de Frame

| Campo | Tipo | Descrição |
|-------|------|-----------|
| magic | uint64 (0xAD51256AD51256A) | Identificador |
| timestamp_ns | uint64 | Epoch (CLOCK_REALTIME) do primeiro sample do frame |
| channels | uint32 | Número de canais |
| samples_per_channel | uint32 | Amostras por canal neste frame |
| sample_rate_hz | uint32 | Taxa nominal de amostragem |
| reserved | uint32 | Reservado (0) |
| payload | int32[channels * samples_per_channel] | Amostras (24b sign extend) |

Endian: little-endian.

## Próximos Passos

1. Implementar driver real ADS1256 usando SPI (`/dev/spidev*`).
2. Adicionar calibração, ganho programável e filtros.
3. Implementar compressão leve (opcional) e/ou UDP multicast para múltiplos clientes.
4. Criar testes unitários (GoogleTest) e benchmarks.
5. Adicionar pybind11 para acesso direto ao ring buffer (modo in-process).

## Licença

Definir licença conforme necessidade (ex: MIT).
