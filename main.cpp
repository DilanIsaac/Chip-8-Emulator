#include "Chip8.h"
#include <iostream>
#include <vector>
#include <windows.h>
#include <conio.h>

int main() {
    Chip8 emulator;
    int opcion = 0;

    std::cout << "==========================================\n";
    std::cout << "       EMULADOR CHIP-8 EN C++ PURO        \n";
    std::cout << "==========================================\n";
    std::cout << "Selecciona una opcion:\n";
    std::cout << "  1. Jugar PONG (Controles: W = Subir, S = Bajar)\n";
    std::cout << "  2. Animacion de Carga del Inicio (Patron Laberinto)\n";
    std::cout << "Opcion (1-2): ";
    std::cin >> opcion;

    if (opcion == 2) {
        // OPCION 2: Animacion de Carga de Inicio (Va llenando la pantalla en vivo)
        std::vector<uint8_t> bootAnimROM = {
            0x00, 0xE0, // 0x200: Limpiar pantalla
            0x60, 0x00, // 0x202: V0 = 0 (X)
            0x61, 0x00, // 0x204: V1 = 0 (Y)
            // Bucle de dibujado (0x206):
            0xA2, 0x24, // 0x206: I = 0x224 (Sprite /)
            0xC2, 0x01, // 0x208: V2 = Random(0 o 1)
            0x32, 0x01, // 0x20A: Si V2 == 1, salta
            0xA2, 0x20, // 0x20C: I = 0x220 (Sprite \)
            0xD0, 0x14, // 0x20E: Dibujar bloque en (V0, V1)
            0x70, 0x04, // 0x210: V0 += 4 (Avanzar en X)
            0x30, 0x40, // 0x212: Si V0 == 64, fin de fila
            0x12, 0x06, // 0x214: Seguir en la misma fila
            0x60, 0x00, // 0x216: V0 = 0 (Reiniciar X)
            0x71, 0x04, // 0x218: V1 += 4 (Bajar fila en Y)
            0x31, 0x20, // 0x21A: Si V1 == 32, pantalla llena -> reiniciar animacion
            0x12, 0x06, // 0x21C: Siguiente fila
            0x12, 0x00, // 0x21E: Al llenar la pantalla, limpia y vuelve a cargar infinito
            // Sprites (0x220 y 0x224):
            0x80, 0x40, 0x20, 0x10, // Barra \ (4 bytes)
            0x10, 0x20, 0x40, 0x80  // Barra / (4 bytes)
        };
        emulator.loadROMFromMemory(bootAnimROM);
    }
    else {
        // OPCION 1: PONG / SQUASH JUGABLE (Paleta con W y S + Pelota con rebote y colision XOR)
        // V0 = Y de tu paleta (X fijo en 2)
        // V1 = X de la pelota, V2 = Y de la pelota
        // V3 = Velocidad X pelota (+1 o -1), V4 = Velocidad Y pelota (+1 o -1)
        std::vector<uint8_t> pongROM = {
            0x00, 0xE0, // 0x200: Limpiar pantalla
            0x60, 0x0C, // 0x202: V0 = 12 (Posicion Y inicial de tu paleta)
            0x61, 0x1E, // 0x204: V1 = 30 (Posicion X inicial de la pelota)
            0x62, 0x0E, // 0x206: V2 = 14 (Posicion Y inicial de la pelota)
            0x63, 0xFF, // 0x208: V3 = -1 (Pelota empieza viajando hacia tu paleta)
            0x64, 0x01, // 0x20A: V4 = 1  (Pelota viajando en diagonal abajo)

            // Dibujar paleta y pelota iniciales:
            0x65, 0x02, // 0x20C: V5 = 2 (X fija de tu paleta a la izquierda)
            0xA2, 0x5E, // 0x20E: I = 0x25E (Sprite Paleta)
            0xD5, 0x06, // 0x210: Dibujar Paleta en (V5, V0) de 6 pixeles de alto
            0xA2, 0x64, // 0x212: I = 0x264 (Sprite Pelota)
            0xD1, 0x22, // 0x214: Dibujar Pelota en (V1, V2) de 2 pixeles de alto

            // --- BUCLE PRINCIPAL DEL JUEGO (0x216) ---
            // 1. Mover Paleta con W (Tecla 2) y S (Tecla 8)
            0xA2, 0x5E, // 0x216: I = Sprite Paleta
            0xD5, 0x06, // 0x218: Borrar Paleta actual (XOR)

            0x66, 0x02, // 0x21A: V6 = Tecla W (2)
            0xE6, 0xA1, // 0x21C: Si NO presionas W, salta
            0x70, 0xFE, // 0x21E: V0 -= 2 (Subir paleta)

            0x66, 0x08, // 0x220: V6 = Tecla S (8)
            0xE6, 0xA1, // 0x222: Si NO presionas S, salta
            0x70, 0x02, // 0x224: V0 += 2 (Bajar paleta)

            0xD5, 0x06, // 0x226: Redibujar Paleta en nueva posicion

            // 2. Mover Pelota y calcular rebotes
            0xA2, 0x64, // 0x228: I = Sprite Pelota
            0xD1, 0x22, // 0x22A: Borrar Pelota actual (XOR)

            0x81, 0x34, // 0x22C: V1 += V3 (X_pelota += Vel_X)
            0x82, 0x44, // 0x22E: V2 += V4 (Y_pelota += Vel_Y)

            // Rebote en techo (Y == 0) y piso (Y == 30):
            0x32, 0x00, // 0x230: Si Y == 0, salta a invertir Vel_Y
            0x12, 0x36, // 0x232: Si no, revisa el piso
            0x12, 0x3A, // 0x234: Ir a invertir Vel_Y
            0x32, 0x1E, // 0x236: Si Y == 30, salta a invertir Vel_Y
            0x12, 0x3E, // 0x238: Si no toco ni techo ni piso, continuar
            // Invertir Vel_Y (V4 = 0 - V4):
            0x67, 0x00, // 0x23A: V7 = 0
            0x84, 0x77, // 0x23C: V4 = V7 - V4 (Cambia +1 a -1 o viceversa)

            // Rebote en pared derecha (X == 62) o pared izquierda (X == 0):
            0x31, 0x3E, // 0x23E: Si X == 62 (pared derecha), rebota
            0x12, 0x44, // 0x240: Revisar pared izquierda
            0x12, 0x48, // 0x242: Ir a invertir Vel_X
            0x31, 0x00, // 0x244: Si X == 0 (fondo izquierdo), rebota
            0x12, 0x4C, // 0x246: Continuar a dibujar pelota
            // Invertir Vel_X (V3 = 0 - V3):
            0x67, 0x00, // 0x248: V7 = 0
            0x83, 0x77, // 0x24A: V3 = V7 - V3

            // 3. Dibujar pelota y detectar CHOQUE CON TU PALETA (VF == 1)
            0xD1, 0x22, // 0x24C: Dibujar Pelota (Si choca con tu paleta, VF se pone en 1)
            0x3F, 0x01, // 0x24E: Si VF == 1 (¡Choco con la paleta!), salta
            0x12, 0x58, // 0x250: Si no choco, ir al final del frame
            // Si choco con la paleta, rebota hacia la derecha (V3 = +1) y corrige el pixel:
            0x63, 0x01, // 0x252: V3 = 1 (Hacia la derecha)
            0xA2, 0x5E, // 0x254: I = Sprite Paleta
            0xD5, 0x06, // 0x256: Repintar paleta para que no le quede un hueco por el XOR

            // Fin del frame, repetir bucle:
            0x12, 0x16, // 0x258: Volver al inicio del bucle (0x216)
            0x00, 0x00, 0x00, 0x00, // Alineacion

            // --- SPRITES ---
            // 0x25E: Sprite Paleta (6 bytes de alto):
            0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0,
            // 0x264: Sprite Pelota (2x2 pixeles):
            0xC0, 0xC0
        };
        emulator.loadROMFromMemory(pongROM);
    }

    system("cls");

    while (true) {
        // Limpiar teclas al inicio del frame
        for (int k = 0; k < 16; ++k) emulator.keypad[k] = 0;

        // Leer todos los pulsos acumulados en el buffer para que la paleta responda al instante
        while (_kbhit()) {
            char key = tolower(_getch());
            if (key == 27) return 0; // ESC para salir
            if (key == 'w') emulator.keypad[0x2] = 1;
            if (key == 's') emulator.keypad[0x8] = 1;
        }

        if (opcion == 2) {
            // En la animacion de carga ejecutamos 1 instruccion por ciclo para ver como se arma bloque por bloque
            emulator.cycle();
        } else {
            // En Pong ejecutamos 18 instrucciones (1 frame logico completo) antes de pintar pantalla
            // Asi no se ve el parpadeo entre borrar y volver a pintar la pelota
            for (int i = 0; i < 18; ++i) {
                emulator.cycle();
            }
        }

        if (emulator.drawFlag) {
            emulator.renderConsole();
            emulator.renderDebug();
            if (opcion == 1) {
                std::cout << "  [PONG: Presiona W (Subir) y S (Bajar) para mover tu paleta | ESC salir]\n";
            } else {
                std::cout << "  [ANIMACION DE CARGA: Generando patron con Opcode 0xC201 | ESC salir]\n";
            }
        }
    }
}

