#include "hardware/i2c.h"

// Definição dos pinos I2C para o MPU6050
#define I2C_PORT i2c0                 // I2C0 usa pinos 0 e 1
#define I2C_SDA 0
#define I2C_SCL 1


// Endereço padrão do MPU6050
extern int addr;


// Função para resetar e inicializar o MPU6050
extern void mpu6050_reset();
// Função para ler dados crus do acelerômetro, giroscópio e temperatura
extern void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3]);