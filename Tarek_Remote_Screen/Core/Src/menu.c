/*
 *    Version 2
 *
 *    integre un ecran 0 afin de remettre la variable ON � 0.
 *    Inversion de l'affichage et de la remise � 0 des variables afin de gagner du temps et d'�viter des rebonds.
 */



#include "bibliotheque.h"
//#include <time.h>

/*
#define seuillumO1 2300;      // A mesurer
#define seuillumO2 2600;      // A mesurer
#define seuillumO3 3000;      // A mesurer

#define seuillumF1 2300;      // A mesurer
#define seuillumF2 2600;      // A mesurer
#define seuillumF3 3000;      // A mesurer
*/

extern short seuillumO1;      // seuil de luminosit� se calculant automatiquement par rapport � seuillummin et grace � ecartlumFO, diflumOF
extern short seuillumO2;      // A mesurer
extern short seuillumO3;      // A mesurer

extern short seuillumF1;      // A mesurer
extern short seuillumF2;      // A mesurer
extern short seuillumF3;      // A mesurer



extern signed char UTC;
extern signed char absUTC;

extern long tensionpile;

extern short seuillumO;   // valeur numerique du seuil de luminosit� de fermeture
extern short seuillumF;   // valeur numerique du seuil de luminosit� d'ouverture

extern uint8_t ON;           // variable indiquant qu'un appui sur le BP ON � eu lieu
extern uint8_t HUP;          // variable indiquant qu'un appui sur le BP HUP � eu lieu
extern uint8_t DWN;          // variable indiquant qu'un appui sur le BP DWN � eu lieu
extern uint8_t RTN;          // variable indiquant qu'un appui sur le BP RTN � eu lieu

extern uint8_t touche_ON;    // variable indiquant qu'un appui sur le BP ON � eu lieu
extern uint8_t touche_HUP;   // variable indiquant qu'un appui sur le BP HUP � eu lieu
extern uint8_t touche_DWN;   // variable indiquant qu'un appui sur le BP DWN � eu lieu
extern uint8_t touche_RTN;   // variable indiquant qu'un appui sur le BP RTN � eu lieu

//extern char FD_Touche_2;  // Front descendant Touche 2
//extern char FD_Touche_3;  // Front descendant Touche 3

extern char Touche_2_On;
extern char Touche_3_On;

extern unsigned int Compteur_appui_touche; // compteur pour detecter un appui long sur la touche 2 et 3 . ATTENTION si liaison serie sur Ecran 8 le temps augmente

extern short tempmescourant;


/*
 * remplac� par une variable tm de type time_t
extern char annee;      // annee variant de 0 à 255 correspondant à 2000 à 2255
extern char mois;        // mois variant de 1 à 12 !!! peur être de 0 à 11 pour correspondre à l'ephemeride!!!
extern char jour;
extern signed char heure;     // de 0 à 23
extern signed char min;         // de 0 à 59
*/

extern signed char heureO;     // de 0 � 23
extern signed char minO;         // de 0 � 59
extern signed char heureF;     // de 0 � 23
extern signed char minF;         // de 0 � 59

extern char signed minretard;        // de 0 � 90min;
extern char minretardchange;     // indique si la modification de minretard a �t� prise en compte

extern signed char latitude;        // france   // comprise entre -90 et 90� !!! Pb d'h�misph�re!!!
extern int longitude;                // france   // comprise entre 0 et 360� !!!

extern char choixi;                   // permet le choix de r�glage entre lattitude et longitude
long j=0;                   // tempo

extern uint8_t ecran;              // �tat de la machine d'�tat
extern char varmenu;            // indique si on est dans le menu de r�glage. doit repasser � 0 au bout de 10s sans appui
char choix=1;                   // variable permettant de choisir le menu qu'on veut s�lectionner
//extern char langue;             // choix de la langue;
extern int langue;             // choix de la langue;

extern char modeO;               // heure fixe : 1, capteur Jour nuit : 2, GPS : 3
extern char modeF;               // heure fixe : 1, capteur Jour nuit : 2, GPS : 3

extern char choixsensibiliteO;       // choix sensibilit� 1-basse 2-intermediaire 3-haute
extern char choixsensibiliteF;       // choix sensibilit� 1-basse 2-intermediaire 3-haute

