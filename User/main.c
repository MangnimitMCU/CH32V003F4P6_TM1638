

#include "debug.h"

#define GPIO_LED_PORT           GPIOD
#define GPIO_LED_PIN            GPIO_Pin_0

#define GPIO_STB_PORT           GPIOC
#define GPIO_STB_PIN            GPIO_Pin_4

#define GPIO_SCK_PORT           GPIOC
#define GPIO_SCK_PIN            GPIO_Pin_5

#define GPIO_MOSI_PORT          GPIOC
#define GPIO_MOSI_PIN           GPIO_Pin_6

#define GPIO_MISO_PORT          GPIOC
#define GPIO_MISO_PIN           GPIO_Pin_7

// Command-1 Data command setting
#define TM1638_CMD1             0x40
#define TM1638_CMD1_WR_DATA     0x00    // Write data to display
#define TM1638_CMD1_RD_DATA     0x02    // Read key scan data

// Command-2 Address command setting
#define TM1638_CMD2             0xC0

// Command-3 Display control
#define tm1638_bright_default   2
#define TM1638_CMD3             0x80
#define TM1638_DISPLAY_ON       0x08

static u8 tm1638_bright = tm1638_bright_default;
static u8 tm1638_display_on = TM1638_DISPLAY_ON;

const u8 TubeTab[]={
    0x3F, 0x06, 0x5B, 0x4F, 0x66,
    0x6D, 0x7D, 0x07, 0x7F, 0x6F,
    0x77, 0x7C, 0x39, 0x5E, 0x79,
    0x71
};//0~9, A, b, C, d, E, F

#define STB_HIGH        GPIO_WriteBit(GPIO_STB_PORT, GPIO_STB_PIN, Bit_SET);
#define STB_LOW         GPIO_WriteBit(GPIO_STB_PORT, GPIO_STB_PIN, Bit_RESET);

typedef struct tm1638_struct{
    u8 segment;
    u8 led:1;
}tm1638_struct;

tm1638_struct tm1638[8];
u8 key;

/////////////////////////////////////////////////////////////////////
u8 SPI_Transfer(u8 spi_data) {
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, spi_data);
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI1);
}

/////////////////////////////////////////////////////////////////////
void TM1638_Write(u8 data) {
    SPI_Transfer(data);
}

/////////////////////////////////////////////////////////////////////
u8 TM1638_Read(void) {
    u8 data = 0;
    data = SPI_Transfer(0xFF);
    return data;
}

/////////////////////////////////////////////////////////////////////
void TM1638_Command(u8 command) {
    STB_LOW;
    TM1638_Write(command);
    STB_HIGH;
}

/////////////////////////////////////////////////////////////////////
void TM1638_ControlDisplay(void) {
    u8 control = tm1638_bright;

    if(tm1638_display_on) {
        control |= TM1638_DISPLAY_ON;
    }

    TM1638_Command(TM1638_CMD3|control);
}

/////////////////////////////////////////////////////////////////////
void TM1638_SetBrightness(u8 brightness) {
    tm1638_bright = brightness&0x07;
    TM1638_ControlDisplay();
}

/////////////////////////////////////////////////////////////////////
void TM1638_Enable(u8 display_on_off) {
    if(display_on_off) { tm1638_display_on = 1; }
    else { tm1638_display_on = 0; }
    TM1638_ControlDisplay();
}

/////////////////////////////////////////////////////////////////////
void TM1638_SetSegments(u8 position, u8 len) {
    u8 u8cnt;

    TM1638_Command(TM1638_CMD1|TM1638_CMD1_WR_DATA);

    STB_LOW;
    TM1638_Write(TM1638_CMD2|(position&0x0F));
    for(u8cnt=0;u8cnt<len;u8cnt++) {
        TM1638_Write(tm1638[u8cnt].segment);
        TM1638_Write(tm1638[u8cnt].led);
    }
    STB_HIGH;

    TM1638_ControlDisplay();
}

/////////////////////////////////////////////////////////////////////
u8 TM1638_ReadKey(void) {
    u8 u8cnt, button = 0;

    STB_LOW;
    TM1638_Write(TM1638_CMD1|TM1638_CMD1_RD_DATA);
    for(u8cnt=0;u8cnt<4;u8cnt++) {
        button |= TM1638_Read()<<u8cnt;
    }
    STB_HIGH;

    return button;
}

/////////////////////////////////////////////////////////////////////
void TM1638_Clear(void) {
    u8 u8cnt;

    TM1638_Command(0x40);   // Set Auto Address

    STB_LOW;
    TM1638_Write(0xC0);     // Set Start Address
    for (u8cnt=0;u8cnt<16;u8cnt++) {
        TM1638_Write(0x00);
    }
    STB_HIGH;
}

/////////////////////////////////////////////////////////////////////
void SPI_Config(void) {
    SPI_InitTypeDef SPI_IniyStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    SPI_IniyStructure.SPI_FirstBit = SPI_FirstBit_LSB;
    SPI_IniyStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_IniyStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;
    SPI_IniyStructure.SPI_Mode = SPI_Mode_Master;
    SPI_IniyStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_IniyStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_IniyStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_IniyStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_Init(SPI1, &SPI_IniyStructure);

    SPI_Cmd(SPI1, ENABLE);
}

/////////////////////////////////////////////////////////////////////
void GPIO_Config(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; // LED
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; // STB
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; // SCK
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; // MOSI
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7; // MISO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/////////////////////////////////////////////////////////////////////
int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SystemCoreClockUpdate();
    GPIO_Config();
    SPI_Config();
    Delay_Init();

    TM1638_Clear();
    TM1638_SetBrightness(7);

    //led[0].led = 1;
    //led[7].segment = TubeTab[3];

    while(1) {
        /*GPIO_WriteBit(GPIO_LED_PORT, GPIO_LED_PIN, Bit_RESET);
        Delay_Ms(500);
        GPIO_WriteBit(GPIO_LED_PORT, GPIO_LED_PIN, Bit_SET);
        Delay_Ms(500);*/

        TM1638_SetSegments(0, 8);

        key = TM1638_ReadKey();
        tm1638[0].led = key&0x01?1:0;
        tm1638[1].led = key&0x02?1:0;
        tm1638[2].led = key&0x04?1:0;
        tm1638[3].led = key&0x08?1:0;
        tm1638[4].led = key&0x10?1:0;
        tm1638[5].led = key&0x20?1:0;
        tm1638[6].led = key&0x40?1:0;
        tm1638[7].led = key&0x80?1:0;

        if(key) {
            //tm1638[0].segment = 0;
            tm1638[1].segment = 0b01000000;
            tm1638[2].segment = 0b01110011;
            tm1638[3].segment = 0b01010000;
            tm1638[4].segment = 0b01111001;
            tm1638[5].segment = 0b01101101;
            tm1638[6].segment = 0b01101101;
            tm1638[7].segment = 0b01000000;
        }
        else {
            //tm1638[0].segment = 0;
            tm1638[1].segment = 0;
            tm1638[2].segment = 0;
            tm1638[3].segment = 0;
            tm1638[4].segment = 0;
            tm1638[5].segment = 0;
            tm1638[6].segment = 0;
            tm1638[7].segment = 0;
        }
    }
}
