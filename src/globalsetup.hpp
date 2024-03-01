/** \file globalSetup.hpp
 * \brief Archivo de cabecera para la clase GlobalSetup. 
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 * \date 2020.02.17
 * \version 0.1
 */

#ifndef _GLOBALSETUP_H
#define _GLOBALSETUP_H

#include <cstdlib>
using std::exit;
#include <iostream>
using std::cout; using std::endl;
#include <fstream>
using std::ifstream;
#include <string>
using std::string;
#include <vector>
using std::vector;
#include <cmath>
using std::sin; using std::cos;
#include <iomanip>
using std::fixed; using std::setw;
#include <box2d/box2d.h>

#define PI 3.14159265359
#define INFO(msg) \
    fprintf(stderr, "info: %s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, "%s\n", msg);
/** \struct Contenedor
 * \brief Estructura que almacena la información relativa a un contenedor de 
 * granos
 * 
 * Estructura que almacena la información relativa a un contenedor de granos:
 * \li datos geométricos
 * \li información sobre el material
 * */
struct Contenedor {
    double altura; /*!< Altura del contenedor */
    double base; /*!< Base del contenedor */
    double espesor; /*!< Espesor de las paredes del contenedor */
    double friccion_s; /*!< Coeficiente de fricción estática del contenedor  */
    double friccion_d; /*!< Coeficiente de fricción dinámica del contenedor  */
    double restitucion; /*!< Coeficiente de restitución del contenedor */
    double dens; /*!< Densidad del material del contenedor */
    double frec; /*!< Frecuencia de vibración de la caja */
    double Gamma; /*!< Amplitud de la excitación armónica reducida */
    double zero_tol; /*!< Tolerancia para comparación con cero */
    double rho; /*!< Fracción de amplitud entre armónicos */
    double phi; /*!< Diferencia de fase entre armónicos */
};

/** \struct tipoGrano
 * \brief Estructura que contiene información sobre un determinado tipo de 
 * granos
 * 
 * Estructura que contiene información sobre un determinado tipo de grano:
 * \li datos geométricos
 * \li información sobre el material que lo compone */
struct tipoGrano {
    int noGranos;	/*!< Cantidad de granos de este tipo */
    double radio;	/*!< Radio */
    int nLados;		/*!< Número de lados */
    double **vertices;	/*!< Array con coordenadas de los vértices 
                          (double x, double y) */
    double dens; /*!< Densidad de los granos */
    double fric; /*!< Coeficiente de rozamiento de los granos */
    double rest; /*!< Coeficiente de restitución de los granos */
    double packFrac; /*!< Fraccion de empaquetamiento para los granos */
};

/** \struct bodyData
 * \brief Estructura que almacena datos asociados a cada grano.
 * */
struct BodyData {
    int tipo; /*!< Tipo de grano (en el orden en que aparecen en el .in */
    bool isGrain;   /*!< Variable lógica que identifica granos */
    int gID; /*!< Identificador del grano */
    int nLados; /*!< Número de lados del grano (1 -> disco) */
};


/*! \class GlobalSetup
 * \brief Clase que contiene los parámetros de control del programa.
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */
class GlobalSetup {
public:

    // Parámetros de objetos del modelo Box2D
    Contenedor caja; /*!< Recinto de contención */
    int noTipoGranos; /*!< Cantidad de tipos de granos distintos en la 
                        simulación */
    tipoGrano **granos; /*!< Array que contiene los distintos tipos de granos */

    // Parámetros de la excitación externa
    double ex_amp; /*!< Amplitud de la excitación */
    double ex_freq; /*!< Frecuencia de la excitación */

    // Parámetros de control de la simulación
    double tStep; /*!< Paso temporal de integración */
    double maxT; /*!< Límite temporal de la simulación - en períodos de 
                           la excitación */
    int pIter; /*!< Iteraciones para la satisfacción de restricciones de 
                 posición */
    int vIter; /*!< Iteraciones para la satisfacción de restricciones de 
                 velocidad */
    double g; /*!< Aceleración de la gravedad */

    // Parámetros de estadísticas y 
    string dirID; /*!< Identificador del directorio de archivos de frames de modo
                        que no se pisen. */
    int saveFrameFreq; /*!< Frecuencia de guardado de frames */
    string preFrameFile; /*!< Prefijo del nombre del archivo de salida de 
                           frame/trayectoria */
    uint32_t rnd_seed;

    
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


