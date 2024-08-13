/*! \file siloAux.hpp
 * \brief Archivo de cabecera para funciones auxiliares
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 * \version 1.0
 * \date 20018.12.14
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <box2d/box2d.h>
#include "globalsetup.hpp"
#include <cmath>
using std::sqrt; using std::sin; using std::cos; using std::atan2;
using std::acos;
//#include <gsl/gsl_sf_legendre.h>
#include <iostream>

extern GlobalSetup *globalSetup;
extern RNG *rng;

struct Tensor{
    float xx, xy, yx, yy;
};


/*! Convierte un número en una string de ancho fijo
 * y rellena de ceros, para enumerar frames secuencialmente
 * \param int num
 * \return std::string
 */
std::string int2str(int num);

/*! Detecta si el sistema está activo
 * \param b2World* w 
 * \return bool
 */
bool isActive(b2World *w);

/*! Escribe en el archivo de salida las coordenadas de las partículas
 * \param b2Word* w
 * \param int file_ID
 * \param GlobalSetup* parámetros globales
 * \return void
 */
void savePart(b2World *w, int file_id, const GlobalSetup *globalSetup);

/*! Escribe todas las coordenadas necesarias para generar imágenes
 * y posteriores animaciones
 * \param b2World* w
 * \param int frm_id
 * \param GlobalSetup* parámetros globales
 * \return void
 */
void saveFrame(b2World *w, int n_frame, int frm_id, const GlobalSetup *globalSetup);

/*! Devuelve la cantidad de granos descargados
 * \param b2World* w
 * \param int n_frame (número de archivo a guardar)
 * \param int* st (suma por tipo de granos)
 * \param paso paso actual de la simulación
 * \param ofstream archivo de salida del flujo
 * \param GlobalSetup* parámetros globales
 * \return int
 */
int countDesc(b2World *w, int *st, int paso,
        std::ofstream &fluxFile, const GlobalSetup* gs);

/*! Imprime las velocidades y energías
 * \param float timeS : tiempo de la 
 * \param b2Word* w : mundo
 * \param GlobalSetup* parámetros globales
 * \return void
 */
void printVE(int frm_id, float timeS, b2World *w, const GlobalSetup* gs);

/*! \fn saveContacts
 * \brief Guarda las fuerzas de contacto normal y tangencial
 * \param b2World* : mundo
 * \param float : time step
 * \param int : file ID
 * \param GlobalSetup* parámetros globales
 * */
void saveContacts(b2World *w, float ts, int file_id, 
        const GlobalSetup *globalSetup);

/*! \fn karnopp
 * \brief Devuelve el modelo de fricción de Karnopp como fuerza de contacto.
 * \param b2Vec2 : v - velocidad relativa
 * \param double : v_tol - umbral de velocidad para fricción estática
 * \param double : mu_s - coeficiente de fricción estática
 * \param double : mu_d - coeficiente de fricción dinámica (o cinética)
 * \param double : p - peso del cuerpo apoyado sobre la superficie 
 * \return b2Vec2 : fuerza de fricción de contacto
 * */
b2Vec2 karnopp(b2Vec2 v, double v_tol, double mu_s, double mu_d, double p);

/*! \fn smooth_coulomb
 * \brief Devuelve el modelo de fricción de Smooth Coulomb como fuerza de contacto.
 * \param b2Vec2 : v - velocidad relativa
 * \param double : v_d - velocidad de tolerancia
 * \param double : mu_d - coeficiente de fricción dinámica (o cinética)
 * \param double : p - peso del cuerpo apoyado sobre la superficie 
 * \return b2Vec2 : fuerza de fricción de contacto
 * */
b2Vec2 smooth_coulomb(b2Vec2 v, double v_d, double mu_d, double p);

/*! \fn smooth_coulomb_2
 * \brief Devuelve el modelo de fricción de Smooth Coulomb como fuerza de contacto.
 * \param b2Vec2 : v - velocidad relativa
 * \param double : v_d - velocidad de tolerancia
 * \param double : v_d - velocidad de Stribeck
 * \param double : mu_s - coeficiente de fricción estática
 * \param double : mu_d - coeficiente de fricción dinámica (o cinética)
 * \param double : p - peso del cuerpo apoyado sobre la superficie 
 * \return b2Vec2 : fuerza de fricción de contacto
 * */
b2Vec2 smooth_coulomb_2(b2Vec2 v, double v_d, double v_s, double mu_d, double mu_s, double p);

/*! \fn exitacion_mm
 * \brief Función que produce una exitación bi-armónica como en el paper de MM.
 * \param double : t - tiempo
 * \param double : gamma - aceleración reducida
 * \param double : w - frecuencia base (en rad/s)
 * \return Mov_Base : estructura que contiene x, v, a de la base en t
 * */
Mov_Base exitacion_mm(double t, double gamma, double w, const GlobalSetup *gs);

/*! \fn do_base_force
 * \brief Función que aplica la fuerza de fricción de la base sobre cada grano.
 * \param b2World* : w mundo
 * \param double : bvel - velocidad de la base (en x)
 * \param double : epsilon_v - velocidad umbral para el modelo de Karnopp (en y)
 * \param GlobalSetup* : gs parámetros de simulación
 * \return velocidad
 * */
void do_base_force(b2World* w, double bvel, double epsilon_v, double g);

/*! \fn do_rot_friction
 * \brief Función que aplica una atenuación a la velocidad angular.
 * \param b2World* : w mundo
 * \param GlobalSetup* : gs parámetros de simulación
 * \return void
 * */
void do_rot_friction(b2World *w, const GlobalSetup *gs);

/*! \fn do_reinyection
 * \brief Función que reinyecta los granos que salieron del silo.
 * \param b2World* : w mundo
 * \param GlobalSetup* : gs parámetros de simulación
 * \return void
 * */
void do_reinyection(b2World *w, GlobalSetup *gs);

/*! \fn save_pf
 * \brief Guarda el packing fraction bulk y a la salida del silo.
 * \param b2World* : w mundo
 * \param GlobalSetup* : gs parámetros de la simulación
 * \param double : t tiempo de registro
 * \param ofstream : &fout archivo de registro
 * \return void
 * */
void save_pf(b2World *w, GlobalSetup *gs, double t, std::ofstream &fout);

/*! \fn get_clipped_area
 * \brief Función que calcula la intersección entre un círculo y un rectángulo.
 * \param double : y_inf altura inferior del rectángulo
 * \param double : y_sup altura superior del rectángulo
 * \param double : y altura del centro de la circunferencia
 * \param double : r radio de la circunferencia
 * \return double : area de intersección
 * */
double get_clipped_area(double y_inf, double y_sup, double y, double r);

/*! \fn update_pf_vx
 * \brief Función que actualiza el perfil de velocidad y pf en el orificio de salida.
 * \param b2World* : w mundo
 * \param double* : vel_0 array que almacena el histograma de velocidades
 * \param double* : pf_0 array que almacena el histograma de packing fraction
 * \param int : n_bins número de bins que divide el orificio de salida
 * \param double : r_out radio del orificio de salida
 * */
void update_pf_vx(b2World *w, double *vel_0, size_t *pf_0, int n_bins, double r_out);

/*! \fn save_tensors
 * \brief Función que guarda los tensores fabric y stress de cada grano.
 * \param b2World* : w mundo
 * \param int : frm_id identificador de frame para nombre de archivo
 * \param GlobalSetup* : gs parámetros de la simulación
 * \return void
 * */
void save_tensors(b2World *w, int n_frame, const GlobalSetup *globalSetup);
