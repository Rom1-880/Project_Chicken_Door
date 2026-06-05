/* moteur.h */
#ifndef INC_MOTEUR_H_
#define INC_MOTEUR_H_



// --- Broches LED de signalisation (gauche à droite) ---
#define LED_G_1_PORT  GPIOB
#define LED_G_1_PIN   GPIO_PIN_7  // LED 1 (la plus à gauche)
#define LED_G_2_PORT  GPIOB
#define LED_G_2_PIN   GPIO_PIN_3  // LED 2
#define LED_D_3_PORT  GPIOB
#define LED_D_3_PIN   GPIO_PIN_8  // LED 3 — ⚠ conflit possible I2C1_SCL
#define LED_D_4_PORT  GPIOB
#define LED_D_4_PIN   GPIO_PIN_9  // LED 4 (la plus à droite) — ⚠ conflit I2C1_SDA

#include "main.h"

// Définition des différents états de la machine à états (FSM)
typedef enum {
    MOTEUR_OFF,
    DEMARRAGE_MOTEUR,
    CALIBRATION_MOTEUR,
    MOTEUR_MARCHE,
    MOTEUR_ERREUR_ABSENCE,
    MOTEUR_ERREUR_BLOCAGE
} MotorState_t;


// Prototypes des fonctions publiques (accessibles par le main.c)
void Motor_Forward(void);
void Motor_Reverse(void);
void Motor_Stop(void);
void Motor_SetSpeed(uint16_t speed);
float Get_Motor_Current(void);
float Update_Moving_Average(float new_sample);
void Motor_Periodic_Update(void);
void Process_UART_Command(void);
void Update_LED_Timeout(void);
void Gerer_Erreur_Moteur(void);
void Enter_Low_Power_Mode(void);

extern MotorState_t motor_state;
extern uint8_t led_active; // (Remplace uint8_t par le vrai type de ta led_active si c'est un int ou un bool)
#endif /* INC_MOTEUR_H_ */
