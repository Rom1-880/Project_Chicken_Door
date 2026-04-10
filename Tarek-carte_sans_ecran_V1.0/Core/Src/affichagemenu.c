/*
 * affichagemenu.c
 *
 *  Created on: 8 mai 2019
 *      Author: Joel
 */

#ifndef AFFICHAGEMENU_C_
#define AFFICHAGEMENU_C_

#include "bibliotheque.h"

#include <color.h>
//#include <time.h>

//#include "pile.h"


/*
#include <graphics.h>
#include <typedefs.h>
#include <lcd.h>
*/
// les menus affichés vont dépendre de la langue choisie!!!

/*
 * remplac� par une variable tm de type time_t
extern char annee;      // annee variant de 0 à 255 correspondant à 2000 à 2255
extern char mois;        // mois variant de 1 à 12 !!! peur être de 0 à 11 pour correspondre à l'ephemeride!!!
extern char jour;
extern signed char heure;     // de 0 à 23
extern signed char min;         // de 0 à 59
*/

extern time_t temps;
extern struct tm tm_temps;

extern signed char UTC;
extern signed char absUTC;

extern long tensionpile;

extern char langue;             // choix de la langue;

char chaine[6]={0,0,':',0,0,0};     // chaine contenant la valeur de l'heure dans la partie affichage ecran8
char chainep[6]={0,0,':',0,0,0};    // chaine contenant la valeur de l'heure precedente dans la partie affichage ecran8 afin de pouvoir l'effacer
char eff=1;                         // variable permettant de savoir si on doit effacer l'heure
                                    // (en fait sinon, on efface pendant 1 seconde entiere => clignotement genant!)

extern char modeO;               // heure fixe : 1, capteur Jour nuit : 2, GPS : 3
extern char modeF;               // heure fixe : 1, capteur Jour nuit : 2, GPS : 3

extern signed char heureO;     // de 0 à 23
extern signed char minO;         // de 0 à 59
extern signed char heureF;     // de 0 à 23
extern signed char minF;         // de 0 à 59

extern char signed minretard;        // de 0 à 90min;

extern signed char latitude;        // france   // comprise entre -90 et 90° !!! Pb d'hémisphère!!!
extern int longitude;               // france   // comprise entre -180 et +180� !!!


void affichecarre(char choix)
{
    char j;
    for (j=1;j<6;j++)                                   // effacement de tous les carrés rouges potentiels
    {
    setColor(COLOR_16_BLACK);
    if (j!=choix)
        {
//        setColor(COLOR_16_BLACK);
        drawRect(2, (26 + (j-1)*20), 120, (44+ (j-1)*20)); // (x, y, Longueur du rectangle, yfin)
        }
    }
    setColor(COLOR_16_RED);
    drawRect(2, (26 + (choix-1)*20), 120, (44+ (choix-1)*20)); // (x, y, Longueur du rectangle, yfin)
}

void effaceligne(void)
{
    setColor(COLOR_16_BLACK);
//    drawString(45, 50, FONT_MD, chaine);
    fillRect(44, 49, 120, 71);              // void fillRect(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);
}

void effaceligne2(void)
{
    setColor(COLOR_16_BLACK);
//    drawString(45, 50, FONT_MD, chaine);
    fillRect(4, 69, 120, 91);              // void fillRect(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);
}

void effaceligne3(void)
{
    setColor(COLOR_16_BLACK);
//    drawString(45, 50, FONT_MD, chaine);
    fillRect(4, 109, 120, 131);              // void fillRect(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);
}

void afficheecran1(void)   // langue
{
        setColor(COLOR_16_WHITE);
        if (langue==1) {drawString(40, 10, FONT_MD, "LANGUAGE");}
        if (langue==2) {drawString(40, 10, FONT_MD, "LANGUE");}
        if (langue==3) {drawString(40, 10, FONT_MD, "SPRACHE");}
        if (langue==4) {drawString(40, 10, FONT_MD, "LENGUA");}
        if (langue==5) {drawString(40, 10, FONT_MD, "LINGUA");}


        setColor(COLOR_16_WHITE);
        drawString(5, 30, FONT_MD, "ENGLISH");

        setColor(COLOR_16_WHITE);
        drawString(5, 50, FONT_MD, "FRANCAIS");

        setColor(COLOR_16_WHITE);
        drawString(5, 70, FONT_MD, "DEUTSCH");

        setColor(COLOR_16_WHITE);
        drawString(5, 90, FONT_MD, "ESPANOL");

        setColor(COLOR_16_WHITE);
        drawString(5, 110, FONT_MD, "ITALIANO");
/*
        setColor(COLOR_16_RED);
        drawRect(4, 25, 120, 45); // (x, y, Longueur du rectangle, yfin)
*/
}

