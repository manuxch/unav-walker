/** \file globalSetup.hpp
 * \brief Archivo de cabecera para la clase GlobalSetup.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 * \date 2024.03.13
 * \version 1.0
 */

#ifndef _GLOBALSETUP_H
#define _GLOBALSETUP_H

#include <cstdlib>
using std::exit;
#include <iostream>
using std::cout;
using std::endl;
#include <fstream>
using std::ifstream;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <cmath>
using std::cos;
using std::sin;
#include <iomanip>
using std::fixed;
using std::setw;
#include "rng.hpp"
#include <box2d/box2d.h>

#define PI 3.141592653589793
#define INFO(msg)                                                              \
  fprintf(stderr, "info: %s:%d: ", __FILE__, __LINE__);                        \
  fprintf(stderr, "%s\n", msg);

/** \struct Mov_Base
 * \brief Estructura que contiene la cinética de la base vibrada en una
 * dimensión:
 * \li x : posición
 * \li v : velocidad
 * \li a : aceleración
 * */
struct Mov_Base {
  double x;
  double v;
  double a;
};
/** \struct Contenedor
 * \brief Estructura que almacena la información relativa a un contenedor de
 * granos
 *
 * Estructura que almacena la información relativa a un contenedor de granos:
 * \li datos geométricos
 * \li información sobre el material
 * */
struct Contenedor {
  double H;        /*!< Altura del contenedor */
  double R;        /*!< Radio del contenedor */
  double r;        /*!< Radio del orificio de salida del contenedor */
  double rest;     /*!< Coeficiente de restitución del contenedor */
  double fric;     /*!< Coeficiente de fricción del contenedor */
  double at_rotac; /*!< Coeficiente de atenuación de velocidad angular */
  double frec;     /*!< Frecuencia de vibración de la base */
  double Gamma;    /*!< Amplitud de la excitación armónica reducida */
  double zero_tol; /*!< Tolerancia para comparación de velocidad con cero */
  double rho;      /*!< Fracción de amplitud entre armónicos */
  double phi;      /*!< Diferencia de fase entre armónicos */
};

/** \struct tipoGrano
 * \brief Estructura que contiene información sobre un determinado tipo de
 * granos
 *
 * Estructura que contiene información sobre un determinado tipo de grano:
 * \li datos geométricos
 * \li información sobre el material que lo compone */
struct tipoGrano {
  int noGranos;      /*!< Cantidad de granos de este tipo */
  double radio;      /*!< Radio */
  int nLados;        /*!< Número de lados */
  double **vertices; /*!< Array con coordenadas de los vértices
                       (double x, double y) */
  double dens;       /*!< Densidad de los granos */
  double fric;       /*!< Coeficiente de rozamiento de los granos */
  double fric_s;     /*!< Coeficiente de fricción estática con la base */
  double fric_d;     /*!< Coeficiente de fricción dinámica con la base  */
  double rest;       /*!< Coeficiente de restitución de los granos */
};

/** \struct bodyData
 * \brief Estructura que almacena datos asociados a cada grano.
 * */
struct BodyData {
  int tipo;      /*!< Tipo de grano (en el orden en que aparecen en el .in */
  bool isGrain;  /*!< Variable lógica que identifica granos */
  bool isIn;     /*!< Variable lógica que identifica granos dentro del siloº */
  int gID;       /*!< Identificador del grano */
  int nLados;    /*!< Número de lados del grano (1 -> disco) */
  double fric_d; /*!< Fricción con la base, dinámica */
  double fric_s; /*!< Fricción con la base, estática */
};

/*! \class GlobalSetup
 * \brief Clase que contiene los parámetros de control del programa.
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */
class GlobalSetup {
public:
  // Parámetros de objetos del modelo Box2D
  Contenedor silo;    /*!< Recinto de contención */
  int noTipoGranos;   /*!< Cantidad de tipos de granos distintos en la
                        simulación */
  tipoGrano **granos; /*!< Array que contiene los distintos tipos de granos */

  // Parámetros de la excitación externa
  double ex_amp;  /*!< Amplitud de la excitación */
  double ex_freq; /*!< Frecuencia de la excitación */

  // Parámetros de control de la simulación
  double tStep;      /*!< Paso temporal de integración */
  double tBlock;     /*!< Tiempo de simulación con salida bloqueada */
  double maxT;       /*!< Límite temporal de la simulación - en períodos de
                              la excitación */
  int pIter;         /*!< Iteraciones para la satisfacción de restricciones de
                       posición */
  int vIter;         /*!< Iteraciones para la satisfacción de restricciones de
                       velocidad */
  double g;          /*!< Aceleración de la gravedad */
  double t_register; /*!< Tiempo de inicio de registros */

  // Parámetros de estadísticas y control
  string dirID; /*!< Identificador del directorio de archivos de frames de modo
                      que no se pisen. */
  int saveFrameFreq;   /*!< Frecuencia de guardado de frames */
  int fluxFreq;        /*!< Frecuencia de observación del flujo */
  string fluxFile;     /*!< Prefijo del nombre del archivo de salida de flujo */
  string preFrameFile; /*!< Prefijo del nombre del archivo de salida de
                         frame/trayectoria */
  uint32_t rnd_seed;   /*!< Semilla del generador de números aleatorios */
  int pf_freq; /*!< Frecuencia de guardado del packing fraction en la salida */
  string pf_file;     /*!< Archivo de guardado del pf en la salida */
  int freq_perfiles;  /*!< Frecuencia de actualización de perfiles de pf y
                        velocidad  en la salida */
  int n_bin_perfiles; /*!< Cantidad de bines en los perfiles de pf y velocidad
                      en la salida */
  int save_ve_freq;   /*!< Frecuencia de guardado de velocidades y energías */
  int save_contact_freq; /*!< Frecuencia de guardado de fuerzas de contacto */
  int save_tensors_freq; /*!< Frecuencia de guardado de tensores de estrés */

  // Constructor & destructor
  GlobalSetup(string input);
  ~GlobalSetup();

  // Info
  void printGlobalSetup();
  string input_par_file; /*!< Nombre del archivo que contiene los parámetros de
                      ejecución */

private:
  void load(string iFile);
};
#endif
