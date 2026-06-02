/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN PD */
// --- Broches de commande du driver moteur ---
#define MOTEUR_NSLEEP_PORT  GPIOA
#define MOTEUR_NSLEEP_PIN   GPIO_PIN_9   // Réveil du driver (LOW = veille driver)
#define MOTEUR_AVANT_PORT   GPIOB
#define MOTEUR_AVANT_PIN    GPIO_PIN_1   // Sens avant : HIGH = actif
#define MOTEUR_ARRIERE_PORT GPIOA
#define MOTEUR_ARRIERE_PIN  GPIO_PIN_10  // Sens arrière : HIGH = actif

// --- Broches LED de signalisation (gauche à droite) ---
#define LED_G_1_PORT  GPIOB
#define LED_G_1_PIN   GPIO_PIN_7  // LED 1 (la plus à gauche)
#define LED_G_2_PORT  GPIOB
#define LED_G_2_PIN   GPIO_PIN_3  // LED 2
#define LED_D_3_PORT  GPIOB
#define LED_D_3_PIN   GPIO_PIN_8  // LED 3 — ⚠ conflit possible I2C1_SCL
#define LED_D_4_PORT  GPIOB
#define LED_D_4_PIN   GPIO_PIN_9  // LED 4 (la plus à droite) — ⚠ conflit I2C1_SDA
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef   hadc1;
LPTIM_HandleTypeDef hlptim1;
RTC_HandleTypeDef   hrtc;
TIM_HandleTypeDef   htim1;
UART_HandleTypeDef  huart2;

/* USER CODE BEGIN PV */

/* --- Énumération états physiques de la vanne (direction moteur) ----------- */
typedef enum {
    MOTEUR_ARRET = 0,
    MOTEUR_DROITE,
    MOTEUR_GAUCHE
} Motor_State_t;

/* --- Vitesse et commandes UART ------------------------------------------- */
uint8_t        speed_level    = 3;                          // Niveau actuel (1=lent … 4=rapide)
const uint16_t speed_table[4] = {700, 800, 900, 999};      // Table des 4 vitesses fixes
uint16_t       current_speed  = 900;                        // Valeur PWM correspondant au niveau 3
uint8_t        rx_data;                                     // Dernier octet reçu sur l'UART
uint8_t        rx_pending     = 0;  // 1 = octet de réveil disponible dans rx_data (capturé IT)
char msg_cmd[150];    // Dédié uniquement aux réponses de commandes ('D', 'A', 'S'...)
char msg_status[400]; // Dédié uniquement au gros tableau d'état périodique


/* --- Mesure de courant moteur via ADC ------------------------------------ */
uint32_t adc_value      = 0;    // Valeur brute 12 bits (0–4095)
float    courant_moteur = 0.0f; // Courant calculé en Ampères (instantané)
uint32_t last_tick      = 0;    // Référence pour la cadence de 500 ms



/* --- Filtre : moyenne glissante sur 5 échantillons ----------------------- */
float lecture[5]      = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Fenêtre FIFO
float somme_courant   = 0.0f; // Somme recalculée à chaque nouvelle mesure
float moyenne_courant = 0.0f; // Résultat lissé utilisé par la FSM de sécurité



/* --- Sécurité : calibration et détection de blocage/absence ------------- */
float    courant_fonctionnement_morteur = 0.0f; // Courant nominal mesuré après 3 s de marche
float    threshold                      = 0.9f; // Seuil de surcourant (recalculé après calibration)
int      compteur_securite              = 0;    // Nombre de dépassements consécutifs du seuil
uint32_t motor_start_time               = 0;    // Horodatage du dernier démarrage



/* --- Machine à états de sécurité du moteur -------------------------------- */
typedef enum {
    MOTEUR_OFF,             // Arrêt propre, MCU peut entrer en veille
    DEMARRAGE_MOTEUR,       // Phase initiale : surcourant transitoire ignoré
    CALIBRATION_MOTEUR,     // Mesure du courant nominal pour fixer le seuil
    MOTEUR_MARCHE,          // Surveillance active : détection blocage/absence
    MOTEUR_ERREUR_ABSENCE,  // Moteur déconnecté (courant < 0.03 A) — clignotement 3 s
    MOTEUR_ERREUR_BLOCAGE   // Moteur bloqué (surcourant × 5) — clignotement 0.5 s
} MotorState_t;

MotorState_t motor_state = MOTEUR_OFF; // État initial
uint32_t     elapsed     = 0;          // Durée ms depuis le dernier démarrage (accès global)



/* --- Gestion du timeout d'affichage LEDs vitesse ------------------------- */
uint32_t led_timer  = 0; // Horodatage du dernier changement de vitesse
uint8_t  led_active = 0; // 1 = LEDs vitesse allumées, empêche la mise en veille


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
void Enter_Low_Power_Mode(void);
void Update_LED_Timeout(void);
void Gerer_Erreur_Moteur(void);
static void UART_Send_Welcome_Msg(void);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */


