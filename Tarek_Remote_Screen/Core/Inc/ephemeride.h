/*
 * ephemeride.h
 *
 *  Created on: 27 juin 2019
 *      Author: Joel
 */

#ifndef EPHEMERIDE_H_
#define EPHEMERIDE_H_

//* fonction principale :

void calculerEphemeride();
//Entrées :
//   jour
//   mois
//   annee : valeur 00 à 99 ou bien 2000 à 2099
//   longitude_ouest : nombre décimal, négatif si longitude est
//   latitude_nord : nombre décimal, négatif si latitude sud
//Sorties : lever, meridien, coucher sous forme de nombre décimal (julien)
//   -0.5 =>  0h00 UTC
//    0.0 => 12h00 UTC
//    0.5 => 24h00 UTC
//
//Les valeurs obtenues peuvent être vérifiées sur le site de l'Institut de Mécanique Céleste et de Calcul des Ephémérides (IMCCE)
//http://www.imcce.fr
//=> Ephémérides
//=> Phénomènes célestes
//=> Levers, couchers et passages au méridien des corps du système solaire
//=> Lieu géographique à saisir en coordonnées latitude / longitude
//=> Options : précision = seconde, Format = Sexagésimal


void calculerCentreEtVariation(double d);

double calculerCoordonneeDecimale(int degre, int minute, int seconde);

void avancerDate(int *jour, int *mois, int *annee);

void testerEphemeride(int nbjours,
                      int longitude_ouest_degres, int longitude_ouest_minutes, int longitude_ouest_secondes,
                      int latitude_nord_degres, int latitude_nord_minutes, int latitude_nord_secondes);



#endif /* EPHEMERIDE_H_ */