extern char sens;
extern long courant;
extern long luminosite;

extern time_t temps;
extern struct tm tm_temps;


void menu()
{
    if ((ON==1)&(touche_ON==1)) touche_ON=0;
    if ((DWN==1)&(touche_DWN==1)) touche_DWN=0;
    if ((HUP==1)&(touche_HUP==1)) touche_HUP=0;
    if ((RTN==1)&(touche_RTN==1)) touche_RTN=0;

//    _BIS_SR(GIE);       // Enter Low Power Mode 0 and activate interrupts
//    switch (1)
    switch (ecran)
               {
                case 0:                  // etat initial qui passe � l'�cran1 directement en effa�ant ON!
                       if (ON==1)
                           {
                           ecran = 1;
                           choix = langue;
                           clearScreen(1);                                      // initialisation du LCD
                           }
                       if (DWN==1)
                           {
                           }
                       if (HUP==1)
                           {
                           }
                       if (RTN==1)
                           {
                           }
  //                     print("�cran = ");printnum99(ecran);print("  langue choisie = (1,2,3,4,5) : ");printnum99(choix);print("                 \r");
   //                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                       break;

               case 1:                  // choix langue
                   // affiche �cran1 avec english s�lectionn�
                   afficheecran1();
                   affichecarre(choix);
                   if (ON==1)
                       {
                       ecran = 2;
                       clearScreen(1);                                      // initialisation du LCD
                       choix=1;                                             // remise � 1 de la valeur choix pour l'�cran n�2, reglage langues
                       }
                   if (DWN==1)
                       {
                       // monte la selection en descendant le carr� de s�lection
                       choix++;
                       if (choix>5) choix = 5;
                       }
                   if (HUP==1)
                       {
                       // descend la s�lection
                       choix--;
                       if (choix<1) choix = 1;
                       }
                   if (RTN==1)
                       {
                       // valide la langue
                       if ((langue-choix)!=0) clearScreen(1);                                      // efface l'ecran en cas de changement de langue
                       langue = choix;

                       //// --------A cooriger----flash_erase(0xFA00);

                       flash_write(0xFA00,langue);       // pourquoi 2 fois?
                       flash_write(0xFA00,langue);       // pourquoi 2 fois?
                       flash_write(0xFA02,longitude);       // pourquoi 2 fois?
                       flash_write(0xFA02,longitude);       // pourquoi 2 fois?
                       flash_write(0xFA04,latitude);       // pourquoi 2 fois?
                       flash_write(0xFA04,latitude);       // pourquoi 2 fois?

                       }
 //                  print("�cran = ");printnum99(ecran);print("  langue choisie = (1,2,3,4,5) : ");printnum99(langue);print("                 \r");
                   //if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                   break;

               case 2:                  // choix r�glage
                   // affiche �cran2 avec langue s�lectionn�e
                   afficheecran2();
                   affichecarre(choix);

                   if (ON==1)
                       {
                       ecran = 8;
                       clearScreen(1);                                      // initialisation du LCD
                       }
                   if (DWN==1)
                       {
                       //monte la s�lection
                       choix++;
                       if (choix>5) choix = 5;
                       }
                   if (HUP==1)
                       {
                       // descend la selection
                       choix--;
                       if (choix<1) choix = 1;
                       }
                   if (RTN==1)
                       {
                       // valide la selection et envoie vers l'�cran s�lectionn�
                       switch (choix)
                                      {
                                      case 1:       // langue
                                          ecran = 1;
                                          choix = langue;
                                          clearScreen(1);                                      // initialisation du LCD
                                          break;
                                      case 2:       // date heure
                                          ecran = 3;
                                          clearScreen(1);                                      // initialisation du LCD
                                          break;
                                      case 3:       // mode ouverture
                                          ecran = 10;
                                          clearScreen(1);                                      // initialisation du LCD
                                          break;
                                      case 4:       // mode fermeture
                                          ecran = 20;
                                          clearScreen(1);                                      // initialisation du LCD
                                          break;
                                      case 5:       // GPS
                                          ecran = 9;
                                          clearScreen(1);                                      // initialisation du LCD
                                          choixi=1;      // r�glage de la lattitude
                                          break;
                                      }
                       }
   //                    print("�cran = ");printnum99(ecran);print("  r�glage choisi = (1,2,3,4) : ");printnum99(choix);print("                   \r");
   //                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                       break;

               case 3:      // reglage ann�e
                   // affiche �cran3
                   // affiche ann�e en cours
                   afficheecran3();
                   if (ON==1)
                       {
                       ecran = 2;
                       choix=1;                                             // remise � 1 de la valeur choix pour l'�cran n�2, reglage langues
                       clearScreen(1);                                      // initialisation du LCD
                       }
                   if (DWN==1)
                       {
                       // descend la selection
                       effaceligne();                                // efface l'ancienne ann�e
                       tm_temps.tm_year--;
                       temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                       }
                   if (HUP==1)
                       {
                       //monte la s�lection
                       effaceligne();                                                                               // efface l'ancienne ann�e
                       tm_temps.tm_year++;
                       temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                       }
                   if (RTN==1)
                       {
                       ecran=4;     // reglage mois
                       clearScreen(1);                                      // initialisation du LCD
                       }
//                   print("�cran = ");printnum99(ecran);print("  ann�e : ");printnum99(tm_temps.tm_year-30);print("                   \r");
                   //if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                   break;

                case 4:      // reglage mois
                    // affiche �cran4
                    // affiche mois en cours
                    afficheecran4();
                    if (ON==1)
                        {
                        ecran = 3;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        // descend la selection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_mon--;
                        if (tm_temps.tm_mon<0) tm_temps.tm_mon = 0;
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (HUP==1)
                        {
                        //monte la s�lection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_mon++;
                        if (tm_temps.tm_mon>11) tm_temps.tm_mon = 11;// int sign�!!
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (RTN==1)
                        {
                        ecran=5;     // reglage jour
                        clearScreen(1);                                      // initialisation du LCD
                        }
 //                   print("�cran = ");printnum99(ecran);print("  mois (1 � 12) : ");printnum99(tm_temps.tm_mon+1);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 5:      // reglage jour
                    // affiche �cran5
                    // affiche jour en cours
                    afficheecran5();
                    if (ON==1)
                        {
                        ecran = 4;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        // descend la selection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_mday--;
                        if (tm_temps.tm_mday<1) tm_temps.tm_mday = 1;
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (HUP==1)
                        {
                        //monte la s�lection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_mday++;
                        if (tm_temps.tm_mday>31) tm_temps.tm_mday = 31;
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (RTN==1)
                        {
                        ecran=6;     // reglage heure
                        clearScreen(1);                                      // initialisation du LCD
                        }
 //                   print("�cran = ");printnum99(ecran);print("  jour : ");printnum99(tm_temps.tm_mday);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 6:      // reglage heure
                    // affiche �cran6
                    // affiche heure en cours
                    afficheecran6();
                    if (ON==1)
                        {
                        ecran = 5;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        // descend la selection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_hour--;
                        if (tm_temps.tm_hour<0) tm_temps.tm_hour = 0;
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (HUP==1)
                        {
                        //monte la s�lection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_hour++;
                        if (tm_temps.tm_hour>23) tm_temps.tm_hour = 23;
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (RTN==1)
                        {
                        ecran=7;     // reglage minutes
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    print("�cran = ");printnum99(ecran);print("  heure : ");printnum99(tm_temps.tm_hour);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 7:      // reglage minutes
                    // affiche �cran7
                    // affiche minutes en cours
                    afficheecran7();
                    if (ON==1)
                        {
                        ecran = 6;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        // descend la selection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_min--;
                        if (tm_temps.tm_min<0) tm_temps.tm_min = 0;
                        tm_temps.tm_sec=0;
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (HUP==1)
                        {
                        //monte la s�lection
                        effaceligne();                                // efface l'ancienne ligne
                        tm_temps.tm_min++;
                        if (tm_temps.tm_min>59) tm_temps.tm_min = 59;
                        tm_temps.tm_sec=0;
                        temps=mktime(&tm_temps);        // converti tm_temps en temps (seconde depuis 1970!)
                        }
                    if (RTN==1)
                        {
                        ecran=25;     // reglage UTC
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    print("�cran = ");printnum99(ecran);print("  minutes : ");printnum99(tm_temps.tm_min);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;


                case 8:      // Ecran principal
                    // affiche �cran8
                    afficheecran8();
                    if (ON==1)
                    {
                        // rien
                    }

                    //                    if ((DWN==1)&&(sens==0))
                    if ((Compteur_appui_touche==15)&&(Touche_2_On)&&(sens==0))  // Si la touche est appuy�e un moment que le moteur ne tourne pas au relachement on ferme
                    {
                        // Fermeture manuelle
                        sens=1;
                        enablePWM();
                        //                       P4OUT = P4OUT | BIT0;     // fait tourner le moteur en continu (sans PWM)
                        //                       P4OUT = P4OUT &~BIT1;
                        // lorsque le courant de blocage sera atteint, il faudra disablePWM();
                    }
                    //                    if ((HUP==1)&&(sens==0))
                    if ((Compteur_appui_touche==15)&&(Touche_3_On)&&(sens==0)) // Si la touche est appuy�e un moment que le moteur ne tourne pas au relachement on ouvre
                    {
                        // Ouverture manuelle
                        sens=2;
                        enablePWM();
                        //                       P4OUT = P4OUT | BIT1;     // fait tourner le moteur en continu (sans PWM)
                        //                       P4OUT = P4OUT &~BIT0;
                        // lorsque le courant de blocage sera atteint, il faudra disablePWM();
                    }
                    if (RTN==1)
                    {
                        ecran=2;                   // reglage
                        clearScreen(1);            // initialisation du LCD
                        tempmescourant=10000;      // Arr�t du moteur si sortie de l'ecran 8 alors qu'il tourne  ( simule une rotation au dela de 20s )
                    }
                    if ((sens==1)&&(HUP))  // Arr�t du moteur si inversion de sens
                    {
                        tempmescourant=10000;      // simule une rotation au dela de 20s
                    }
                    if ((sens==2)&&(DWN))  // Arr�t du moteur si inversion de sens
                    {
                        tempmescourant=10000;      // simule une rotation au dela de 20s
                    }

//                    print("�cran = ");printnum99(ecran);print("  attente O/F par BP!    courant/10: ");printnum99(courant/10);print("  heure: ");printnum99(tm_temps.tm_hour);print(":");printnum99(tm_temps.tm_min );print(":");printnum99(tm_temps.tm_sec );print("  sens: ");printnum99(sens);
//                    print("   luminosit�/100 = ");printnum99(luminosite/100);print("   pile*10 = ");printnum99((13*tensionpile)/857); print(" \r");
                    //                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 9:      // GPS
                    // affiche �cran9
                    // affiche minutes en cours
                    afficheecran9();
                    if (ON==1)
                        {
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        // reglage lattitude puis longitude
                        if (choixi==1)
                            {
                            latitude--;
                            effaceligne2();                                // efface l'ancienne ligne
                            }
                        else
                            {
                            longitude--;
                            effaceligne3();                                // efface l'ancienne ligne
                            }
                        if (latitude<-90) latitude = -90;
                        if (longitude<-180) longitude = -180;
                        }
                    if (HUP==1)
                        {
                        // reglage lattitude puis longitude
                        if (choixi==1)
                            {
                            latitude++;
                            effaceligne2();                                // efface l'ancienne ligne
                            }
                        else
                            {
                            longitude++;
                            effaceligne3();                                // efface l'ancienne ligne
                            }
                        if (latitude>90) latitude = 90;
                        if (longitude>180) longitude = 180;
                        }
                    if (RTN==1)
                        {
                        if (choixi==1) choixi=2;  // passe � la lattitude
                        else
                            {
                            //// --------A cooriger----flash_erase(0xFA00);

                            flash_write(0xFA00,langue);       // pourquoi 2 fois?
                            flash_write(0xFA00,langue);       // pourquoi 2 fois?
                            flash_write(0xFA02,longitude);       // pourquoi 2 fois?
                            flash_write(0xFA02,longitude);       // pourquoi 2 fois?
                            flash_write(0xFA04,latitude);       // pourquoi 2 fois?
                            flash_write(0xFA04,latitude);       // pourquoi 2 fois?
                            choixi=1;
                            ecran=24;     // reglage
                            clearScreen(1);                                      // initialisation du LCD
                            }
                        }
//                    print("�cran = ");printnum99(ecran);print("  GPS : ");printnum99(latitude);printnum99(longitude);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 10:      // Mode d'ouverture
                    // affiche �cran10
                    afficheecran10();
                    affichecarre(modeO);
                    if (ON==1)
                        {
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        modeO++;
                        if (modeO>3) modeO = 3;
                        }
                    if (HUP==1)
                        {
                        modeO--;
                        if (modeO<1) modeO = 1;
                        }
                    if (RTN==1)
                        {
                        switch (modeO)
                              {
                              case 1:       // heure fixe
                                  ecran = 11;
                                  clearScreen(1);                                      // initialisation du LCD
                                  break;
                              case 2:       // capteur J/N
                                  ecran = 13;
                                  clearScreen(1);                                      // initialisation du LCD
                                  break;
                              case 3:       // Position GPS
                                  ecran = 9;
                                  clearScreen(1);                                      // initialisation du LCD
                                  break;
                              }
                        }
//                    print("�cran = ");printnum99(ecran);print("  Mode d'ouverture (1,2,3) : ");printnum99(modeO);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 11:      // Mode ouverture Heure fixe
                    // affiche �cran11
                    afficheecran11();
                    if (ON==1)
                        {
                        ecran = 10;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        heureO--;
                        if (heureO<0) heureO = 0;
                        }
                    if (HUP==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        heureO++;
                        if (heureO>23) heureO = 23;
                        }
                    if (RTN==1)
                        {
                        ecran=12;     // reglage des minutes d'ouverture
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    print("�cran = ");printnum99(ecran);print("  HeureO fixe : ");printnum99(heureO);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 12:      // Mode ouverture minute fixe
                    // affiche �cran12
                    afficheecran12();
                    if (ON==1)
                        {
                        ecran = 11;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        minO--;
                        if (minO<0) minO = 0;
                        }
                    if (HUP==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        minO++;
                        if (minO>59) minO = 59;
                        }
                    if (RTN==1)
                        {
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    print("�cran = ");printnum99(ecran);print("  minO fixe : ");printnum99(minO);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 13:      // Mode ouverture Capteur J/N
                    // affiche �cran13
                    afficheecran13();
                    affichecarre(choixsensibiliteO+2);
                    if (ON==1)
                        {
                        ecran = 10;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        choixsensibiliteO++;
                        if (choixsensibiliteO>3) choixsensibiliteO = 3;
                        }
                    if (HUP==1)
                        {
                        choixsensibiliteO--;
                        if (choixsensibiliteO<1) choixsensibiliteO = 1;
                        }
                    if (RTN==1)
                        {
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (choixsensibiliteO==1)   seuillumO=seuillumO1;
                    if (choixsensibiliteO==2)   seuillumO=seuillumO2;
                    if (choixsensibiliteO==3)   seuillumO=seuillumO3;
//                    print("�cran = ");printnum99(ecran);print("  Sensibilit�O (1,2,3) : ");printnum99(choixsensibiliteO);print("  SeuillumO/100 : ");printnum99(seuillumO/100);print("                  \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;


                    ////////////////////////////////////////////////////


                case 20:      // Mode de fermeture
                    // affiche �cran20
                    afficheecran20();
                    affichecarre(modeF);
                    if (ON==1)
                        {
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        modeF++;
                        if (modeF>3) modeF = 3;
                        }
                    if (HUP==1)
                        {
                        modeF--;
                        if (modeF<1) modeF = 1;
                        }
                    if (RTN==1)
                        {
                        switch (modeF)
                              {
                              case 1:       // heure fixe
                                  ecran = 21;
                                  clearScreen(1);                                      // initialisation du LCD
                                  break;
                              case 2:       // capteur J/N
                                  ecran = 23;
                                  clearScreen(1);                                      // initialisation du LCD
                                  break;
                              case 3:       // Position GPS
                                  ecran = 9;
                                  clearScreen(1);                                      // initialisation du LCD
                                  break;
                              }
                        }
//                    print("�cran = ");printnum99(ecran);print("  Mode fermeture (1,2,3) : ");printnum99(modeF);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;


                case 21:      // Mode fermeture Heure fixe
                    // affiche �cran21
                    afficheecran21();
                    if (ON==1)
                        {
                        ecran = 20;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        heureF--;
                        if (heureF<0) heureF = 0;
                        }
                    if (HUP==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        heureF++;
                        if (heureF>23) heureF = 23;
                        }
                    if (RTN==1)
                        {
                        ecran=22;     // reglage des minutes d'ouverture
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    print("�cran = ");printnum99(ecran);print("  HeureF fixe : ");printnum99(heureF);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 22:      // Mode fermeture minute fixe
                    // affiche �cran22
                    afficheecran22();
                    if (ON==1)
                        {
                        ecran = 21;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        minF--;
                        if (minF<0) minF = 0;
                        }
                    if (HUP==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        minF++;
                        if (minF>59) minF = 59;
                        }
                    if (RTN==1)
                        {
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    print("�cran = ");printnum99(ecran);print("  minF fixe : ");printnum99(minF);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 23:      // Mode fermeture Capteur J/N
                    // affiche �cran23
                    afficheecran23();
                    affichecarre(choixsensibiliteF+2);
                    if (ON==1)
                        {
                        ecran = 20;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        choixsensibiliteF++;
                        if (choixsensibiliteF>3) choixsensibiliteF = 3;
                        }
                    if (HUP==1)
                        {
                        choixsensibiliteF--;
                        if (choixsensibiliteF<1) choixsensibiliteF = 1;
                        }
                    if (RTN==1)
                        {
                        ecran=24;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (choixsensibiliteF==1)   seuillumF=seuillumF1;
                    if (choixsensibiliteF==2)   seuillumF=seuillumF2;
                    if (choixsensibiliteF==3)   seuillumF=seuillumF3;
//                    print("�cran = ");printnum99(ecran);print("  Sensibilit�F (1,2,3) : ");printnum99(choixsensibiliteF);print("  SeuillumF/100 : ");printnum99(seuillumF/100);print("                   \r");
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
                    break;

                case 24:      // Mode fermeture retard
                    // affiche �cran24
                    afficheecran24();
                    if (ON==1)
                        {
                        ecran = 20;
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        minretard--;
                        if (minretard<0) minretard = 0;
                        }
                    if (HUP==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        minretard++;
                        if (minretard>90) minretard = 90;
                        }
                    if (RTN==1)
                        {
                        minretardchange=1;
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
//                    print("�cran = ");printnum99(ecran);print("  Delai fermeture (0 � 90) : ");printnum99(minretard);print("                   \r");
                    break;

                case 25:      // Mode reglage UTC
                    // affiche �cran25
                    afficheecran25();
                    if (ON==1)
                        {
                        ecran = 7;                                          // retour reglage min
                        clearScreen(1);                                      // initialisation du LCD
                        }
                    if (DWN==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        UTC--;
                        if (UTC<-12) UTC = -12;
                        }
                    if (HUP==1)
                        {
                        effaceligne2();                                // efface l'ancienne ligne
                        UTC++;
                        if (UTC>+12) UTC = 12;
                        }
                    if (RTN==1)
                        {
                        ecran=2;     // reglage
                        clearScreen(1);                                      // initialisation du LCD
                        }
//                    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
//                    print("�cran = ");printnum99(ecran);print("  UTC (-12 � +12) : ");printnum99(absUTC);print("                   \r");
                    break;
                }
    ON=touche_ON;
    DWN=touche_DWN;
    HUP=touche_HUP;
    RTN=touche_RTN;

//    if ((ON==1)|(HUP==1)|(DWN==1)|(RTN==1)) {ON=0; HUP=0; DWN=0; RTN=0;}   // RAZ des tous les boutons au cas ou appui sur plusieurs d'un coHUP!
//    for (j=0;j<20000;j++);

               // affichage de l'�tat ou on se trouve
         /*  print("�cran = ");
           printnum99(ecran);
           print("\r");
         */

}