/* USER CODE BEGIN 0 */
/* ---------------------------------------------------------------------------
 * UART_Send_Welcome_Msg — Affichage sécurisé du grand logo d'origine
 * --------------------------------------------------------------------------- */
static void UART_Send_Welcome_Msg(void)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 10);
    HAL_Delay(20);

    // Tableau fixe stocké en Flash avec les caractères spéciaux d'origine
    static const char logo[] =
            "                              ######\r\n"
            "                            ########     ##\r\n"
            "                            #########   ######\r\n"
            "                             #########  ########\r\n"
            "                   ########  ##########  ########\r\n"
            "                  ################################\r\n"
            "                  ################################\r\n"
            "                   ###############################\r\n"
            "                    ##########         ##########\r\n"
            "                     #######             #######\r\n"
            "                      #####         @@@   ######\r\n"
            "     ###########        #         @@@@@@@  #########\r\n"
            "   #######    #### #####           @@@@@    #########   #####\r\n"
            "  ######      ##########                    ########### ######\r\n"
            " ######      #### ######                    #################\r\n"
            " ######           ##### #####      ######     #######   #####    #####    #######   ###### ###### #####\r\n"
            "######           #############    #####    #####      #####   ####     ###### ###  ############\r\n"
            "######           ######  ######   ######  ######       ##### ####      ######  ### ######   ######\r\n"
            "######          ######   ######   ###### ######       ###########   ######   ### ######   ######\r\n"
            "######          ######   #####   ######  #####        ######  #####  #####  ###   #####    #####\r\n"
            "######          #####   ######   ######  #####       ######    ##### #########    ######   ######\r\n"
            "######         ######   ######   #####   #####       ######   ###### ######      #######   #####\r\n"
            "#######     #########   ######  ####### #######   ########    ###### ######    ########    #####\r\n"
            "  ##################    ################# ################    ################# ######    #######\r\n"
            "     #####    #####        ##       ##      ####    #####        ##      ###     #####       ###\r\n"
            "\r\n"
            "                                               ###########\r\n"
            "                                             ### ####   ####   #######     #######\r\n"
            "                                            ###  ####   ####  #########   #########\r\n"
            "                                            ### #####   #### #######################     ########\r\n"
            "                                             ## ####    #### ########################   ##### ###\r\n"
            "                                               #####   ##### ########################   ####\r\n"
            "                                               ####    ####  ########################  #####\r\n"
            "                                               ####   ####    ######################   ####\r\n"
            "                                              ###########      ####################   ####\r\n"
            "                                              ##########        ##################   #####\r\n\0";

    // Envoi précis basé sur sizeof pour ne pas baver sur le reste de la mémoire
        HAL_UART_Transmit(&huart2, (uint8_t*)logo, sizeof(logo) - 1, 1500);
        HAL_Delay(100);

        static const char bloc_info[] =
            "\r\n+------------------------------------------------+\r\n"
            "|     SYSTEME DE CONTROLE MOTEUR INITIALISE      |\r\n"
            "|            PRET A RECEVOIR LES ORDRES          |\r\n"
            "+------------------------------------------------+\r\n\r\n\0";

        HAL_UART_Transmit(&huart2, (uint8_t*)bloc_info, sizeof(bloc_info) - 1, 200);
    }
/*USER CODE END 0 */



/* ---------------------------------------------------------------------------
 * LED_AllOff
 * Éteint les 4 LEDs d'un coup.
 * Mutualisée : Motor_Stop, Update_LED_Timeout, Reset erreur.
 * --------------------------------------------------------------------------- */

static void LED_AllOff(void)
{
    HAL_GPIO_WritePin(LED_G_1_PORT, LED_G_1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_G_2_PORT, LED_G_2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_3_PORT, LED_D_3_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_4_PORT, LED_D_4_PIN, GPIO_PIN_RESET);
}



/* ---------------------------------------------------------------------------
 * LED_ToggleAll
 * Inverse l'état des 4 LEDs simultanément.
 * Utilisée par Gerer_Erreur_Moteur pour le clignotement non-bloquant.
 * --------------------------------------------------------------------------- */

static void LED_ToggleAll(void)
{
    HAL_GPIO_TogglePin(LED_G_1_PORT, LED_G_1_PIN);
    HAL_GPIO_TogglePin(LED_G_2_PORT, LED_G_2_PIN);
    HAL_GPIO_TogglePin(LED_D_3_PORT, LED_D_3_PIN);
    HAL_GPIO_TogglePin(LED_D_4_PORT, LED_D_4_PIN);
}



/* ---------------------------------------------------------------------------
 * Motor_HardStop
 * Coupe la PWM et met les pins de direction à 0 SANS modifier motor_state.
 * Utilisée par la FSM d'erreur pour figer le hardware avant de verrouiller l'état.
 * Différent de Motor_Stop() qui remet motor_state = MOTEUR_OFF.
 * --------------------------------------------------------------------------- */