void afficheecran2(void)   // réglages
{
    /*
    setColor(COLOR_16_ORANGE);
    drawRect(10,14,30,26);
    fillRect(30,18,32,22);
    setColor(COLOR_16_BLACK);
    fillRect(12,16,16,24);
    fillRect(18,16,22,24);
    fillRect(24,16,28,24);
    _delay_cycles(6400000);

    // ma version de graphique pile 1
    setColor(COLOR_16_ORANGE);
    drawRect(10,14,30,26);
    fillRect(30,18,32,22);
    fillRect(12,16,16,24);
    _delay_cycles(6400000);

    // ma version de graphique pile 2
    setColor(COLOR_16_ORANGE);
    drawRect(10,14,30,26);
    fillRect(30,18,32,22);
    fillRect(12,16,16,24);
    fillRect(18,16,22,24);
    _delay_cycles(6400000);

    // ma version de graphique pile 3
    setColor(COLOR_16_ORANGE);
    drawRect(10,14,30,26);
    fillRect(30,18,32,22);
    fillRect(12,16,16,24);
    fillRect(18,16,22,24);
    fillRect(24,16,28,24);
    _delay_cycles(6400000);
    */
    setColor(COLOR_16_WHITE);
    drawString(30, 10, FONT_MD, "MENU");

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 30, FONT_MD, "LANGUAGE");}
    if (langue==2) {drawString(5, 30, FONT_MD, "LANGUE");}
    if (langue==3) {drawString(5, 30, FONT_MD, "SPRACHE");}
    if (langue==4) {drawString(5, 30, FONT_MD, "LENGUA");}
    if (langue==5) {drawString(5, 30, FONT_MD, "LINGUA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 50, FONT_MD, "DATE HOUR");}
    if (langue==2) {drawString(5, 50, FONT_MD, "DATE HEURE");}
    if (langue==3) {drawString(5, 50, FONT_MD, "DATUM ZEIT");}
    if (langue==4) {drawString(5, 50, FONT_MD, "FECHA HORA");}
    if (langue==5) {drawString(5, 50, FONT_MD, "DATA ORA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 70, FONT_MD, "OPENING MODE");}
    if (langue==2) {drawString(5, 70, FONT_MD, "MODE OUVERTURE");}
    if (langue==3) {drawString(5, 70, FONT_MD, "OFFNUNG MODUS");}
    if (langue==4) {drawString(5, 70, FONT_MD, "MODO APERTURA");}
    if (langue==5) {drawString(5, 70, FONT_MD, "MODO APERTURA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 90, FONT_MD, "CLOSE MODE");}
    if (langue==2) {drawString(5, 90, FONT_MD, "MODE FERMETURE");}
    if (langue==3) {drawString(5, 90, FONT_MD, "CLOSE MODUS");}
    if (langue==4) {drawString(5, 90, FONT_MD, "MODO CIERRE");}
    if (langue==5) {drawString(5, 90, FONT_MD, "MODO CHIUSURA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 110, FONT_MD, "GPS LOCATION");}
    if (langue==2) {drawString(5, 110, FONT_MD, "POSITION GPS");}
    if (langue==3) {drawString(5, 110, FONT_MD, "GPS-POSITION");}
    if (langue==4) {drawString(5, 110, FONT_MD, "UBICACION GPS");}
    if (langue==5) {drawString(5, 110, FONT_MD, "POSITIONE GPS");}
/*
    setColor(COLOR_16_RED);
    drawRect(4, 25, 120, 45); // (x, y, Longueur du rectangle, yfin)
    */

}


void afficheecran3(void)   // réglage année
{
    char chaine[5]={'2','0',0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(15, 10, FONT_MD, "YEAR SET");}
    if (langue==2) {drawString(15, 10, FONT_MD, "REGLAGE ANNEE");}
    if (langue==3) {drawString(15, 10, FONT_MD, "JAHR SET");}
    if (langue==4) {drawString(15, 10, FONT_MD, "AJUSTE ANOS");}
    if (langue==5) {drawString(15, 10, FONT_MD, "ANNO SET");}

    chaine[2]=(tm_temps.tm_year-30)/10+0x30;
    chaine[3]=(tm_temps.tm_year-30)%10+0x30;;

    setColor(COLOR_16_RED);
    drawString(45, 50, FONT_MD, chaine);
}

void afficheecran4(void)   // réglage mois
{
    char chaine[5]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(15, 10, FONT_MD, "MONTH SET");}
    if (langue==2) {drawString(15, 10, FONT_MD, "REGLAGE MOIS");}
    if (langue==3) {drawString(15, 10, FONT_MD, "MONAT SET");}
    if (langue==4) {drawString(15, 10, FONT_MD, "AJUSTE MES");}
    if (langue==5) {drawString(15, 10, FONT_MD, "MESE SET");}

    chaine[0]=(tm_temps.tm_mon+1)/10+0x30;              //  rq mon varie de 0 � 11 au lieu de 1 � 12!!!
    chaine[1]=(tm_temps.tm_mon+1)%10+0x30;;             // chaine[01] varie de 1 � 12

    setColor(COLOR_16_RED);
    drawString(45, 50, FONT_MD, chaine);
}

void afficheecran5(void)   // réglage jour
{
    char chaine[5]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(15, 10, FONT_MD, "DAY SET");}
    if (langue==2) {drawString(15, 10, FONT_MD, "REGLAGE JOUR");}
    if (langue==3) {drawString(15, 10, FONT_MD, "TAG SET");}
    if (langue==4) {drawString(15, 10, FONT_MD, "AJUSTE DIA");}
    if (langue==5) {drawString(15, 10, FONT_MD, "GIORNO SET");}

    chaine[0]=tm_temps.tm_mday/10+0x30;
    chaine[1]=tm_temps.tm_mday%10+0x30;;

    setColor(COLOR_16_RED);
    drawString(45, 50, FONT_MD, chaine);
}

void afficheecran6(void)   // réglage heure
{
    char chaine[5]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(15, 10, FONT_MD, "HOUR SET");}
    if (langue==2) {drawString(15, 10, FONT_MD, "REGLAGE HEURE");}
    if (langue==3) {drawString(15, 10, FONT_MD, "ZEIT SET");}
    if (langue==4) {drawString(15, 10, FONT_MD, "AJUSTE HORA");}
    if (langue==5) {drawString(15, 10, FONT_MD, "ORA SET");}

    chaine[0]=tm_temps.tm_hour/10+0x30;
    chaine[1]=tm_temps.tm_hour%10+0x30;;

    setColor(COLOR_16_RED);
    drawString(45, 50, FONT_MD, chaine);
}

void afficheecran7(void)   // réglage min
{
    char chaine[5]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(15, 10, FONT_MD, "MIN SET");}
    if (langue==2) {drawString(15, 10, FONT_MD, "REGLAGE MIN");}
    if (langue==3) {drawString(15, 10, FONT_MD, "MINUTE SET");}
    if (langue==4) {drawString(15, 10, FONT_MD, "AJUSTE MINUTO");}
    if (langue==5) {drawString(15, 10, FONT_MD, "MINUTO SET");}

    chaine[0]=tm_temps.tm_min/10+0x30;
    chaine[1]=tm_temps.tm_min%10+0x30;;

    setColor(COLOR_16_RED);
    drawString(45, 50, FONT_MD, chaine);
}

void afficheecran8(void)   // Ecran principal (après un appui de 5s sur le BP ON)
{
    /*
     *  Ecran de dimension foireuse!!
     */

/*    char chaine[6]={0,0,':',0,0,0};           // Pb, me r�initialise les variables � chaque fois
    char chainep[6]={0,0,':',0,0,0};
*/
/*
    if ((tm_temps.tm_sec==0)&(eff==1))                     // si changement de minutes (seconde =0 et pas encore effac�)=> effacement de l'ancienne heure affich�e
        {
        setColor(COLOR_16_BLACK);
        drawString(45, 40, FONT_MD, chainep);
        eff=0;
        }
    if (tm_temps.tm_sec==2)
        {
        eff=1;
        }
*/
    setColor(COLOR_16_WHITE);

    if (langue==1)
    {
    drawString(5, 10, FONT_SM, "[  ] to close or ");     // modification de font.h pour transformer '[' en '↓' et ']' en '↑'
    drawString(5, 20, FONT_SM, "open manually");         // en taille SM
    }
    if (langue==2)
    {
    drawString(5, 10, FONT_SM, "[  ] pour fermer ou");     // modification de font.h pour transformer '[' en '↓' et ']' en '↑'
    drawString(5, 20, FONT_SM, "ouvrir manuellement");         // en taille SM
    }
    if (langue==3)
    {
    drawString(5, 10, FONT_SM, "[ ] manuell zu offnen");     // modification de font.h pour transformer '[' en '↓' et ']' en '↑'
    drawString(5, 20, FONT_SM, "oder zu schlie�en");         // en taille SM
    }
    if (langue==4)
    {
    drawString(5, 10, FONT_SM, "[  ] para cerrar o");     // modification de font.h pour transformer '[' en '↓' et ']' en '↑'
    drawString(5, 20, FONT_SM, "abrir manualmente");         // en taille SM
    }
    if (langue==5)
    {
    drawString(5, 10, FONT_SM, "[  ] per chiudere o");     // modification de font.h pour transformer '[' en '↓' et ']' en '↑'
    drawString(5, 20, FONT_SM, "aprire manualmente");         // en taille SM
    }
        /*
        * affichage de l'heure
        */

    chaine[0]=tm_temps.tm_hour/10+0x30;
    chaine[1]=tm_temps.tm_hour%10+0x30;
    chaine[3]=tm_temps.tm_min/10+0x30;
    chaine[4]=tm_temps.tm_min%10+0x30;

    if ((tm_temps.tm_sec==0)&(eff==1))                     // si changement de minutes (seconde =0 et pas encore effac�)=> effacement de l'ancienne heure affich�e
        {
        setColor(COLOR_16_BLACK);
        drawString(45, 40, FONT_MD, chainep);
        eff=0;
        }
    if (tm_temps.tm_sec==2)
        {
        eff=1;
        }

    drawString(45, 40, FONT_MD, chaine);

    chainep[0]=chaine[0];       // m�morisation de la derni�re chaine affich�e
    chainep[1]=chaine[1];
    chainep[3]=chaine[3];
    chainep[4]=chaine[4];


    /*
     * Inserer l'affichage de la pile à coté de l'heure!
     */

 /*   on mesure Vpile * 68/168 (pont diviseur) soit environ 2.43V quand on met 6V en entr�e.
  *              pile pleine:    1.5 -> 1.3    seuil :         643     ->  (1.3*4*68/168)/q
  *              pile 3/4 :      1.3 -> 1.18   seuil :  644 et 594
  *              pile 1/2 :      1.18 -> 1.1   seuil :  593 et 544
  *              pile vide :     1.1 -> 0      seuil :  544                 avec q=3.2227.10-3    convertisseur 10 bits
 */


  if (tensionpile<545)                 // vrai seuils
 //   if (tensionpile<600)                 // seuil simul�s par ma LDR
        {
         setColor(COLOR_16_RED);        // pile vide en rouge
         drawRect(10,40,30,52);
         fillRect(30,44,32,48);
         setColor(COLOR_16_BLACK);
         fillRect(12,42,16,50);
         fillRect(18,42,22,50);
         fillRect(24,42,28,50);
        }
   else if ((tensionpile>544)&&(tensionpile<594))                 // vrai seuils
//   else if ((tensionpile>600)&&(tensionpile<800))                   // seuil simul�s par ma LDR
       {
       setColor(COLOR_16_WHITE);        // pile 1 barre
       drawRect(10,40,30,52);
       fillRect(30,44,32,48);
       fillRect(12,42,16,50);
       setColor(COLOR_16_BLACK);
       fillRect(18,42,22,50);
       fillRect(24,42,28,50);
       }
   else if ((tensionpile>593)&&(tensionpile<644))                 // vrai seuils
//   else if ((tensionpile>800)&&(tensionpile<1000))                   // seuil simul�s par ma LDR
         {
         setColor(COLOR_16_WHITE);        // pile 2 barres
         drawRect(10,40,30,52);
         fillRect(30,44,32,48);
         fillRect(12,42,16,50);
         fillRect(18,42,22,50);
         setColor(COLOR_16_BLACK);
         fillRect(24,42,28,50);
         }
   else if (tensionpile>643)                 // vrai seuils
//   else if (tensionpile>1000)                   // seuil simul�s par ma LDR
         {
         setColor(COLOR_16_WHITE);        // pile 3 barres
         drawRect(10,40,30,52);
         fillRect(30,44,32,48);
         fillRect(12,42,16,50);
         fillRect(18,42,22,50);
         fillRect(24,42,28,50);
         //setColor(COLOR_16_BLACK);    // inutile, la pile est pleine
         }



   /*
   * Inserer l'affichage de la pile à coté de l'heure!
   */




    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 60, FONT_MD, "Opening Mode");}
    if (langue==2) {drawString(5, 60, FONT_MD, "Mode Ouverture");}
    if (langue==3) {drawString(5, 60, FONT_MD, "Offnung Modus");}
    if (langue==4) {drawString(5, 60, FONT_MD, "Modo Apertura");}
    if (langue==5) {drawString(5, 60, FONT_MD, "Modo Apertura");}

    setColor(COLOR_16_WHITE);
    if (modeO==1)
        {
        chaine[0]=heureO/10+0x30;
        chaine[1]=heureO%10+0x30;
        chaine[3]=minO/10+0x30;
        chaine[4]=minO%10+0x30;
        drawString(5, 75, FONT_SM, chaine);
        }
    if (modeO==2)
            {
            if (langue==1) {drawString(5, 75, FONT_SM, "Day Night Sensor");}
            if (langue==2) {drawString(5, 75, FONT_SM, "Capteur Jour Nuit");}
            if (langue==3) {drawString(5, 75, FONT_SM, "Tag-Nacht-Sensor");}
            if (langue==4) {drawString(5, 75, FONT_SM, "Sensor Noche Dia");}
            if (langue==5) {drawString(5, 75, FONT_SM, "Sensore Notte Giorno");}
            }

    if (modeO==3)
            {
            if (langue==1) {drawString(5, 75, FONT_SM, "GPS location");}
            if (langue==2) {drawString(5, 75, FONT_SM, "Position GPS");}
            if (langue==3) {drawString(5, 75, FONT_SM, "GPS position");}
            if (langue==4) {drawString(5, 75, FONT_SM, "Ubicacion GPS");}
            if (langue==5) {drawString(5, 75, FONT_SM, "Positione GPS");}
            }

    setColor(COLOR_16_WHITE);

    if (langue==1) {drawString(5, 95, FONT_MD, "Close Mode");}
    if (langue==2) {drawString(5, 95, FONT_MD, "Mode Fermeture");}
    if (langue==3) {drawString(5, 95, FONT_MD, "Close Modus");}
    if (langue==4) {drawString(5, 95, FONT_MD, "Modo Cierre");}
    if (langue==5) {drawString(5, 95, FONT_MD, "Modo Chiusura");}

    setColor(COLOR_16_WHITE);
    if (modeF==1)
        {
        chaine[0]=heureF/10+0x30;
        chaine[1]=heureF%10+0x30;
        chaine[3]=minF/10+0x30;
        chaine[4]=minF%10+0x30;
        drawString(5, 110, FONT_SM, chaine);
        }
    if (modeF==2)
        {
        if (langue==1) {drawString(5, 110, FONT_SM, "Day Night Sensor");}
        if (langue==2) {drawString(5, 110, FONT_SM, "Capteur Jour Nuit");}
        if (langue==3) {drawString(5, 110, FONT_SM, "Tag-Nacht-Sensor");}
        if (langue==4) {drawString(5, 110, FONT_SM, "Sensor Noche Dia");}
        if (langue==5) {drawString(5, 110, FONT_SM, "Sensore Notte Giorno");}
        }
    if (modeF==3)
        {
        if (langue==1) {drawString(5, 110, FONT_SM, "GPS location");}
        if (langue==2) {drawString(5, 110, FONT_SM, "Position GPS");}
        if (langue==3) {drawString(5, 110, FONT_SM, "GPS position");}
        if (langue==4) {drawString(5, 110, FONT_SM, "Ubicacion GPS");}
        if (langue==5) {drawString(5, 110, FONT_SM, "Positione GPS");}
        }
}

void afficheecran9(void)   // réglage GPS
{
    char chaine[5]={0,0,0,0,0};
    extern char choixi;                   // permet le choix de r�glage entre lattitude et longitude

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(15, 10, FONT_MD, "GPS LOCATION");}
    if (langue==2) {drawString(15, 10, FONT_MD, "POSITION GPS");}
    if (langue==3) {drawString(15, 10, FONT_MD, "GPS POSITION");}
    if (langue==4) {drawString(15, 10, FONT_MD, "UBICACION GPS");}
    if (langue==5) {drawString(15, 10, FONT_MD, "POSITIONE GPS");}

    drawString(5, 25, FONT_SM, "PARIS: 48.8534 2.3288");

    if (langue==1) {drawString(5, 35, FONT_SM, "Enter +48 et +002");}
    if (langue==2) {drawString(5, 35, FONT_SM, "Entrer +48 et +002");}
    if (langue==3) {drawString(5, 35, FONT_SM, "Eingeben +48 et +002");}
    if (langue==4) {drawString(5, 35, FONT_SM, "Entrar +48 et +002");}
    if (langue==5) {drawString(5, 35, FONT_SM, "Entrez +48 et +002");}

    if (langue==1) {drawString(5, 50, FONT_MD, "FIRST DIGIT");}
    if (langue==2) {drawString(5, 50, FONT_MD, "PREMIER CHIFFRE");}
    if (langue==3) {drawString(5, 50, FONT_MD, "ERSTE ZIFFER");}
    if (langue==4) {drawString(5, 50, FONT_MD, "PRIMER DIGITO");}
    if (langue==5) {drawString(5, 50, FONT_MD, "PRIMA CIFRA");}

    if (choixi==1)
        {
        setColor(COLOR_16_RED);
        }
    else  setColor(COLOR_16_WHITE);
    if (latitude>=0)
                {
                chaine[0]='+';
                chaine[1]=latitude/10+0x30;
                chaine[2]=latitude%10+0x30;
                }
            else
                {
                chaine[0]='-';
                chaine[1]=(-latitude)/10+0x30;
                chaine[2]=(-latitude)%10+0x30;
                }
    drawString(45, 70, FONT_MD,chaine);

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 90, FONT_MD, "SECOND DIGIT");}
    if (langue==2) {drawString(5, 90, FONT_MD, "SECOND CHIFFRE");}
    if (langue==3) {drawString(5, 90, FONT_MD, "ZWEITE ZIFFER");}
    if (langue==4) {drawString(5, 90, FONT_MD, "SEGUNDO DIGITO");}
    if (langue==5) {drawString(5, 90, FONT_MD, "SECONDA CIFRA");}

    if (choixi==2)
        {
        setColor(COLOR_16_RED);
        }
    else  setColor(COLOR_16_WHITE);
    if (longitude>=0)
                {
                chaine[0]='+';
                chaine[1]=longitude/100+0x30;
                chaine[2]=(longitude%100)/10+0x30;
                chaine[3]=longitude%10+0x30;
                }
            else
                {
                chaine[0]='-';
                chaine[1]=(-longitude)/100+0x30;
                chaine[2]=((-longitude)%100)/10+0x30;
                chaine[3]=(-longitude)%10+0x30;
                }
    drawString(45, 110, FONT_MD, chaine);
}

