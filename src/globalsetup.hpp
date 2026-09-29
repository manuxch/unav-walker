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
#include <chrono>
#include <sstream>

#define PI 3.141592653589793

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
  int noGranos;  /*!< Cantidad de granos de este tipo */
  double radio;  /*!< Radio */
  int nLados;    /*!< Número de lados */
  double dens;   /*!< Densidad de los granos */
  double fric;   /*!< Coeficiente de rozamiento de los granos */
  double fric_s; /*!< Coeficiente de fricción estática con la base */
  double fric_d; /*!< Coeficiente de fricción dinámica con la base  */
  double rest;   /*!< Coeficiente de restitución de los granos */
};

/** \struct bodyData
 * \brief Estructura que almacena datos asociados a cada grano.
 * */
struct BodyData {
  int tipo = 0; /*!< Tipo de grano (en el orden en que aparecen en el .in */
  bool isGrain = false; /*!< Variable lógica que identifica granos */
  bool isIn =
      false;      /*!< Variable lógica que identifica granos dentro del silo */
  int gID = 0;    /*!< Identificador del grano */
  int nLados = 0; /*!< Número de lados del grano (1 -> disco) */
  double fric_d = 0.0; /*!< Fricción con la base, dinámica */
  double fric_s = 0.0; /*!< Fricción con la base, estática */
  // Estado auxiliar para el chequeo de balance de fuerzas (ver
  // check_force_balance): fuerza y torque de la base aplicados y velocidades
  // previas al Step.
  b2Vec2 F_base = b2Vec2(0.0f, 0.0f);
  float tau_base = 0.0f;
  b2Vec2 v_prev = b2Vec2(0.0f, 0.0f);
  float w_prev = 0.0f;
};

/*! \class GlobalSetup
 * \brief Clase que contiene los parámetros de control del programa.
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */
class GlobalSetup {
public:
  // Parámetros de objetos del modelo Box2D
  Contenedor silo{};            /*!< Recinto de contención */
  int noTipoGranos = 0;         /*!< Cantidad de tipos de granos distintos */
  tipoGrano **granos = nullptr; /*!< Array con los distintos tipos de granos */

  // Parámetros de control de la simulación
  double tStep = 0.0;       /*!< Paso temporal de integración */
  double tBlock = 0.0;      /*!< Tiempo de simulación con salida bloqueada */
  double maxT = 0.0;        /*!< Tiempo máximo de simulación */
  int pIter = 0;            /*!< Iteraciones de restricciones de posición */
  int vIter = 0;            /*!< Iteraciones de restricciones de velocidad */
  bool reinyection = false; /*!< True si se reinyectan granos por arriba */
  double g = 0.0;           /*!< Aceleración de la gravedad (carga normal sobre
                               la base para la fricción de Karnopp) */
  double t_register = 0.0;  /*!< Tiempo de inicio de registros */
  int maxGranosDesc = 0;    /*!< Máx. granos descargados para detener la
                               simulación. 0 = condición deshabilitada. */
  bool continuous_physics = false; /*!< True: detección continua de colisiones
                                    (TOI) y granos "bullet". Los impulsos de
                                    los subpasos TOI no quedan registrados en
                                    los manifolds de contacto. */
  bool fondo_medicion = false;     /*!< True: silo cerrado con fondo de medición
                                      (gID=-200), sin orificio ni descarga */

  // Parámetros de estadísticas y control
  string dirID;          /*!< Identificador del directorio de frames */
  int saveFrameFreq = 0; /*!< Frecuencia de guardado de frames */
  int fluxFreq = 0;      /*!< Frecuencia de observación del flujo */
  string fluxFile;       /*!< Nombre del archivo de salida de flujo */
  string preFrameFile;   /*!< Prefijo de los archivos de frames */
  uint32_t rnd_seed = 0; /*!< Semilla del generador de números aleatorios */
  int pf_freq = 0;       /*!< Frecuencia de guardado del packing fraction */
  string pf_file = "pf_out.dat"; /*!< Archivo de guardado del pf */
  int freq_perfiles = 0;  /*!< Frecuencia de actualización de perfiles pf-v */
  int n_bin_perfiles = 1; /*!< Cantidad de bines en los perfiles pf-v */
  int save_ve_freq = 0;   /*!< Frecuencia de guardado de velocidades */
  int save_contact_freq = 0;  /*!< Frecuencia de guardado de contactos */
  int save_tensors_freq = 0;  /*!< Frecuencia de guardado de tensores */
  int check_balance_freq = 0; /*!< Frecuencia del chequeo de balance de
                                 fuerzas (0 = deshabilitado) */

  // Parámetros de ROI (Region of Interest) para guardado de datos
  bool save_roi_only = false;
  double x_roi = 0.0;
  double y_min_roi = 0.0;
  double y_max_roi = 0.0;

  // Procedencia
  string params_hash; /*!< Hash FNV-1a (hex) del contenido del archivo de
                         parámetros */

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