static void Motor_HardStop(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1); // Coupe PWM marche arrière
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);        // Coupe PWM marche avant
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); // Pin direction pont H à 0
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET); // Pin direction pont H à 0
}



/* ---------------------------------------------------------------------------
 * PWM_StopAll
 * Arrête les deux PWM avant tout changement de direction.
 * Évite un court-circuit du pont en H si les deux sens sont actifs simultanément.
 * --------------------------------------------------------------------------- */

static void PWM_StopAll(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
}



/* ---------------------------------------------------------------------------
 * Motor_StartTimer
 * Réinitialise le chrono et le compteur de sécurité à chaque nouveau démarrage.
 * Mutualisé entre Motor_Forward et Motor_Reverse.
 * --------------------------------------------------------------------------- */

static void Motor_StartTimer(void)
{
    motor_start_time  = HAL_GetTick(); // Référence pour la FSM (demarrage, calibration, marche)
    motor_state       = DEMARRAGE_MOTEUR; // Ignore les surcourants pendant le transitoire
    compteur_securite = 0;            // Repart à zéro pour la prochaine détection de blocage
    elapsed           = 0;            // Affichage UART remis à zéro
}



/* ---------------------------------------------------------------------------
 * Motor_Forward — marche avant via TIM1 CH4
 * Séquence : arrêt propre → délai anti-shoot-through → démarrage PWM → chrono
 * --------------------------------------------------------------------------- */

void Motor_Forward(void)
{
    PWM_StopAll(); // Coupe les deux PWM : évite court-circuit pendant la transition

    // Pins de direction à 0 avant d'activer le nouveau sens
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(5); // 5 ms : attend l'extinction complète des transistors du pont en H

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed); // Charge le rapport cyclique
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);   // Lance la PWM marche avant
    Motor_StartTimer(); // Démarre la FSM de sécurité
}



/* ---------------------------------------------------------------------------
 * Motor_Reverse — marche arrière via LPTIM1 CH1
 * Même séquence que Motor_Forward, mais PWM sur LPTIM1.
 * L'ordre de reset des pins est inversé (spécifique au câblage du pont en H).
 * --------------------------------------------------------------------------- */

void Motor_Reverse(void)
{
    PWM_StopAll();

    // Ordre de reset inversé par rapport à Forward : dépend du câblage du pont en H
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_Delay(5); // Anti-shoot-through

    HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1); // Lance la PWM marche arrière
    __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed); // Applique le rapport cyclique
    Motor_StartTimer();
}



/* ---------------------------------------------------------------------------
 * Motor_Stop — arrêt propre + reset état + extinction LEDs
 * Remet motor_state = MOTEUR_OFF → autorise la mise en veille dans le while(1).
 * --------------------------------------------------------------------------- */

void Motor_Stop(void)
{
    PWM_StopAll(); // Coupe les deux canaux PWM

    // Pont en H en roue libre (les deux pins à 0 = pas de freinage actif)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);

    motor_state = MOTEUR_OFF; // Condition de mise en veille dans le while(1)
    threshold   = 1.0f;       // Réinitialise le seuil : sera recalibré au prochain démarrage

    LED_AllOff();  // Éteint les LEDs vitesse : moteur à l'arrêt = pas d'information à afficher
    led_active = 0; // Lève le verrou de mise en veille lié aux LEDs
}



/* ---------------------------------------------------------------------------
 * Motor_SetSpeed — change la vitesse (4 niveaux fixes) + affiche sur LEDs
 * Les LEDs servent de barre de progression : 1 LED = lent, 4 LEDs = rapide.
 * --------------------------------------------------------------------------- */

