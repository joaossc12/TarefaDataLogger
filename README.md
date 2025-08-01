﻿# TarefaDataLogger:
# PICO DATALOGGER INERCIAL

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Platform: Raspberry Pi Pico](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico-green.svg)
![Language: C/C++](https://img.shields.io/badge/Language-C/C++-orange.svg)

Um datalogger inercial completo e autônomo baseado na Raspberry Pi Pico. Este projeto captura dados de aceleração e giroscópio de um sensor MPU6050, os armazena em um cartão SD com timestamps precisos e fornece uma interface de usuário clara através de um display OLED, LED RGB e feedback sonoro.

## Funcionalidades Principais

* **📈 Coleta de Dados Inerciais:** Leitura de dados de 6 eixos (acelerômetro e giroscópio) do sensor MPU6050.
* **💾 Armazenamento em Cartão SD:** Salva os dados em formato `.csv` universal, utilizando a biblioteca FatFs para gerenciamento de arquivos.
* **🕒 Timestamp Preciso:** Utiliza o Relógio de Tempo Real (RTC) da Pico para adicionar data e hora a cada amostra de dados.
* **📟 Interface de Usuário Clara:** Um display OLED (SSD1306) exibe o status do sistema, contagem de amostras e mensagens de feedback em tempo real.
* **🚥 Status Visual e Sonoro:** Um LED RGB e um buzzer fornecem feedback imediato sobre o estado do sistema (Ocioso, Pronto, Gravando, Erro).
* **⚙️ Controle Simplificado:** Operação controlada por botões com funções dedicadas para montar o SD, iniciar/parar a gravação e entrar no modo de atualização de firmware.

---

## Mapeamento de Pinos (Pinout)

A tabela abaixo descreve a conexão dos componentes aos pinos GPIO da Raspberry Pi Pico.

| Pino (GP) | Função | Componente / Periférico |
| :--- | :--- | :--- |
| **GP0** | I2C0 SDA | Sensor MPU6050 |
| **GP1** | I2C0 SCL | Sensor MPU6050 |
| **GP5** | Entrada (Pull-up) | Botão A (Inicia/Para Gravação) |
| **GP6** | Entrada (Pull-up) | Botão B (Monta/Desmonta SD) |
| **GP11** | Saída | LED Verde |
| **GP12** | Saída | LED Azul |
| **GP13** | Saída | LED Vermelho |
| **GP14** | I2C1 SDA | Display OLED SSD1306 |
| **GP15** | I2C1 SCL | Display OLED SSD1306 |
| **GP21** | Saída PWM | Buzzer |
| **GP22** | Entrada (Pull-up) | Botão Joystick (Entra em modo BOOTSEL) |

**Nota sobre o Cartão SD:**
Os pinos para o leitor de cartão SD (SPI) não estão definidos neste arquivo principal. Eles devem ser configurados no arquivo `hw_config.h` (ou similar) da biblioteca do cartão SD. Uma configuração SPI típica para a Pico é:
* **GP16 (SPI0 TX)** -> MOSI
* **GP17 (SPI0 CS)** -> CS (Chip Select)
* **GP18 (SPI0 SCK)** -> SCK (Clock)
* **GP19 (SPI0 RX)** -> MISO

---

## Como Usar o Datalogger

Após gravar o firmware, a operação do dispositivo é simples e controlada pelos botões:

1.  **Ligar:** Conecte a alimentação. O aparelho tocará uma melodia de inicialização e o LED ficará **Amarelo** (Modo Ocioso).
2.  **Montar Cartão SD:** Pressione o **Botão B**. O aparelho emitirá um bip, o display confirmará a montagem e o LED ficará **Verde** (Modo Pronto).
3.  **Iniciar Gravação:** Com o LED verde, pressione o **Botão A**. O aparelho emitirá um bip, o display mostrará "Status: Capturando" e o LED ficará **Vermelho**. Os dados agora estão sendo salvos no arquivo `dados.csv`.
4.  **Parar Gravação:** Pressione o **Botão A** novamente. O LED voltará a ficar **Verde**.
5.  **Desmontar Cartão SD:** Pressione o **Botão B** para desmontar o cartão com segurança antes de removê-lo. O LED voltará a ficar **Amarelo**.

* **Feedback do LED:**
    * **Amarelo:** Ocioso.
    * **Verde:** Pronto para gravar.
    * **Red:** Gravando.
    * **Roxo:** Erro (ex: tentar gravar sem SD montado).
* **Atualização de Firmware:** Pressione o **Botão do Joystick** a qualquer momento para que a Pico reinicie em modo BOOTSEL, pronta para receber um novo arquivo `.uf2`.

---

## Como Compilar e Executar

### Pré-requisitos
- Ter o **SDK C/C++ para Raspberry Pi Pico** configurado em sua máquina. Siga o guia oficial [**"Getting started with Raspberry Pi Pico"**](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) se necessário.
- Clonar este repositório e suas bibliotecas de submódulo.

### Passos para Compilação
Execute os seguintes comandos no terminal a partir da pasta raiz do projeto:

```bash
# 1. Crie a pasta de build
mkdir build
cd build

# 2. Configure o projeto com CMake
# (Certifique-se de que a variável PICO_SDK_PATH está definida no seu sistema)
cmake ..

# 3. Compile o código para gerar o arquivo .uf2
make