void afficheecran10(void)  // choix mode ouverture
{
    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(10, 10, FONT_MD, "OPENING MODE");}
    if (langue==2) {drawString(10, 10, FONT_MD, "MODE OUVERTURE");}
    if (langue==3) {drawString(10, 10, FONT_MD, "OFFNUNG MODUS");}
    if (langue==4) {drawString(10, 10, FONT_MD, "MODO APERTURA");}
    if (langue==5) {drawString(10, 10, FONT_MD, "MODO APERTURA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 30, FONT_MD, "FIXED TIME");}
    if (langue==2) {drawString(5, 30, FONT_MD, "HEURES FIXES");}
    if (langue==3) {drawString(5, 30, FONT_MD, "FESTE ZEIT");}
    if (langue==4) {drawString(5, 30, FONT_MD, "HORA FIJA");}
    if (langue==5) {drawString(5, 30, FONT_MD, "ORA FISSA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 50, FONT_MD, "DAY NIGHT Sensor");}
    if (langue==2) {drawString(5, 50, FONT_MD, "Capt JOUR NUIT");}
    if (langue==3) {drawString(5, 50, FONT_MD, "TAG-NACHT-Sensor");}
    if (langue==4) {drawString(5, 50, FONT_MD, "Sensor NOCHE DIA");}
    if (langue==5) {drawString(5, 50, FONT_MD, "NOTTE GIORNO");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 70, FONT_MD, "GPS LOCATION");}
    if (langue==2) {drawString(5, 70, FONT_MD, "POSITION GPS");}
    if (langue==3) {drawString(5, 70, FONT_MD, "GPS POSITION ");}
    if (langue==4) {drawString(5, 70, FONT_MD, "UBICACION GPS");}
    if (langue==5) {drawString(5, 70, FONT_MD, "POSITIONE GPS");}

}

void afficheecran11(void)  // Reglage heure mode ouverture fixe
{
    char chaineh[4]={0,0,':',0};
    char chainem[4]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 10, FONT_MD, "OPENING MODE");}
    if (langue==2) {drawString(5, 10, FONT_MD, "MODE OUVERTURE");}
    if (langue==3) {drawString(5, 10, FONT_MD, "OFFNUNG MODUS");}
    if (langue==4) {drawString(5, 10, FONT_MD, "MODO APERTURA");}
    if (langue==5) {drawString(5, 10, FONT_MD, "MODO APERTURA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(25, 30, FONT_SM, "FIXED TIME");}
    if (langue==2) {drawString(25, 30, FONT_SM, "HEURES FIXES");}
    if (langue==3) {drawString(25, 30, FONT_SM, "FESTE ZEIT");}
    if (langue==4) {drawString(25, 30, FONT_SM, "HORA FIJA");}
    if (langue==5) {drawString(25, 30, FONT_SM, "ORA FISSA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(45, 50, FONT_MD, "HOUR ?");}
    if (langue==2) {drawString(45, 50, FONT_MD, "HEURE ?");}
    if (langue==3) {drawString(45, 50, FONT_MD, "ZEIT ?");}
    if (langue==4) {drawString(45, 50, FONT_MD, "HORA ?");}
    if (langue==5) {drawString(45, 50, FONT_MD, "ORA ?");}


    chaineh[0]=heureO/10+0x30;
    chaineh[1]=heureO%10+0x30;
    chainem[0]=minO/10+0x30;
    chainem[1]=minO%10+0x30;

    setColor(COLOR_16_RED);
    drawString(45, 70, FONT_MD, chaineh);

    setColor(COLOR_16_WHITE);
    drawString(75, 70, FONT_MD, chainem);
}

void afficheecran12(void)  // Reglage min mode ouverture fixe
{
    char chaineh[4]={0,0,':',0};
    char chainem[4]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 10, FONT_MD, "OPENING MODE");}
    if (langue==2) {drawString(5, 10, FONT_MD, "MODE OUVERTURE");}
    if (langue==3) {drawString(5, 10, FONT_MD, "OFFNUNG MODUS");}
    if (langue==4) {drawString(5, 10, FONT_MD, "MODO APERTURA");}
    if (langue==5) {drawString(5, 10, FONT_MD, "MODO APERTURA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(25, 30, FONT_SM, "FIXED TIME");}
    if (langue==2) {drawString(25, 30, FONT_SM, "HEURES FIXES");}
    if (langue==3) {drawString(25, 30, FONT_SM, "FESTE ZEIT");}
    if (langue==4) {drawString(25, 30, FONT_SM, "HORA FIJA");}
    if (langue==5) {drawString(25, 30, FONT_SM, "ORA FISSA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(35, 50, FONT_MD, "MINUTE ?");}
    if (langue==2) {drawString(35, 50, FONT_MD, "MINUTE ?");}
    if (langue==3) {drawString(35, 50, FONT_MD, "MINUTE ?");}
    if (langue==4) {drawString(35, 50, FONT_MD, "MINUTO ?");}
    if (langue==5) {drawString(35, 50, FONT_MD, "MINUTO ?");}



    chaineh[0]=heureO/10+0x30;
    chaineh[1]=heureO%10+0x30;
    chainem[0]=minO/10+0x30;
    chainem[1]=minO%10+0x30;

    setColor(COLOR_16_WHITE);
    drawString(45, 70, FONT_MD, chaineh);

    setColor(COLOR_16_RED);
    drawString(75, 70, FONT_MD, chainem);
}

void afficheecran13(void)  // choix sensibilité capteur J/N Ouverture
{
    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 10, FONT_MD, "OPENING MODE");}
    if (langue==2) {drawString(5, 10, FONT_MD, "MODE OUVERTURE");}
    if (langue==3) {drawString(5, 10, FONT_MD, "OFFNUNG MODUS");}
    if (langue==4) {drawString(5, 10, FONT_MD, "MODO APERTURA");}
    if (langue==5) {drawString(5, 10, FONT_MD, "MODO APERTURA");}

    if (langue==1) {drawString(10, 30, FONT_SM, "DAY NIGHT Sensor");}
    if (langue==2) {drawString(10, 30, FONT_SM, "Capt JOUR NUIT");}
    if (langue==3) {drawString(10, 30, FONT_SM, "TAG-NACHT-Sensor");}
    if (langue==4) {drawString(10, 30, FONT_SM, "Sensor NOCHE DIA");}
    if (langue==5) {drawString(10, 30, FONT_SM, "Sensore NOTTE GIORNO");}

    if (langue==1) {drawString(15, 50, FONT_MD, "SENSITIVITY ?");}
    if (langue==2) {drawString(15, 50, FONT_MD, "SENSIBILITE ?");}
    if (langue==3) {drawString(15, 50, FONT_MD, "EMPFINDLICHKEIT ?");}
    if (langue==4) {drawString(15, 50, FONT_MD, "SENSIBILIDAD ?");}
    if (langue==5) {drawString(15, 50, FONT_MD, "SENSIBILITA ?");}


    if (langue==1) {drawString(5, 70, FONT_MD, "LOW");}
    if (langue==2) {drawString(5, 70, FONT_MD, "BASSE");}
    if (langue==3) {drawString(5, 70, FONT_MD, "LOW");}
    if (langue==4) {drawString(5, 70, FONT_MD, "BAJA");}
    if (langue==5) {drawString(5, 70, FONT_MD, "LOW");}


    if (langue==1) {drawString(5, 90, FONT_MD, "INTERMEDIATE");}
    if (langue==2) {drawString(5, 90, FONT_MD, "INTERMEDIARE");}
    if (langue==3) {drawString(5, 90, FONT_MD, "INTERMEDIATE");}
    if (langue==4) {drawString(5, 90, FONT_MD, "INTERMEDIO");}
    if (langue==5) {drawString(5, 90, FONT_MD, "INTERMEDIO");}

    if (langue==1) {drawString(5, 110, FONT_MD, "HIGH");}
    if (langue==2) {drawString(5, 110, FONT_MD, "HAUTE");}
    if (langue==3) {drawString(5, 110, FONT_MD, "HOCH");}
    if (langue==4) {drawString(5, 110, FONT_MD, "ALTA");}
    if (langue==5) {drawString(5, 110, FONT_MD, "ALTA");}
}

void afficheecran20(void)  // choix mode fermeture
{
    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(10, 10, FONT_MD, "CLOSE MODE");}
    if (langue==2) {drawString(10, 10, FONT_MD, "MODE FERMETURE");}
    if (langue==3) {drawString(10, 10, FONT_MD, "CLOSE MODUS");}
    if (langue==4) {drawString(10, 10, FONT_MD, "MODO CIERRE");}
    if (langue==5) {drawString(10, 10, FONT_MD, "MODO CHIUSURA");}



    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 30, FONT_MD, "FIXED TIME");}
    if (langue==2) {drawString(5, 30, FONT_MD, "HEURES FIXES");}
    if (langue==3) {drawString(5, 30, FONT_MD, "FESTE ZEIT");}
    if (langue==4) {drawString(5, 30, FONT_MD, "HORA FIJA");}
    if (langue==5) {drawString(5, 30, FONT_MD, "ORA FISSA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 50, FONT_MD, "DAY NIGHT Sensor");}
    if (langue==2) {drawString(5, 50, FONT_MD, "Capt JOUR NUIT");}
    if (langue==3) {drawString(5, 50, FONT_MD, "TAG-NACHT-Sensor");}
    if (langue==4) {drawString(5, 50, FONT_MD, "Sensor NOCHE DIA");}
    if (langue==5) {drawString(5, 50, FONT_MD, "NOTTE GIORNO");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 70, FONT_MD, "GPS LOCATION");}
    if (langue==2) {drawString(5, 70, FONT_MD, "POSITION GPS");}
    if (langue==3) {drawString(5, 70, FONT_MD, "GPS POSITION ");}
    if (langue==4) {drawString(5, 70, FONT_MD, "UBICACION GPS");}
    if (langue==5) {drawString(5, 70, FONT_MD, "POSITIONE GPS");}
}

void afficheecran21(void)  // Reglage heure mode fermeture fixe
{
    char chaineh[4]={0,0,':',0};
    char chainem[4]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 10, FONT_MD, "CLOSE MODE");}
    if (langue==2) {drawString(5, 10, FONT_MD, "MODE FERMETURE");}
    if (langue==3) {drawString(5, 10, FONT_MD, "CLOSE MODUS");}
    if (langue==4) {drawString(5, 10, FONT_MD, "MODO CIERRE");}
    if (langue==5) {drawString(5, 10, FONT_MD, "MODO CHIUSURA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(25, 30, FONT_SM, "FIXED TIME");}
    if (langue==2) {drawString(25, 30, FONT_SM, "HEURES FIXES");}
    if (langue==3) {drawString(25, 30, FONT_SM, "FESTE ZEIT");}
    if (langue==4) {drawString(25, 30, FONT_SM, "HORA FIJA");}
    if (langue==5) {drawString(25, 30, FONT_SM, "ORA FISSA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(45, 50, FONT_MD, "HOUR ?");}
    if (langue==2) {drawString(45, 50, FONT_MD, "HEURE ?");}
    if (langue==3) {drawString(45, 50, FONT_MD, "ZEIT ?");}
    if (langue==4) {drawString(45, 50, FONT_MD, "HORA ?");}
    if (langue==5) {drawString(45, 50, FONT_MD, "ORA ?");}

    chaineh[0]=heureF/10+0x30;
    chaineh[1]=heureF%10+0x30;
    chainem[0]=minF/10+0x30;
    chainem[1]=minF%10+0x30;

    setColor(COLOR_16_RED);
    drawString(45, 70, FONT_MD, chaineh);

    setColor(COLOR_16_WHITE);
    drawString(75, 70, FONT_MD, chainem);
}

void afficheecran22(void)  // Reglage min mode fermeture fixe
{
    char chaineh[4]={0,0,':',0};
    char chainem[4]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 10, FONT_MD, "CLOSE MODE");}
    if (langue==2) {drawString(5, 10, FONT_MD, "MODE FERMETURE");}
    if (langue==3) {drawString(5, 10, FONT_MD, "CLOSE MODUS");}
    if (langue==4) {drawString(5, 10, FONT_MD, "MODO CIERRE");}
    if (langue==5) {drawString(5, 10, FONT_MD, "MODO CHIUSURA");}


    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(25, 30, FONT_SM, "FIXED TIME");}
    if (langue==2) {drawString(25, 30, FONT_SM, "HEURES FIXES");}
    if (langue==3) {drawString(25, 30, FONT_SM, "FESTE ZEIT");}
    if (langue==4) {drawString(25, 30, FONT_SM, "HORA FIJA");}
    if (langue==5) {drawString(25, 30, FONT_SM, "ORA FISSA");}

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(35, 50, FONT_MD, "MINUTE ?");}
    if (langue==2) {drawString(35, 50, FONT_MD, "MINUTE ?");}
    if (langue==3) {drawString(35, 50, FONT_MD, "MINUTE ?");}
    if (langue==4) {drawString(35, 50, FONT_MD, "MINUTO ?");}
    if (langue==5) {drawString(35, 50, FONT_MD, "MINUTO ?");}

    chaineh[0]=heureF/10+0x30;
    chaineh[1]=heureF%10+0x30;
    chainem[0]=minF/10+0x30;
    chainem[1]=minF%10+0x30;

    setColor(COLOR_16_WHITE);
    drawString(45, 70, FONT_MD, chaineh);

    setColor(COLOR_16_RED);
    drawString(75, 70, FONT_MD, chainem);
}