void Motor_SetSpeed(uint16_t speed)
{
    if (speed > 999)              speed = 999; // Limite haute : période timer = 999
    if (speed > 0 && speed < 700) speed = 700; // Limite basse : en dessous le moteur cale

    current_speed = speed;

    // Mise à jour PWM uniquement si le moteur tourne (évite une sortie fantôme à l'arrêt)
    if (motor_state != MOTEUR_OFF)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);    // Marche avant
        __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed); // Marche arrière
    }



    // Affichage barre de vitesse : N LEDs allumées de gauche à droite = speed_level

    HAL_GPIO_WritePin(LED_G_1_PORT, LED_G_1_PIN, (speed_level >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_G_2_PORT, LED_G_2_PIN, (speed_level >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_3_PORT, LED_D_3_PIN, (speed_level >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_4_PORT, LED_D_4_PIN, (speed_level >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);


    led_timer  = HAL_GetTick(); // Démarre le timeout de 1.5 s avant extinction auto
    led_active = 1;             // Empêche la mise en veille pendant l'affichage
}



/* ---------------------------------------------------------------------------
 * Get_Motor_Current — lecture ADC → conversion en Ampères
 * Formule : I = (ADC/4095 × 3.3 V) / R_shunt × facteur_correction - offset
 * --------------------------------------------------------------------------- */

float Get_Motor_Current(void)
{
    float res_ohm = 1.3f;   // Résistance de shunt mesurée physiquement (Ω)
    float offset  = 0.062f; // Tension résiduelle à courant nul (A), compensée ici

    HAL_ADC_Start(&hadc1); // Déclenche une conversion logicielle unique

    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&hadc1); // Valeur brute 12 bits (0–4095)

        // Conversion : tension ADC → courant, avec correction du gain de l'amplificateur
        courant_moteur = (((adc_value * 3.3f) / 4095.0f) / res_ohm) * 1.103f;
        courant_moteur -= offset; // Soustrait le zéro de l'ampèremètre à vide

        if (courant_moteur < 0) courant_moteur = 0; // Valeur physiquement impossible
    }

    HAL_ADC_Stop(&hadc1); // Libère l'ADC (LowPowerAutoPowerOff l'éteint aussi)
    return courant_moteur;
}



/* ---------------------------------------------------------------------------
 * Update_Moving_Average — filtre glissant FIFO 5 points
 * Lisse le courant pour éviter les fausses détections sur les pics transitoires.
 * --------------------------------------------------------------------------- */

float Update_Moving_Average(float new_sample)
{
    // Décalage FIFO : indice 0 = le plus ancien, 4 = le plus récent
    lecture[0] = lecture[1];
    lecture[1] = lecture[2];
    lecture[2] = lecture[3];
    lecture[3] = lecture[4];
    lecture[4] = new_sample; // Insère le nouvel échantillon en queue

    // Recalcul complet de la somme (5 valeurs, calcul incrémental inutile ici)
    somme_courant = 0;
    for (int i = 0; i < 5; i++) somme_courant += lecture[i];

    return somme_courant / 5.0f;
}



/* ---------------------------------------------------------------------------
 * UART_Send_Status — trame de diagnostic périodique (toutes les 500 ms)
 * Centralise l'envoi pour ne pas disperser les sprintf dans la FSM.
 * --------------------------------------------------------------------------- */

static void UART_Send_Status(void)
{
    int len = sprintf(msg_status,
        "\r\n+--------------------------------------+\r\n"
        "|            ETAT DU SYSTEME           |\r\n"
        "+--------------------------------------+\r\n"
        "| Courant Moy.   : %6.3f A            |\r\n"
        "| Courant Fct.   : %6.3f A            |\r\n"
        "| Seuil (Thresh) : %6.3f A            |\r\n"
        "| Compteur Secu. : %-6d              |\r\n"
        "| Etat Moteur    : %-6d              |\r\n"
        "| Temps Ecoule   : %-8lu ms         |\r\n"
        "| Niveau Vitesse : %-6d              |\r\n"
        "+--------------------------------------+\r\n",
        moyenne_courant, courant_fonctionnement_morteur,
        threshold, compteur_securite, (int)motor_state, elapsed, speed_level);

    // Augmentation légère du timeout (100ms) car le message contient plus de caractères
    HAL_UART_Transmit(&huart2, (uint8_t*)msg_status, len, 200);
}



/* ---------------------------------------------------------------------------
 * Motor_Security_FSM — surveillance courant en 4 phases
 *
 *  DEMARRAGE (0→2 s)   : transitoire ignoré, courant élevé normal
 *  CALIBRATION (2→3 s) : mesure du courant nominal → fixe threshold = nominal × 1.2
 *  MARCHE (>3 s)       : surveillance active
 *    → courant < 0.03 A : moteur absent/déconnecté → ERREUR_ABSENCE
 *    → surcourant × 5   : blocage mécanique → ERREUR_BLOCAGE
 *    → elapsed > 20 s   : fin de course temporelle → arrêt propre
 * --------------------------------------------------------------------------- */

static void Motor_Security_FSM(uint32_t current_time)
{
    elapsed = current_time - motor_start_time; // Durée depuis le dernier Motor_StartTimer()

    if (motor_state == DEMARRAGE_MOTEUR && elapsed > 2000)
    {
        // 2 s écoulées : fin du transitoire de démarrage
        motor_state = CALIBRATION_MOTEUR;
        HAL_UART_Transmit(&huart2, (uint8_t*)"Calibrage...\r\n", 14, 10);
    }
    else if (motor_state == CALIBRATION_MOTEUR && elapsed > 3000)
    {
        // 3 s : courant stabilisé → on le prend comme référence nominale
        courant_fonctionnement_morteur = moyenne_courant;

        // Plancher à 0.05 A : évite un seuil nul si le moteur tourne à vide
        if (courant_fonctionnement_morteur < 0.05f) courant_fonctionnement_morteur = 0.05f;

        threshold   = courant_fonctionnement_morteur * 1.25f; // +25 % de marge
        motor_state = MOTEUR_MARCHE; // Surveillance active

        int len = sprintf(msg_status, "Seuil fixé à: %.2f A\r\n", threshold);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg_status, len, 50);
    }
    else if (motor_state == MOTEUR_MARCHE && elapsed > 20000)
    {
        // 20 s : fin de course temporelle → arrêt propre autorisé
        Motor_Stop();
        HAL_UART_Transmit(&huart2, (uint8_t*)"FIN DE COURSE (TEMPS)\r\n", 23, 50);
        elapsed = 0;
    }
    else if (motor_state == MOTEUR_MARCHE)
    {
        // --- Défaut de présence : moteur déconnecté du circuit ---
        if (moyenne_courant < 0.03f)
        {
            Motor_HardStop(); // Coupe le hardware sans écraser motor_state
            motor_state = MOTEUR_ERREUR_ABSENCE; // Verrouille en état d'erreur (clignotement 3 s)
            HAL_UART_Transmit(&huart2, (uint8_t*)"!!! DEFAUT: MOTEUR DECONNECTE !!!\r\n", 35, 100);
            elapsed = 0;
        }
        // --- Détection de blocage mécanique par surcourant ---
        else if (moyenne_courant > threshold)
        {
            compteur_securite++; // Un seul dépassement n'est pas suffisant (anti-faux positif)

            if (compteur_securite >= 5)
            {
                // 5 dépassements consécutifs = blocage confirmé
                Motor_HardStop(); // Coupe le hardware sans passer par Motor_Stop()
                motor_state = MOTEUR_ERREUR_BLOCAGE; // Verrouille (clignotement 0.5 s)
                HAL_UART_Transmit(&huart2, (uint8_t*)"!!! DEFAUT: BLOCAGE MOTEUR  !!!\r\n", 35, 100);
                elapsed = 0;
            }
        }
        else
        {
            compteur_securite = 0; // Retour sous le seuil : on réinitialise le compteur
        }
    }
}

