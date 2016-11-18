//
// Created by development on 11/18/16.
//

#include "NoFlashSpi.h"

//********** SPI Code in RAM

 void ICACHE_RAM_ATTR my_setDataBits(uint16_t bits) {
    const uint32_t mask = ~((SPIMMOSI << SPILMOSI) | (SPIMMISO << SPILMISO));
    bits--;
    SPI1U1 = ((SPI1U1 & mask) | ((bits << SPILMOSI) | (bits << SPILMISO)));
}


 uint8_t ICACHE_RAM_ATTR my_transfer(uint8_t data) {
    while(SPI1CMD & SPIBUSY) {}
    // reset to 8Bit mode
    my_setDataBits(8);
    SPI1W0 = data;
    SPI1CMD |= SPIBUSY;
    while(SPI1CMD & SPIBUSY) {}
    return (uint8_t) (SPI1W0 & 0xff);
}


 uint16_t ICACHE_RAM_ATTR my_transfer16(uint16_t data) {
    union {
        uint16_t val;
        struct {
            uint8_t lsb;
            uint8_t msb;
        };
    } in, out;
    in.val = data;

    if((SPI1C & (SPICWBO | SPICRBO))) {
        //LSBFIRST
        out.lsb = my_transfer(in.lsb);
        out.msb = my_transfer(in.msb);
    } else {
        //MSBFIRST
        out.msb = my_transfer(in.msb);
        out.lsb = my_transfer(in.lsb);
    }
    return out.val;
}