void afficheecran23(void)  // choix sensibilité capteur J/N Fermeture
{
    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 10, FONT_MD, "CLOSE MODE");}
    if (langue==2) {drawString(5, 10, FONT_MD, "MODE FERMETURE");}
    if (langue==3) {drawString(5, 10, FONT_MD, "CLOSE MODUS");}
    if (langue==4) {drawString(5, 10, FONT_MD, "MODO CIERRE");}
    if (langue==5) {drawString(5, 10, FONT_MD, "MODO CHIUSURA");}

    if (langue==1) {drawString(10, 30, FONT_SM, "DAY NIGHT Sensor");}
    if (langue==2) {drawString(10, 30, FONT_SM, "Capt JOUR NUIT");}
    if (langue==3) {drawString(10, 30, FONT_SM, "TAG-NACHT-Sensor");}
    if (langue==4) {drawString(10, 30, FONT_SM, "Sensor NOCHE DIA");}
    if (langue==5) {drawString(10, 30, FONT_SM, "Sensore NOTTE GIORNO");}

    if (langue==1) {drawString(15, 50, FONT_MD, "SENSITIVITY ?");}
    if (langue==2) {drawString(15, 50, FONT_MD, "SENSIBILITE ?");}
    if (langue==3) {drawString(15, 50, FONT_MD, "EMPFINDLICHKEIT ?");}
    if (langue==4) {drawString(15, 50, FONT_MD, "SENSIBILIDAD ?");}
    if (langue==5) {drawString(15, 50, FONT_MD, "SENSIBILITA ?");}


    if (langue==1) {drawString(5, 70, FONT_MD, "LOW");}
    if (langue==2) {drawString(5, 70, FONT_MD, "BASSE");}
    if (langue==3) {drawString(5, 70, FONT_MD, "LOW");}
    if (langue==4) {drawString(5, 70, FONT_MD, "BAJA");}
    if (langue==5) {drawString(5, 70, FONT_MD, "LOW");}


    if (langue==1) {drawString(5, 90, FONT_MD, "INTERMEDIATE");}
    if (langue==2) {drawString(5, 90, FONT_MD, "INTERMEDIARE");}
    if (langue==3) {drawString(5, 90, FONT_MD, "INTERMEDIATE");}
    if (langue==4) {drawString(5, 90, FONT_MD, "INTERMEDIO");}
    if (langue==5) {drawString(5, 90, FONT_MD, "INTERMEDIO");}

    if (langue==1) {drawString(5, 110, FONT_MD, "HIGH");}
    if (langue==2) {drawString(5, 110, FONT_MD, "HAUTE");}
    if (langue==3) {drawString(5, 110, FONT_MD, "HOCH");}
    if (langue==4) {drawString(5, 110, FONT_MD, "ALTA");}
    if (langue==5) {drawString(5, 110, FONT_MD, "ALTA");}

}

