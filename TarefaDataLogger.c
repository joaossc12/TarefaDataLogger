
/**
 * =================================================================================
 * PICO DATALOGGER INERCIAL - VERSÃO FINAL ORGANIZADA
 *
 * Captura dados de um sensor MPU6050 e os salva em um cartão SD.
 * A interface com o usuário é feita através de um display OLED, um LED RGB,
 * botões e um buzzer para feedback sonoro.
 * =================================================================================
 */

// =================================================================================
// SEÇÃO DE INCLUDES
// =================================================================================

// Bibliotecas Padrão
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Bibliotecas do Raspberry Pi Pico SDK
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"

// Bibliotecas de Terceiros e Locais
#include "lib/mpu6050.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "ff.h"
#include "f_util.h"
#include "diskio.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"
#include "lib/sd_config.h"

// =================================================================================
// SEÇÃO DE DEFINES
// =================================================================================

// Mapeamento de Pinos (GPIO)
#define LED_RED       13
#define LED_BLUE      12
#define LED_GREEN     11
#define BUTTON_A      5
#define BUTTON_B      6
#define BUZZER        21
#define JOYSTICK_BT   22

// Configurações do Display e I2C
#define I2C_PORT_SENSOR   i2c0
#define I2C_SDA_SENSOR    0
#define I2C_SCL_SENSOR    1
#define I2C_PORT_DISP     i2c1
#define I2C_SDA_DISP      14
#define I2C_SCL_DISP      15
#define ENDERECO_DISP     0x3C
#define DISP_W            128
#define DISP_H            64

// Notas Musicais para o Buzzer
#define NOTE_C5   523
#define NOTE_E5   659
#define NOTE_G5   784
#define NOTE_C6   1047

// =================================================================================
// SEÇÃO DE VARIÁVEIS GLOBAIS E TIPOS
// =================================================================================

// Configurações do Sistema
char arquivo[10] = "dados.csv";
uint32_t N_amostras = 0;

// Flags de Estado (volatile para segurança em interrupções)
volatile bool flag_captura = false;
volatile bool flag_montagem = false;
volatile bool flag_estado_montagem = false;

// Flags de Eventos (para comunicação entre ISR e loop principal)
volatile bool play_confirmation_sound = false;
volatile bool play_error_sound = false;

// Controle da Interface do Usuário (Display)
ssd1306_t ssd;

// Enum para os diferentes tipos de mensagens temporárias no display
typedef enum {
    MSG_NONE,
    MSG_SD_MONTADO,
    MSG_SD_DESMONTADO,
    MSG_INICIANDO_GRAVACAO,
    MSG_PARANDO_GRAVACAO,
    MSG_ERRO_AO_GRAVAR
} temp_message_state_t;

volatile temp_message_state_t current_message_state = MSG_NONE;
absolute_time_t message_timestamp;

// =================================================================================
// SEÇÃO DE FUNÇÕES
// (Definidas em ordem de dependência para evitar a necessidade de protótipos)
// =================================================================================

// Aciona o LED RGB para exibir uma cor específica.
void att_led(uint state) {
    // 0 = desligado, 1 = amarelo, 2 = verde, 3 = vermelho, 4 = azul, 5 = roxo
    switch (state) {
        case 1: gpio_put(LED_RED, 1); gpio_put(LED_GREEN, 1); gpio_put(LED_BLUE, 0); break;
        case 2: gpio_put(LED_RED, 0); gpio_put(LED_GREEN, 1); gpio_put(LED_BLUE, 0); break;
        case 3: gpio_put(LED_RED, 1); gpio_put(LED_GREEN, 0); gpio_put(LED_BLUE, 0); break;
        case 5: gpio_put(LED_RED, 1); gpio_put(LED_GREEN, 0); gpio_put(LED_BLUE, 1); break;
        default: gpio_put(LED_RED, 0); gpio_put(LED_GREEN, 0); gpio_put(LED_BLUE, 0); break;
    }
}

// Toca um tom no buzzer com frequência e duração específicas.
void play_tone(uint freq, uint duration_ms);

// Toca um bip de confirmação de ação.
void play_confirmation_beep() {
    play_tone(600, 120);
}

// Toca um bip duplo de erro.
void play_error_beep() {
    play_tone(400, 150);
    sleep_ms(50);
    play_tone(400, 150);
}

// Toca a melodia de inicialização.
void play_startup_melody() {
    play_tone(NOTE_C5, 100);
    play_tone(NOTE_E5, 100);
    play_tone(NOTE_G5, 100);
    play_tone(NOTE_C6, 150);
}

// Configura o estado de uma nova mensagem temporária para o display.
void set_temporary_message(temp_message_state_t new_state) {
    current_message_state = new_state;
    message_timestamp = get_absolute_time();
}

