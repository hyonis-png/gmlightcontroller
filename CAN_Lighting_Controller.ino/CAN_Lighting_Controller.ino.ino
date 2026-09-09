#include <Arduino.h>
#include <avr/io.h>
#include <stdint.h>
#include "lights.h"

#define LIGHTING_COMMAND_ID 2047
#define CMD_HAZARDS_ON      15
#define CMD_HAZARDS_OFF     16

bool hazards_enabled = false;
bool blink_on = false;
uint32_t last_toggle = 0;

void spi_init(void)
{
    // D53 CS: start HIGH.
    PORTB |= (1 << PB0);

    // D53 CS, D52 SCK, D51 MOSI: outputs.
    DDRB |= (1 << PB0)
          | (1 << PB1)
          | (1 << PB2);

    // D50 MISO: input.
    DDRB &= ~(1 << PB3);

    // Master, mode 0, MSB first, 1 MHz SPI.
    SPCR = (1 << SPE)
         | (1 << MSTR)
         | (1 << SPR0);

    SPSR &= ~(1 << SPI2X);
}

void cs_low(void)
{
    PORTB &= ~(1 << PB0);
}

void cs_high(void)
{
    PORTB |= (1 << PB0);
}

uint8_t spi_transfer(uint8_t data)
{
    SPDR = data;

    while (!(SPSR & (1 << SPIF)))
    {
    }

    return SPDR;
}

void MCP2515_reset(void)
{
    cs_low();
    spi_transfer(0xC0);
    cs_high();

    delay(10);
}

void MCP2515_write(uint8_t address, uint8_t data)
{
    cs_low();
    spi_transfer(0x02);
    spi_transfer(address);
    spi_transfer(data);
    cs_high();
}

uint8_t MCP2515_read(uint8_t address)
{
    cs_low();
    spi_transfer(0x03);
    spi_transfer(address);
    uint8_t data = spi_transfer(0x00);
    cs_high();

    return data;
}

void MCP2515_init(uint8_t cnf1,
                  uint8_t cnf2,
                  uint8_t cnf3,
                  uint8_t loopback)
{
    MCP2515_reset();

    MCP2515_write(0x2A, cnf1);
    MCP2515_write(0x29, cnf2);
    MCP2515_write(0x28, cnf3);

    // Receive without filtering; rollover disabled.
    MCP2515_write(0x60, 0x60);

    // Loopback or normal mode.
    MCP2515_write(0x0F, loopback ? 0x40 : 0x00);
}

// Standard 11-bit data frames, transmit buffer 0.
// data must contain at least length bytes.
void CAN_SEND(uint16_t id, uint8_t length, uint8_t *data)
{
    if (length > 8 || id > 0x7FF)
        return;

    // Do not overwrite a pending transmission.
    if (MCP2515_read(0x30) & (1 << 3))
        return;

    MCP2515_write(0x35, length);
    MCP2515_write(0x31, id >> 3);
    MCP2515_write(0x32, (id & 0x07) << 5);

    for (uint8_t i = 0; i < length; i++)
    {
        MCP2515_write(0x36 + i, data[i]);
    }

    cs_low();
    spi_transfer(0x81);
    cs_high();
}

// Receive buffer 0 only.
// data must have space for 8 bytes.
uint8_t CAN_receive(uint16_t *id,
                    uint8_t *length,
                    uint8_t *data)
{
    uint8_t flags = MCP2515_read(0x2C);

    if ((flags & (1 << 0)) == 0)
    {
        return 0;
    }

    uint8_t high = MCP2515_read(0x61);
    uint8_t low = MCP2515_read(0x62);
    uint8_t control = MCP2515_read(0x60);

    *length = MCP2515_read(0x65) & 0x0F;

    // Accept standard data frames with valid lengths.
    uint8_t valid = (*length <= 8)
                  && ((low & 0x08) == 0)
                  && ((control & 0x08) == 0);

    if (valid)
    {
        *id = ((uint16_t)high << 3) | (low >> 5);

        for (uint8_t i = 0; i < *length; i++)
        {
            data[i] = MCP2515_read(0x66 + i);
        }
    }

    // Clear RX0IF to release receive buffer 0.
    cs_low();
    spi_transfer(0x05);
    spi_transfer(0x2C);
    spi_transfer(0x01);
    spi_transfer(0x00);
    cs_high();

    return valid;
}

void pin12_high(void)
{
    PORTB |= (1 << PB6);
}

void pin12_low(void)
{
    PORTB &= ~(1 << PB6);
}

void setup(void)
{
    // D12: start LOW and configure as output.
    PORTB &= ~(1 << PB6);
    DDRB |= (1 << PB6);

    // Initialize D14 and D15 using lights.cpp.
    lights_init();

    spi_init();

    // 8 MHz crystal, 500 kbit/s, normal mode.
    MCP2515_init(0x00, 0x90, 0x02, 0);
}

void loop(void)
{
    uint16_t id;
    uint8_t length;
    uint8_t data[8];

    if (CAN_receive(&id, &length, data))
    {
        if (id == LIGHTING_COMMAND_ID && length == 1)
        {
            if (data[0] == CMD_HAZARDS_ON)
            {
                // Repeated ON commands do not restart blinking.
                if (!hazards_enabled)
                {
                    hazards_enabled = true;
                    blink_on = true;
                    last_toggle = millis();
                    lights_set(HAZARD_ON);
                }
            }
            else if (data[0] == CMD_HAZARDS_OFF)
            {
                hazards_enabled = false;
                blink_on = false;
                lights_set(ALL_OFF);
            }
        }
    }

    uint32_t now = millis();

    if (hazards_enabled)
    {
        if ((uint32_t)(now - last_toggle) >= 500)
        {
            blink_on = !blink_on;
            last_toggle = now;

            lights_set(blink_on ? HAZARD_ON : ALL_OFF);
        }
    }
}