void afficheecran24(void)  // réglage delai fermeture
{
    char chaine[3]={0,0,0};

    setColor(COLOR_16_WHITE);
    if (langue==1) {drawString(5, 10, FONT_MD, "CLOSE MODE");}
    if (langue==2) {drawString(5, 10, FONT_MD, "MODE FERMETURE");}
    if (langue==3) {drawString(5, 10, FONT_MD, "CLOSE MODUS");}
    if (langue==4) {drawString(5, 10, FONT_MD, "MODO CIERRE");}
    if (langue==5) {drawString(5, 10, FONT_MD, "MODO CHIUSURA");}

    if (langue==1) {drawString(5, 30, FONT_SM, "DELAYS THE CLOSING");}
    if (langue==2) {drawString(5, 30, FONT_SM, "RETARDE L'HEURE DE");}
    if (langue==3) {drawString(5, 30, FONT_SM, "VERZOGERUNGEN");}
    if (langue==4) {drawString(5, 30, FONT_SM, "RETRASOS HORA DE");}
    if (langue==5) {drawString(5, 30, FONT_SM, "RITARDI DI TIEMPO");}


    if (langue==1) {drawString(5, 40, FONT_SM, "TIME");}
    if (langue==2) {drawString(5, 40, FONT_SM, "FERMETURE");}
    if (langue==3) {drawString(5, 40, FONT_SM, "SCHLIEBUNG");}
    if (langue==4) {drawString(5, 40, FONT_SM, "CIERRE");}
    if (langue==5) {drawString(5, 40, FONT_SM, "DI CHIUSURA");}


    chaine[0]=minretard/10+0x30;
    chaine[1]=minretard%10+0x30;

    setColor(COLOR_16_RED);
    drawString(45, 70, FONT_MD, chaine);

    setColor(COLOR_16_WHITE);
    drawString(75, 70, FONT_MD, "min");
}

