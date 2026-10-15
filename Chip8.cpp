#include "Chip8.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <ctime>

const uint8_t Chip8::fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8() {
    initialize();
}

void Chip8::initialize() {
    pc = 0x200;
    I = 0;
    sp = 0;
    delayTimer = 0;
    soundTimer = 0;
    drawFlag = false;

    std::memset(memory, 0, sizeof(memory));
    std::memset(V, 0, sizeof(V));
    std::memset(stack, 0, sizeof(stack));
    std::memset(display, 0, sizeof(display));
    std::memset(keypad, 0, sizeof(keypad));

    for (int i = 0; i < 80; ++i) {
        memory[i] = fontset[i];
    }

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

bool Chip8::loadROM(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false;

    std::streampos size = file.tellg();
    if (size > (4096 - 512)) return false;

    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(&memory[0x200]), size);
    file.close();
    return true;
}

void Chip8::loadROMFromMemory(const std::vector<uint8_t>& romData) {
    for (size_t i = 0; i < romData.size() && (0x200 + i) < 4096; ++i) {
        memory[0x200 + i] = romData[i];
    }
}

void Chip8::cycle() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];

    uint8_t X   = (opcode & 0x0F00) >> 8;
    uint8_t Y   = (opcode & 0x00F0) >> 4;
    uint8_t N   = opcode & 0x000F;
    uint8_t NN  = opcode & 0x00FF;
    uint16_t NNN = opcode & 0x0FFF;

    pc += 2;

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                std::memset(display, 0, sizeof(display));
                drawFlag = true;
            } else if (opcode == 0x00EE) {
                --sp;
                pc = stack[sp];
            }
            break;

        case 0x1000: pc = NNN; break;
        case 0x2000: stack[sp++] = pc; pc = NNN; break;
        case 0x3000: if (V[X] == NN) pc += 2; break;
        case 0x4000: if (V[X] != NN) pc += 2; break;
        case 0x5000: if (V[X] == V[Y]) pc += 2; break;
        case 0x6000: V[X] = NN; break;
        case 0x7000: V[X] += NN; break;

        case 0x8000:
            switch (N) {
                case 0x0: V[X] = V[Y]; break;
                case 0x1: V[X] |= V[Y]; break;
                case 0x2: V[X] &= V[Y]; break;
                case 0x3: V[X] ^= V[Y]; break;
                case 0x4: {
                    uint16_t sum = V[X] + V[Y];
                    V[0xF] = (sum > 255) ? 1 : 0;
                    V[X] = sum & 0xFF;
                    break;
                }
                case 0x5: V[0xF] = (V[X] >= V[Y]) ? 1 : 0; V[X] -= V[Y]; break;
                case 0x6: V[0xF] = V[X] & 0x1; V[X] >>= 1; break;
                case 0x7: V[0xF] = (V[Y] >= V[X]) ? 1 : 0; V[X] = V[Y] - V[X]; break;
                case 0xE: V[0xF] = (V[X] & 0x80) >> 7; V[X] <<= 1; break;
            }
            break;

        case 0x9000: if (V[X] != V[Y]) pc += 2; break;
        case 0xA000: I = NNN; break;
        case 0xB000: pc = NNN + V[0]; break;
        case 0xC000: V[X] = (std::rand() % 256) & NN; break;

        case 0xD000: {
            uint8_t xPos = V[X] % 64;
            uint8_t yPos = V[Y] % 32;
            V[0xF] = 0;

            for (int row = 0; row < N; ++row) {
                uint8_t spriteByte = memory[I + row];
                for (int col = 0; col < 8; ++col) {
                    if ((spriteByte & (0x80 >> col)) != 0) {
                        int idx = ((xPos + col) % 64) + (((yPos + row) % 32) * 64);
                        if (display[idx] == 1) V[0xF] = 1;
                        display[idx] ^= 1;
                    }
                }
            }
            drawFlag = true;
            break;
        }

        case 0xE000:
            if (NN == 0x9E && keypad[V[X]] != 0) pc += 2;
            if (NN == 0xA1 && keypad[V[X]] == 0) pc += 2;
            break;

        case 0xF000:
            switch (NN) {
                case 0x07: V[X] = delayTimer; break;
                case 0x0A: {
                    bool pressed = false;
                    for (int i = 0; i < 16; ++i) {
                        if (keypad[i] != 0) { V[X] = i; pressed = true; break; }
                    }
                    if (!pressed) pc -= 2;
                    break;
                }
                case 0x15: delayTimer = V[X]; break;
                case 0x18: soundTimer = V[X]; break;
                case 0x1E: I += V[X]; break;
                case 0x29: I = V[X] * 5; break;
                case 0x33:
                    memory[I]     = V[X] / 100;
                    memory[I + 1] = (V[X] / 10) % 10;
                    memory[I + 2] = V[X] % 10;
                    break;
                case 0x55: for (int i = 0; i <= X; ++i) memory[I + i] = V[i]; break;
                case 0x65: for (int i = 0; i <= X; ++i) V[i] = memory[I + i]; break;
            }
            break;
    }

    if (delayTimer > 0) --delayTimer;
    if (soundTimer > 0) --soundTimer;
}

void Chip8::renderConsole() const {
    std::cout << "\033[H"; // Regresa el cursor al inicio sin parpadear
    std::cout << "  +" << std::string(128, '-') << "+\n";
    for (int y = 0; y < 32; ++y) {
        std::cout << "  |";
        for (int x = 0; x < 64; ++x) {
            // \xDB es el bloque solido completo en la consola de Windows
            std::cout << (display[x + y * 64] ? "\xDB\xDB" : "  ");
        }
        std::cout << "|\n";
    }
    std::cout << "  +" << std::string(128, '-') << "+\n";
}