// Atualiza todo o conteúdo do display OLED com base no estado atual.
void atualizar_display(bool montado, bool capturando, uint32_t contador_amostras); 

// Salva uma linha de dados do sensor no arquivo CSV no cartão SD.
bool salvar_dados(char *arquivo, int16_t aceleracao[3], int16_t gyro[3]);

// Define o estado do LED RGB baseado no estado do sistema.
void att_estado();

// Rotina de tratamento de interrupção para os botões.
static void callback_button(uint gpio, uint32_t events);

// Inicializa todos os pinos de GPIO, periféricos I2C e interrupções.
void init_pins();

// Função principal (Ponto de entrada do programa).
int main() {
    stdio_init_all();
    init_pins();

    printf("INICIANDO PROGRAMA!\n");

    run_setrtc("29/07/25 12:00:00");

    play_startup_melody();

    int16_t aceleracao[3], gyro[3];

    while (true) {
        if (play_confirmation_sound) {
            play_confirmation_beep();
            play_confirmation_sound = false;
        }
        if (play_error_sound) {
            play_error_beep();
            play_error_sound = false;
        }

        att_estado();

        if (flag_montagem) {
            if (!flag_estado_montagem) {
                printf("Montando o cartao!\n");
                run_mount();
                flag_estado_montagem = true;
                play_confirmation_beep();
                set_temporary_message(MSG_SD_MONTADO);
            }
        } else {
            if (flag_estado_montagem) {
                printf("Desmontando o cartao!\n");
                run_unmount();
                flag_estado_montagem = false;
                play_confirmation_beep();
                set_temporary_message(MSG_SD_DESMONTADO);
            }
        }

        mpu6050_read_raw(aceleracao, gyro);
        atualizar_display(flag_estado_montagem, flag_captura, N_amostras);

        if (flag_estado_montagem && flag_captura) {
            salvar_dados(arquivo, aceleracao, gyro);
        }

        sleep_ms(500);
    }
}
void play_tone(uint freq, uint duration_ms) {
    if (freq == 0) {
        sleep_ms(duration_ms);
        return;
    }
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    uint chan = pwm_gpio_to_channel(BUZZER);
    uint32_t wrap = 125000000 / freq; // Assumindo clock de 125MHz
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, chan, wrap / 2); // 50% de duty cycle
    pwm_set_enabled(slice_num, true);
    sleep_ms(duration_ms);
    pwm_set_enabled(slice_num, false);
}