void afficheecran25(void)  // réglage delai fermeture
{
    char chaine[4]={0,0,0,0};


    setColor(COLOR_16_WHITE);

    drawString(45, 10, FONT_MD, "UTC");
/*
    if (langue==1) {drawString(5, 30, FONT_SM, "ex France : UTC+2 in sommer");}
    if (langue==2) {drawString(5, 30, FONT_SM, "ex France : UTC+2 en �t�");}
    if (langue==3) {drawString(5, 30, FONT_SM, "ex France : UTC+2 im sommer");}
    if (langue==4) {drawString(5, 30, FONT_SM, "ex France : UTC+2 en verano");}
    if (langue==5) {drawString(5, 30, FONT_SM, "ex France : UTC+2 in estate");}


    if (langue==1) {drawString(5, 40, FONT_SM, "ex France : UTC+1 in winter");}
    if (langue==2) {drawString(5, 40, FONT_SM, "ex France : UTC+1 en hiver");}
    if (langue==3) {drawString(5, 40, FONT_SM, "ex France : UTC+2 im winter");}
    if (langue==4) {drawString(5, 40, FONT_SM, "ex France : UTC+2 en invierno");}
    if (langue==5) {drawString(5, 40, FONT_SM, "ex France : UTC+2 in inverno");}
*/

    drawString(5, 40, FONT_SM, "ex France hiver: +1");

    if (UTC<0)
        {
        chaine[0]='-';
        absUTC=-UTC;
        }
    else
        {
        chaine[0]='+';
        absUTC=UTC;
        }
    chaine[1]=absUTC/10+0x30;
    chaine[2]=absUTC%10+0x30;

    setColor(COLOR_16_RED);
    drawString(45, 70, FONT_MD, chaine);
/*
    setColor(COLOR_16_WHITE);
    drawString(75, 70, FONT_MD, "min");
    */
}


#endif /* AFFICHAGEMENU_C_ */