/* ---------------------------------------------------------------------------
 * Motor_Periodic_Update — tâche périodique 500 ms (moteur en marche)
 * Lit le courant, met à jour la moyenne, exécute la FSM, envoie le diagnostic.
 * --------------------------------------------------------------------------- */
static void Motor_Periodic_Update(void)
{
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_tick >= 1000) // Cadence fixe de 500 ms
    {
        last_tick = current_time; // Référence pour la prochaine fenêtre

        float instant   = Get_Motor_Current();
        moyenne_courant = Update_Moving_Average(instant); // Courant lissé sur 5 mesures

        Motor_Security_FSM(current_time); // Calibration + détection d'anomalie
        UART_Send_Status();               // Envoi trame diagnostic
    }
}

/* ---------------------------------------------------------------------------
 * Process_UART_Command — dispatch commandes UART (non bloquant)
 * rx_pending prioritaire : octet capturé depuis une IT de réveil veille.
 * Polling sinon (timeout = 0 ms = retour immédiat si rien).
 * --------------------------------------------------------------------------- */
static void Process_UART_Command(void)
{
    // Priorité à l'octet de réveil (capturé par IT avant le WFI)
    if (!rx_pending && HAL_UART_Receive(&huart2, &rx_data, 1, 0) != HAL_OK) return;
    rx_pending = 0; // Consomme le flag IT

    HAL_UART_Transmit(&huart2, &rx_data, 1, 10); // Echo : confirme la réception au PC

    switch (rx_data)
    {
        case 'D': Motor_Forward(); break;
        case 'A': Motor_Reverse(); break;
        case 'S': Motor_Stop();    break;

        case '+':
            if (speed_level < 4)
            {
                speed_level++;
                current_speed = speed_table[speed_level - 1]; // Niveau 1-4 → index 0-3
                Motor_SetSpeed(current_speed);
            }
            break;

        case '-':
            if (speed_level > 1)
            {
                speed_level--;
                current_speed = speed_table[speed_level - 1];
                Motor_SetSpeed(current_speed);
            }
            break;
    }

    sprintf(msg_cmd, "\r\nCommande: %c | Niveau Vitesse: %d | Valeur PWM: %d\r\n",
            rx_data, speed_level, current_speed);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg_cmd, strlen(msg_cmd), 100);
}

