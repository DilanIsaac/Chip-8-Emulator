#ifndef CHIP8_H
#define CHIP8_H
#include<iostream>
#include <cstdint>
#include <vector>

class Chip8 {
private:
    uint8_t memory[4096];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delayTimer;
    uint8_t soundTimer;

    static const uint8_t fontset[80];

public:
    uint8_t display[64 * 32];
    uint8_t keypad[16];
    bool drawFlag;

    Chip8();
    void initialize();
    bool loadROM(const char* filename);
    void loadROMFromMemory(const std::vector<uint8_t>& romData);
    void cycle();
    void renderConsole() const;
    void renderDebug() const {
        std::cout << "  [CPU RAYOS-X] PC: 0x" << std::hex << pc
                  << " | I: 0x" << I
                  << " | V0(X): " << std::dec << (int)V[0]
                  << " | V1(Y): " << (int)V[1]
                  << " | VF(Colision): " << (int)V[0xF] << "      \n";
    }
};

#endif
