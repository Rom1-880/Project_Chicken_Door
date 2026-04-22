/*
 * ephemeride.c
 *
 *  Modified on: Apr 22, 2026
 *      Author: ALLAM Tarek
 *      for STM32 Portage
 */
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "ephemeride.h" // Modifié : Guillemets pour un fichier du projet

int jour=1;
int mois=8;
int annee=2019;

double longitude_ouest, latitude_nord;

extern signed char latitude;            // définis dans main.c
extern int longitude;                   // définis dans main.c
extern double lever, coucher;
extern char unsigned heureGPSO;         // de 0 à 23
extern char unsigned minGPSO;           // de 0 à 59
extern char unsigned heureGPSF;         // de 0 à 23
extern char unsigned minGPSF;           // de 0 à 59
extern struct tm tm_temps;              // définis dans main.c

double meridien;

const double radians = 0.017453292520;

int nbjours;
double d, x, sinlat, coslat;

double centre, variation;

int h,m,s;

//constantes précalculées par le compilateur
const double M_2PI = 6.2831853072;
const double degres = 57.2957795131;
const double radians2 = 0.034906585040;
const double m0 = 357.5291;
const double m1 = 0.98560028;
const double l0 = 280.4665;
const double l1 = 0.98564736;
const double c0 = 0.01671;
const double c1 = 1.9147581182;
const double c2 = 0.019997953085;
const double c3 = 0.00028961035658;
const double r1 = 0.207447644182976;
const double r2 = 0.043034525077;
const double d0 = 0.397777138139599;
const double o0 = -0.0106463073113138;

double M,Ca,L,R,dec,omega,x;

//* fonctions internes :

void calculerCentreEtVariation(double d)
{
  M = radians * fmod(m0 + m1 * d, 360.0);
  Ca = c1*sin(M) + c2*sin(2.0*M) + c3*sin(3.0*M);
  L = fmod(l0 + l1 * d + Ca, 360.0);
  x = radians2 * L;
  R = -degres * atan((r2*sin(x))/(1+r2*cos(x)));
  centre = (Ca + R + longitude_ouest)/360.0;
  meridien = (Ca + R + longitude_ouest)/360.0;
  dec = asin(d0*sin(radians*L));
  omega = (o0 - sin(dec)*sinlat)/(cos(dec)*coslat);
  if ((omega > -1.0) && (omega < 1.0))
    variation = acos(omega) / M_2PI;
  else
    variation = 0.0;
  x = variation;
}

void afficherHeure(double d)
{
  d = d + 0.5;
  if (d < 0.0)
  {
    d = d + 1.0;
  }
  else
  {
    if (d > 1.0)
    {
      d = d - 1.0;
    }
  }

  h = d * 24.0;
  d = d - (double) h / 24.0;
  m = d * 1440.0;
  d = d - (double) m / 1440.0;
  s = d * 86400.0 + 0.5;
}

void calculerEphemeride()
{
    longitude_ouest = -longitude;
    latitude_nord = latitude;

    jour=tm_temps.tm_mday;
    mois=tm_temps.tm_mon + 1;
    annee=tm_temps.tm_year + 1970;

  if (annee > 2000) annee -= 2000;
  nbjours = (annee*365) + ((annee+3)>>2) + jour - 1;
  switch (mois)
  {
    case  2 : nbjours +=  31; break;
    case  3 : nbjours +=  59; break;
    case  4 : nbjours +=  90; break;
    case  5 : nbjours += 120; break;
    case  6 : nbjours += 151; break;
    case  7 : nbjours += 181; break;
    case  8 : nbjours += 212; break;
    case  9 : nbjours += 243; break;
    case 10 : nbjours += 273; break;
    case 11 : nbjours += 304; break;
    case 12 : nbjours += 334; break;
  }
  if ((mois > 2) && (annee % 4 == 0)) nbjours++; // Remplacement de l'opérateur binaire pour plus de clarté
  d = nbjours;

  x = radians * latitude_nord;
  sinlat = sin(x);
  coslat = cos(x);
  calculerCentreEtVariation(d + longitude_ouest/360.0);
  lever = meridien - x;
  coucher = meridien + x;

  afficherHeure(lever);
  heureGPSO=h;
  minGPSO=m;

  afficherHeure(coucher);
  heureGPSF=h;
  minGPSF=m;
}