/* ---------------------------------------------------------------------------
 * HAL_UART_RxCpltCallback — IT UART : flag seulement, jamais de logique ici
 * La logique moteur s'exécute dans Process_UART_Command (contexte tâche).
 * --------------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) rx_pending = 1;
}

/* USER CODE END 0 */

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_LPTIM1_Init();
    MX_ADC1_Init();
    MX_USART2_UART_Init();
    MX_RTC_Init();

    /* USER CODE BEGIN 2 */
    /* USER CODE BEGIN 2 */
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // Pré-démarre TIM1 (requis avant Motor_Forward)
      __HAL_TIM_MOE_ENABLE(&htim1);             // Main Output Enable : active la sortie TIM1

      if (HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1) != HAL_OK) Error_Handler();

      // 🔥 LA CORRECTION EST ICI : On appelle juste la fonction, et c'est TOUT !
      UART_Send_Welcome_Msg();

      /* USER CODE END 2 */
    /* USER CODE BEGIN WHILE */

    while (1)
    {
        // 1. Traitement des commandes UART
        Process_UART_Command();

        // 2. Timeout LEDs vitesse
        if (motor_state != MOTEUR_ERREUR_ABSENCE && motor_state != MOTEUR_ERREUR_BLOCAGE)
        {
            Update_LED_Timeout();
        }

        // 3. Clignotement d'erreur
        Gerer_Erreur_Moteur();

        // 4. Moteur en marche normale → surveillance courant
        if (motor_state != MOTEUR_OFF
         && motor_state != MOTEUR_ERREUR_ABSENCE
         && motor_state != MOTEUR_ERREUR_BLOCAGE)
        {
            // On appelle UNIQUEMENT cette fonction.
            // C'est elle qui gère en interne le rythme des 500 ms pour TOUT le monde.
            Motor_Periodic_Update();
        }

        // 5. Veille uniquement si tout est éteint
        else if (motor_state == MOTEUR_OFF && led_active == 0)
        {
            Enter_Low_Power_Mode();
        }
    }
    /* USER CODE END WHILE */
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.LSEState            = RCC_LSE_ON;
    RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_11; // 48 MHz
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV1;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.LowPowerAutoPowerOff  = ENABLE;  // Éteint l'ADC entre deux conversions (~1 mA économisé)
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.SamplingTimeCommon1   = ADC_SAMPLETIME_160CYCLES_5;
    hadc1.Init.SamplingTimeCommon2   = ADC_SAMPLETIME_160CYCLES_5;
    hadc1.Init.OversamplingMode      = DISABLE;
    hadc1.Init.TriggerFrequencyMode  = ADC_TRIGGER_FREQ_HIGH;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    sConfig.Channel      = ADC_CHANNEL_14; // PA7 : shunt de mesure de courant
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();
}

static void MX_LPTIM1_Init(void)
{
    LPTIM_OC_ConfigTypeDef sConfig1 = {0};
    hlptim1.Instance               = LPTIM1;
    hlptim1.Init.Clock.Source      = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    hlptim1.Init.Clock.Prescaler   = LPTIM_PRESCALER_DIV64; // Divise pour obtenir la fréquence PWM
    hlptim1.Init.Trigger.Source    = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.Period            = 999; // Même période que TIM1 : cohérence du rapport cyclique
    hlptim1.Init.UpdateMode        = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.CounterSource     = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim1.Init.Input1Source      = LPTIM_INPUT1SOURCE_GPIO;
    hlptim1.Init.Input2Source      = LPTIM_INPUT2SOURCE_GPIO;
    hlptim1.Init.RepetitionCounter = 0;
    if (HAL_LPTIM_Init(&hlptim1) != HAL_OK) Error_Handler();

    sConfig1.Pulse      = 0;                    // Rapport cyclique initial à 0 %
    sConfig1.OCPolarity = LPTIM_OCPOLARITY_LOW; // Polarité inversée vs TIM1 (câblage pont en H)
    if (HAL_LPTIM_OC_ConfigChannel(&hlptim1, &sConfig1, LPTIM_CHANNEL_1) != HAL_OK) Error_Handler();
    HAL_LPTIM_MspPostInit(&hlptim1); // Reconnecte la broche GPIO à la sortie LPTIM1
}

static void MX_RTC_Init(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv   = 127;  // Prédiviseur asynch : (127+1) × (255+1) = 32768 Hz (LSE)
    hrtc.Init.SynchPrediv    = 255;  // Prédiviseur synch : donne 1 Hz pour le compteur RTC
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutPullUp   = RTC_OUTPUT_PULLUP_NONE;
    hrtc.Init.BinMode        = RTC_BINARY_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) Error_Handler();

    sTime.Hours = 0x0; sTime.Minutes = 0x0; sTime.Seconds = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) Error_Handler();

    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month   = RTC_MONTH_JANUARY;
    sDate.Date    = 0x1;
    sDate.Year    = 0x0;
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) Error_Handler();
}

static void MX_TIM1_Init(void)
{
    TIM_MasterConfigTypeDef        sMasterConfig        = {0};
    TIM_OC_InitTypeDef             sConfigOC            = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 63;   // Divise l'horloge APB pour la fréquence PWM cible
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 999;  // Période = 1000 pas → rapport cyclique sur [0, 999]
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) Error_Handler();

    sMasterConfig.MasterOutputTrigger  = TIM_TRGO_RESET;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) Error_Handler();

    sConfigOC.OCMode       = TIM_OCMODE_PWM1; // Actif tant que CNT < CCR
    sConfigOC.Pulse        = 0;               // Rapport cyclique initial 0 %
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) Error_Handler();

    // Break/Dead-time tout désactivé : le pont en H est géré par GPIO, pas par le break hardware
    sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime         = 0;
    sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter      = 0;
    sBreakDeadTimeConfig.BreakAFMode      = TIM_BREAK_AFMODE_INPUT;
    sBreakDeadTimeConfig.Break2State      = TIM_BREAK2_DISABLE;
    sBreakDeadTimeConfig.Break2Polarity   = TIM_BREAK2POLARITY_HIGH;
    sBreakDeadTimeConfig.Break2Filter     = 0;
    sBreakDeadTimeConfig.Break2AFMode     = TIM_BREAK_AFMODE_INPUT;
    sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) Error_Handler();
    HAL_TIM_MspPostInit(&htim1); // Reconnecte PA11 à la sortie TIM1 CH4
}