void atualizar_display(bool montado, bool capturando, uint32_t contador_amostras) {
    ssd1306_fill(&ssd, false);

    // Desenha o Layout Fixo (bordas e títulos)
    ssd1306_rect(&ssd, 0, 0, ssd.width, ssd.height, true, false);
    ssd1306_draw_string(&ssd, "PICO DATALOGGER", 4, 4);
    ssd1306_line(&ssd, 1, 14, ssd.width - 2, 14, true);
    ssd1306_line(&ssd, 1, 50, ssd.width - 2, 50, true);

    // Lógica para Mensagens Temporárias vs. Status Normal
    bool showing_temp_message = (current_message_state != MSG_NONE) &&
                                (absolute_time_diff_us(message_timestamp, get_absolute_time()) < 2000000); // Duração: 2s

    if (showing_temp_message) {
        // Mostra uma mensagem de feedback por um curto período
        switch (current_message_state) {
            case MSG_SD_MONTADO:
                ssd1306_draw_string(&ssd, "-> SD Card ", 4, 25);
                ssd1306_draw_string(&ssd, "   Montado!", 4, 35);
                break;
            case MSG_SD_DESMONTADO:
                ssd1306_draw_string(&ssd, "-> SD Card", 4, 25);
                ssd1306_draw_string(&ssd, "   Desmontado.", 4, 35);
                break;
            case MSG_INICIANDO_GRAVACAO:
                ssd1306_draw_string(&ssd, "-> Gravacao", 4, 25);
                ssd1306_draw_string(&ssd, "   Iniciada!", 4, 35);
                break;
            case MSG_PARANDO_GRAVACAO:
                ssd1306_draw_string(&ssd, "-> Gravacao", 4, 25);
                ssd1306_draw_string(&ssd, "   Pausada.", 4, 35);
                break;
            case MSG_ERRO_AO_GRAVAR:
                ssd1306_draw_string(&ssd, "ERRO: Montar SD", 4, 25);
                ssd1306_draw_string(&ssd, "para gravar.", 4, 35);
                break;
            default: break;
        }
    } else {
        // Se não há mensagem temporária, mostra o status operacional
        current_message_state = MSG_NONE;

        if (!montado) {
            ssd1306_draw_string(&ssd, "Acao: Montar SD", 4, 25);
            ssd1306_draw_string(&ssd, "      (Botao B)", 4, 35);
        } else if (capturando) {
            char buffer_contador[20];
            sprintf(buffer_contador, "Amostras: %lu", contador_amostras);
            ssd1306_draw_string(&ssd, "Status: Capturando", 4, 25);
            ssd1306_draw_string(&ssd, buffer_contador, 4, 35);
        } else {
            ssd1306_draw_string(&ssd, "Acao: Gravar", 4, 25);
            ssd1306_draw_string(&ssd, "      (Botao A)", 4, 35);
        }
    }

    // Desenha o Status Fixo no Rodapé
    char status_rodape[20];
    if (montado && capturando) {
        strcpy(status_rodape, "MODO: GRAVANDO");
    } else if (montado) {
        strcpy(status_rodape, "MODO: PRONTO");
    } else {
        strcpy(status_rodape, "MODO: OCIOSO");
    }
    ssd1306_draw_string(&ssd, status_rodape, 4, 54);

    // Envia o buffer de imagem finalizado para o hardware do display
    ssd1306_send_data(&ssd);
}
bool salvar_dados(char *arquivo, int16_t aceleracao[3], int16_t gyro[3]) {
    FRESULT fr;
    FIL file;
    uint bw;

    fr = f_open(&file, arquivo, FA_OPEN_ALWAYS | FA_WRITE);
    if (f_size(&file) == 0) {
        const char *header = "Data,Tempo,X_ACLR,Y_ACLR,z_ACLR,X_GYRO,Y_GYRO,Z_GYRO\n";
        f_write(&file, header, strlen(header), &bw);
    } else {
        f_lseek(&file, f_size(&file));
    }

    datetime_t dt;
    char datetime_str[30];
    if (rtc_get_datetime(&dt)) {
        sprintf(datetime_str, "%04d-%02d-%02d,%02d:%02d:%02d",
                dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
    } else {
        strcpy(datetime_str, "0000-00-00,00:00:00");
    }

    char buffer[100];
    sprintf(buffer, "%s,%d,%d,%d,%d,%d,%d\n",
            datetime_str,
            aceleracao[0], aceleracao[1], aceleracao[2],
            gyro[0], gyro[1], gyro[2]);
    f_write(&file, buffer, strlen(buffer), &bw);

    f_close(&file);
    N_amostras++;
    return true;
}
void att_estado() {
    if (flag_estado_montagem && !flag_captura) {
        att_led(2); // Verde
    } else if (flag_estado_montagem && flag_captura) {
        att_led(3); // Vermelho
    } else if (!flag_estado_montagem && flag_captura) {
        att_led(5); // Roxo (Estado de erro)
    } else {
        att_led(1); // Amarelo
    }
}

static void callback_button(uint gpio, uint32_t events) {
    static absolute_time_t last_time_A = 0;
    static absolute_time_t last_time_B = 0;
    static absolute_time_t last_time_J = 0;
    absolute_time_t now = get_absolute_time();

    if (gpio == BUTTON_A) {
        if (absolute_time_diff_us(last_time_A, now) > 200000) { // 200ms debounce
            if (!flag_estado_montagem && !flag_captura) {
                set_temporary_message(MSG_ERRO_AO_GRAVAR);
                play_error_sound = true;
            } else {
                flag_captura = !flag_captura;
                set_temporary_message(flag_captura ? MSG_INICIANDO_GRAVACAO : MSG_PARANDO_GRAVACAO);
                play_confirmation_sound = true;
            }
            last_time_A = now;
        }
    } else if (gpio == BUTTON_B) {
        if (absolute_time_diff_us(last_time_B, now) > 200000) {
            flag_montagem = !flag_montagem;
            last_time_B = now;
        }
    } else if (gpio == JOYSTICK_BT) {
        if (absolute_time_diff_us(last_time_J, now) > 200000) { // Usa last_time_J
            reset_usb_boot(0, 0);
            last_time_J = now;
        }
    }
}

void init_pins() {
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);

    gpio_init(LED_BLUE);
    gpio_init(LED_GREEN);
    gpio_init(LED_RED);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(JOYSTICK_BT);
    gpio_set_dir(JOYSTICK_BT, GPIO_IN);
    gpio_pull_up(JOYSTICK_BT);

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &callback_button);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &callback_button);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BT, GPIO_IRQ_EDGE_FALL, true, &callback_button);

    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    i2c_init(I2C_PORT_SENSOR, 400 * 1000);
    gpio_set_function(I2C_SDA_SENSOR, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_SENSOR, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_SENSOR);
    gpio_pull_up(I2C_SCL_SENSOR);
    bi_decl(bi_2pins_with_func(I2C_SDA_SENSOR, I2C_SCL_SENSOR, GPIO_FUNC_I2C));

    ssd1306_init(&ssd, DISP_W, DISP_H, false, ENDERECO_DISP, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    atualizar_display(false, false, 0);

    mpu6050_reset();
}
