typedef enum {I2C_CMD_READ=1, I2C_CMD_WRITE=2} i2c_cmd_t;
typedef enum {REG_LSB=1, REG_MSB=2} regbyte_t;

void i2c_interrupt (void);
void i2c_init (unsigned char addr);

void          reg_set (unsigned char regnum, unsigned char val, regbyte_t sel);
unsigned char reg_get (unsigned char regnum, regbyte_t sel);