static void MX_USART2_UART_Init(void)
{
    huart2.Instance                    = USART2;
    huart2.Init.BaudRate               = 115200;
    huart2.Init.WordLength             = UART_WORDLENGTH_8B;
    huart2.Init.StopBits               = UART_STOPBITS_1;
    huart2.Init.Parity                 = UART_PARITY_NONE;
    huart2.Init.Mode                   = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling           = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Activation obligatoire des horloges GPIO avant tout accès registre
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // I2C1 : open-drain alternate function (géré par CubeMX, conservé pour l'équipe)
    GPIO_InitStruct.Pin       = I2C1_SCL_Pin | I2C1_SDA_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
    // Pins de commande moteur : sortie push-pull (signaux DC, vitesse faible suffisante)
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = MOTEUR_NSLEEP_PIN | MOTEUR_ARRIERE_PIN; // PA9, PA10
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = MOTEUR_AVANT_PIN; // PB1
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Pins LED : même mode sortie PP
    GPIO_InitStruct.Pin = LED_G_1_PIN | LED_G_2_PIN | LED_D_3_PIN | LED_D_4_PIN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // État initial : driver moteur en veille, LEDs éteintes
    HAL_GPIO_WritePin(GPIOA, MOTEUR_NSLEEP_PIN | MOTEUR_ARRIERE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, MOTEUR_AVANT_PIN | LED_G_1_PIN | LED_G_2_PIN
                            | LED_D_3_PIN | LED_D_4_PIN, GPIO_PIN_RESET);
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* ---------------------------------------------------------------------------
 * Gerer_Erreur_Moteur — clignotement non-bloquant selon le type d'erreur
 *
 * ERREUR_ABSENCE  → toutes les LEDs clignotent toutes les 3 s (défaut discret)
 * ERREUR_BLOCAGE  → toutes les LEDs clignotent toutes les 0.5 s (alarme rapide)
 *
 * Non-bloquant : utilise une variable statique + HAL_GetTick (pas de HAL_Delay).
 * L'état est verrouillé : seule une commande 'S' permet de sortir de l'erreur.
 * --------------------------------------------------------------------------- */
void Gerer_Erreur_Moteur(void)
{
    static uint32_t last_blink_tick = 0; // Statique : persiste entre deux appels
    uint32_t intervalle = 0;

    // Sélection de la période de clignotement selon l'erreur détectée
    if      (motor_state == MOTEUR_ERREUR_ABSENCE) intervalle = 3000; // Lent : déconnexion
    else if (motor_state == MOTEUR_ERREUR_BLOCAGE) intervalle = 500;  // Rapide : blocage

    if (intervalle > 0 && (HAL_GetTick() - last_blink_tick >= intervalle))
    {
        last_blink_tick = HAL_GetTick(); // Mémorise pour la prochaine période
        LED_ToggleAll(); // Inverse l'état des 4 LEDs simultanément
    }
}

/* ---------------------------------------------------------------------------
 * Update_LED_Timeout — extinction automatique des LEDs vitesse après 1.5 s
 * Évite de laisser la barre de vitesse allumée indéfiniment.
 * Non appelée en état d'erreur : le clignotement d'erreur ne doit pas être coupé.
 * --------------------------------------------------------------------------- */
void Update_LED_Timeout(void)
{
    if (led_active && (HAL_GetTick() - led_timer >= 1500))
    {
        LED_AllOff();  // Éteint les 4 LEDs
        led_active = 0; // Lève le verrou de mise en veille
    }
}

/* ---------------------------------------------------------------------------
 * Clock_SwitchToSleep — réduit MSI 48 MHz → 4 MHz + SCALE2 avant veille
 * Séquence obligatoire : flash latency → fréquence → tension → BRR UART
 * HAL_UART_Init() recalcule le BRR pour 115200 baud @ 4 MHz automatiquement.
 * DOIT être appelé AVANT HAL_UART_Receive_IT (sinon le HAL_UART_Init annule l'IT).
 * --------------------------------------------------------------------------- */
static void Clock_SwitchToSleep(void)
{
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0); // 0 wait state suffisant à 4 MHz

    RCC_OscInitTypeDef osc      = {0};
    osc.OscillatorType          = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState                = RCC_MSI_ON;
    osc.MSICalibrationValue     = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange           = RCC_MSIRANGE_6; // 4 MHz
    osc.PLL.PLLState            = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    // SCALE2 (Vcore 1.0 V) : seulement APRÈS la réduction de fréquence
    // Inverser l'ordre = instabilité CPU à haute fréquence sous-alimenté
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

    HAL_UART_Init(&huart2); // Recalcule le BRR pour 115200 baud @ 4 MHz
}

