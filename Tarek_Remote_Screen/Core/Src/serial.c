/*
 * serial.c
 *
 * Created on: 6 mars 2019
 * Author: Joel
 *
 * Modified on: 20 Avril 2026
 * Author: ALLAM Tarek
 * Modified for STM32 HAL
 */

#include "bibliotheque.h" // Contient main.h et serial.h

// Variable générée par CubeMX pour la liaison série (souvent huart2 pour le PC)
extern UART_HandleTypeDef huart2;

// -------------------------------------------------------------------------
// --- NOUVEAU CODE STM32 (HAL) --------------------------------------------
// -------------------------------------------------------------------------

int putchar(int c) {
    uint8_t ch = (uint8_t)c;
    // On envoie le caractère via l'USART avec un délai maximum (HAL_MAX_DELAY)
    HAL_UART_Transmit(&huart2, &ch, 1, HAL_MAX_DELAY);
    return c;
}

int getchar(void) {
    uint8_t ch = 0;
    // On attend un caractère de l'USART.
    // Attention : HAL_MAX_DELAY bloque le programme tant qu'on ne reçoit rien !
    // Pour tes menus, on utilisera plutôt HAL_UART_Receive_IT() dans le main.c
    HAL_UART_Receive(&huart2, &ch, 1, HAL_MAX_DELAY);
    return ch;
}

void print(const char *s) {
    // Tant qu'on n'a pas atteint la fin de la chaîne (le caractère nul '\0')
    while(*s) {
        putchar(*s++);
    }
}

void printx(const uint8_t c) {
    static char hex_table[] = "0123456789abcdef";
    putchar(hex_table[(c & 0xF0) >> 4]);
    putchar(hex_table[c & 0x0F]);
}

void printnum99(const uint8_t c) {
    char u, d;
    d = c / 10 + 0x30; // 0x30 est le code ASCII pour '0'
    u = c % 10 + 0x30;
    putchar(d);
    putchar(u);
}

// -------------------------------------------------------------------------
// --- ANCIEN CODE MSP430 (CONSERVÉ POUR HISTORIQUE) -----------------------
// -------------------------------------------------------------------------

/*
const unsigned long MCLK_HZ = 16000000;          // SMCLK frequency in Hz
const unsigned BPS = 9600;                       // ASYNC serial baud rate
const unsigned long baud_rate_20_bits = (MCLK_HZ + (BPS >> 1)) / BPS; // Bit rate divisor

int getchar(void) {
    while(!(IFG2 & UCA0RXIFG));
    IFG2 &= ~UCA0RXIFG;
    return UCA0RXBUF;
}

int putchar(int c) {
    while(!(IFG2 & UCA0TXIFG));                  // wait for TX buffer to be empty
    UCA0TXBUF = c;
    return c;
}

// Les fonctions print, printx, printnum99 étaient identiques à la nouvelle version
// car elles ne font qu'utiliser putchar() !

void configureUART(void) {
    // OBSOLÈTE : Tout ceci est maintenant géré graphiquement par CubeMX
    // dans la fonction MX_USART2_UART_Init() du fichier main.c
    P3DIR &= ~(BIT4 | BIT5);
    P3SEL  |= BIT4 | BIT5;
    P3SEL2 &= ~BIT4 & ~BIT5;
    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL  = CALDCO_16MHZ;
    __delay_cycles(16000000/100);
    UCA0CTL1 = UCSWRST;
    UCA0CTL0 = 0;
    UCA0BR1 = (baud_rate_20_bits >> 12) & 0xFF;
    UCA0BR0 = (baud_rate_20_bits >> 4) & 0xFF;
    UCA0MCTL = ((baud_rate_20_bits << 4) & 0xF0) | UCOS16;
    UCA0CTL1 = UCSSEL_2;
}
*/