/* ---------------------------------------------------------------------------
 * Clock_SwitchToFullSpeed — restaure MSI 4 MHz → 48 MHz + SCALE1 au réveil
 * Séquence obligatoire : tension → fréquence → flash latency → BRR UART
 * --------------------------------------------------------------------------- */
static void Clock_SwitchToFullSpeed(void)
{
    // SCALE1 (Vcore 1.2 V) obligatoire AVANT de repasser au-dessus de 26 MHz
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitTypeDef osc  = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState            = RCC_MSI_ON;
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange       = RCC_MSIRANGE_11; // 48 MHz
    osc.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1); // Restore latence pour 48 MHz

    HAL_UART_Init(&huart2); // Recalcule le BRR pour 115200 baud @ 48 MHz
}

/* ---------------------------------------------------------------------------
 * Enter_Low_Power_Mode — veille SLEEP avec réveil par UART
 *
 * Séquence :
 *   1. Purge UART → évite réveil immédiat sur flag résiduel
 *   2. GPIO → analogique → supprime courants de fuite
 *   3. Clock gating TIM1/LPTIM1/ADC
 *   4. Clock_SwitchToSleep() EN PREMIER → HAL_UART_Init recalcule le BRR
 *      ⚠ Doit précéder HAL_UART_Receive_IT : sinon l'Init annule l'IT
 *   5. HAL_UART_Receive_IT → capture l'octet de réveil dans rx_data
 *   6. SysTick off → WFI
 *   ---- RÉVEIL ----
 *   7. Clock_SwitchToFullSpeed() → BRR restauré pour 48 MHz
 *   8. Périphériques restaurés
 * --------------------------------------------------------------------------- */
void Enter_Low_Power_Mode(void)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"Entree en veille...\r\n", 21, 50);
    HAL_Delay(10); // Laisse l'UART vider son buffer TX avant la coupure

    /* 1. Purge UART : supprime les flags résiduels pour éviter un faux réveil */
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
    volatile uint32_t tmpreg = huart2.Instance->RDR; // Vide le registre de données
    (void)tmpreg;                                     // Évite le warning "unused variable"

    /* 2. GPIO en analogique : supprime les courants de fuite (économie de courant) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // PA2 (TX) et PA3 (RX) conservés → UART physiquement connecté
    // PA13/PA14 (SWD) conservés → débogage toujours possible
    GPIO_InitStruct.Pin = GPIO_PIN_ALL & ~(GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_13 | GPIO_PIN_14);
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_ALL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); // Tout GPIOB en analogique (moteur + LEDs inutiles)
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 3. Clock gating : coupe les périphériques inutiles en veille */
    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_LPTIM1_CLK_DISABLE();
    __HAL_RCC_ADC_CLK_DISABLE();

    /* 4. Réduction clock EN PREMIER : HAL_UART_Init() recalcule le BRR @ 4 MHz
     *    Règle absolue : Clock_SwitchToSleep() AVANT HAL_UART_Receive_IT
     *    Sinon HAL_UART_Init() à l'intérieur effacerait RXNEIE → pas de réveil */
    Clock_SwitchToSleep();

    /* 5. Setup IT de réveil APRÈS le switch d'horloge
     *    HAL stocke l'octet dans rx_data → HAL_UART_RxCpltCallback → rx_pending = 1 */
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);

    /* 6. SysTick off : sinon réveil toutes les 1 ms (SysTick continue en SLEEP) */
    HAL_SuspendTick();

    // CPU s'arrête ici — USART2 surveille le bus à 115200 baud @ 4 MHz
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* ======= RÉVEIL ICI ======= */

    HAL_ResumeTick(); // Relance le SysTick (HAL_GetTick() redevient fiable)

    /* 7. Restaure 48 MHz AVANT de relancer TIM1/LPTIM1 (configurés pour 48 MHz) */
    Clock_SwitchToFullSpeed();

    /* 8. Réactive les horloges périphériques */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    __HAL_RCC_ADC_CLK_ENABLE();

    /* 9. Restaure GPIO + périphériques (perdus pendant la veille) */
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_LPTIM1_Init();
    MX_ADC1_Init();

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // Relance TIM1 (requis avant Motor_Forward)
    __HAL_TIM_MOE_ENABLE(&htim1);             // Réactive la sortie principale de TIM1

    // IT désactivée automatiquement par HAL après réception → polling reprend
    HAL_UART_Transmit(&huart2, (uint8_t*)"Reveil OK !\r\n", 13, 50);
}

/* USER CODE END 4 */

void Error_Handler(void)
{
    __disable_irq(); // Coupe toutes les interruptions pour figer le système
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
