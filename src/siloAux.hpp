/*! \file siloAux.hpp
 * \brief Archivo de cabecera para funciones auxiliares
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 * \version 1.0
 * \date 20018.12.14
 */

#include "globalsetup.hpp"
#include <box2d/box2d.h>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
using std::acos;
using std::atan2;
using std::cos;
using std::sin;
using std::sqrt;
// #include <gsl/gsl_sf_legendre.h>
#include <iostream>

extern GlobalSetup *globalSetup;
extern RNG *rng;

struct Tensor {
  float xx, xy, yx, yy;
};

/*! \struct ContactPointForce
 * \brief Fuerza en un punto de contacto, en la convención de Box2D.
 *
 * La normal n apunta del cuerpo A al cuerpo B y la tangente es
 * t = b2Cross(n, 1) = (n.y, -n.x), la misma que usa b2ContactSolver.
 * La fuerza sobre B es F = fn * n + ft * t; sobre A es -F.
 * */
struct ContactPointForce {
  b2Vec2 point;   /*!< Punto de contacto (coordenadas del mundo) */
  b2Vec2 normal;  /*!< Normal unitaria, de A hacia B */
  b2Vec2 tangent; /*!< Tangente unitaria, (n.y, -n.x) */
  double fn;      /*!< Componente normal (impulso normal * inv_dt) */
  double ft;      /*!< Componente tangencial (impulso tangencial * inv_dt) */
  double fx;      /*!< Componente x de la fuerza sobre B */
  double fy;      /*!< Componente y de la fuerza sobre B */
};

/*! \fn contact_point_force
 * \brief Fuerza sobre el cuerpo B en el punto i de un contacto. Con
 * inv_dt = 1 devuelve impulsos en lugar de fuerzas.
 * \param b2Contact* : contacto
 * \param b2WorldManifold& : manifold en coordenadas del mundo del contacto
 * \param int : índice del punto del manifold
 * \param double : inv_dt inverso del paso temporal
 * \return ContactPointForce
 * */
ContactPointForce contact_point_force(b2Contact *c, const b2WorldManifold &wm,
                                      int i, double inv_dt);

/*! \fn body_contact_wrench
 * \brief Fuerza y torque netos de contacto sobre un cuerpo (torque respecto
 * de su centro de masa), resueltos en el último Step.
 * \param b2Body* : cuerpo
 * \param double : inv_dt inverso del paso temporal
 * \param b2Vec2* : fuerza neta (salida)
 * \param double* : torque neto (salida)
 * \return void
 * */
void body_contact_wrench(b2Body *b, double inv_dt, b2Vec2 *F, double *tau);

/*! \fn provenance_header
 * \brief Líneas de comentario con la procedencia de un archivo de salida:
 * paso, tiempo, fase de la excitación, dt, versión del código y hash del
 * archivo de parámetros.
 * \return std::string (líneas que empiezan con '#', terminadas en '\n')
 * */
std::string provenance_header(const GlobalSetup *gs, double t, uint32_t nStep,
                              int n_frame);

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
 * \param int frm_id : identificador de frame, usualmente un contador incremental
 * \param int nStep : paso de la simulación (para calcular tiempo)
 * \param GlobalSetup* parámetros globales
 * \return void
 */
void saveFrame(b2World *w, int n_frame, int nStep,
               const GlobalSetup *globalSetup);

/*! Devuelve la cantidad de granos descargados
 * \param b2World* w
 * \param int n_frame (número de archivo a guardar)
 * \param int* st (suma por tipo de granos)
 * \param paso paso actual de la simulación
 * \param ofstream archivo de salida del flujo
 * \param GlobalSetup* parámetros globales
 * \return int
 */
int countDesc(b2World *w, int *st, int paso, std::ofstream &fluxFile,
              const GlobalSetup *gs);

/*! Imprime las velocidades y energías
 * \param frm_id : identificador de frame (acumulativo)
 * \param float timeS : tiempo de la
 * \param b2Word* w : mundo
 * \param GlobalSetup* parámetros globales
 * \return void
 */
void printVE(const int frm_id, const double timeS, uint32_t nStep, b2World *w,
             const GlobalSetup *gs);

/*! \fn saveContacts
 * \brief Guarda las fuerzas de contacto normal y tangencial, con la normal y
 * los centros de los cuerpos en contacto. Con save_roi_only se guardan todos
 * los contactos de los granos cuyo centro está en el ROI.
 * \param b2World* : mundo
 * \param double : tiempo de simulación
 * \param uint32_t : paso de simulación
 * \param int : identificador de frame (nombre de archivo)
 * \param GlobalSetup* parámetros globales
 * */
void saveContacts(b2World *w, double t, uint32_t nStep, int n_frame,
                  const GlobalSetup *globalSetup);

/*! \fn karnopp
 * \brief Fricción grano-base con el modelo de Karnopp.
 *
 * Adherencia: si |v_rel| < v_stick, la fuerza es la necesaria para que el
 * grano se mueva con la base al final del paso,
 *   F_stick = m a_base - F_ext - m v_rel / dt,
 * limitada en módulo a mu_s N. Deslizamiento: F = -mu_d N v_rel / |v_rel|.
 * La banda de adherencia es v_stick = max(v_tol, mu_d N dt / m): en un paso,
 * la fricción dinámica cambia la velocidad en mu_d N dt / m, así que una banda
 * más angosta nunca se alcanzaría y el grano oscilaría alrededor de v_rel = 0.
 * \param b2Vec2 : v_rel - velocidad relativa grano-base
 * \param b2Vec2 : F_ext - resto de las fuerzas sobre el grano (contactos)
 * \param b2Vec2 : a_base - aceleración de la base
 * \param double : m - masa del grano
 * \param double : dt - paso temporal
 * \param double : v_tol - umbral de velocidad para fricción estática
 * \param double : mu_s - coeficiente de fricción estática
 * \param double : mu_d - coeficiente de fricción dinámica (o cinética)
 * \param double : N - carga normal sobre la base (m g)
 * \return b2Vec2 : fuerza de fricción de la base sobre el grano
 * */
b2Vec2 karnopp(b2Vec2 v_rel, b2Vec2 F_ext, b2Vec2 a_base, double m, double dt,
               double v_tol, double mu_s, double mu_d, double N);

/*! \fn smooth_coulomb
 * \brief Devuelve el modelo de fricción de Smooth Coulomb como fuerza de
 * contacto.
 * \param b2Vec2 : v - velocidad relativa
 * \param double : v_d - velocidad de tolerancia
 * \param double : mu_d - coeficiente de fricción dinámica (o cinética)
 * \param double : p - peso del cuerpo apoyado sobre la superficie
 * \return b2Vec2 : fuerza de fricción de contacto
 * */
b2Vec2 smooth_coulomb(b2Vec2 v, double v_d, double mu_d, double p);

/*! \fn smooth_coulomb_2
 * \brief Devuelve el modelo de fricción de Smooth Coulomb como fuerza de
 * contacto.
 * \param b2Vec2 : v - velocidad relativa
 * \param double : v_d - velocidad de tolerancia
 * \param double : v_d - velocidad de Stribeck
 * \param double : mu_s - coeficiente de fricción estática
 * \param double : mu_d - coeficiente de fricción dinámica (o cinética)
 * \param double : p - peso del cuerpo apoyado sobre la superficie
 * \return b2Vec2 : fuerza de fricción de contacto
 * */
b2Vec2 smooth_coulomb_2(b2Vec2 v, double v_d, double v_s, double mu_d,
                        double mu_s, double p);

/*! \fn exitacion_mm
 * \brief Función que produce una exitación bi-armónica como en el paper de MM.
 * \param double : t - tiempo
 * \param double : gamma - aceleración reducida
 * \param double : w - frecuencia base (en rad/s)
 * \return Mov_Base : estructura que contiene x, v, a de la base en t
 * */
Mov_Base exitacion_mm(double t, double gamma, double w, const GlobalSetup *gs);

/*! \fn pivot_friction
 * \brief Torque de fricción de pivoteo de un disco apoyado sobre la base.
 *
 * Con presión de contacto uniforme sobre la cara del disco de radio R, el
 * torque de Coulomb que se opone al giro es (2/3) mu N R. Como en karnopp:
 * adherencia si |w| < w_stick, con el torque necesario para que w = 0 al
 * final del paso, tau = -tau_ext - I w / dt, limitado a (2/3) mu_s N R;
 * deslizamiento: tau = -(2/3) mu_d N R sign(w). La banda de adherencia es
 * w_stick = max(v_tol / R, (2/3) mu_d N R dt / I).
 * La base no rota, así que w es la velocidad angular relativa.
 * \param double : w - velocidad angular del grano
 * \param double : tau_ext - resto de los torques (contactos)
 * \param double : I - momento de inercia respecto del centro
 * \param double : R - radio del disco
 * \param double : dt - paso temporal
 * \param double : v_tol - umbral de velocidad (del borde, w R)
 * \param double : mu_s - coeficiente de fricción estática
 * \param double : mu_d - coeficiente de fricción dinámica
 * \param double : N - carga normal sobre la base (m g)
 * \return double : torque de fricción de la base sobre el grano
 * */
double pivot_friction(double w, double tau_ext, double I, double R, double dt,
                      double v_tol, double mu_s, double mu_d, double N);

/*! \fn do_base_force
 * \brief Función que aplica la fricción de la base sobre cada grano: fuerza
 * (karnopp) y torque de pivoteo (pivot_friction). La fuerza y el torque
 * aplicados quedan guardados en BodyData::F_base y BodyData::tau_base.
 * \param b2World* : w mundo
 * \param double : bvel - velocidad de la base (exitacion_mm)
 * \param double : bacc - aceleración de la base (exitacion_mm)
 * \param double : epsilon_v - velocidad umbral para el modelo de Karnopp
 * \param double : g - aceleración de la gravedad (carga normal)
 * \param double : dt - paso temporal
 * \return void
 * */
void do_base_force(b2World *w, double bvel, double bacc, double epsilon_v,
                   double g, double dt);


/*! \fn do_reinyection
 * \brief Función que reinyecta los granos que salieron del silo, en una
 * posición al azar sin superposición con otros cuerpos y con velocidad nula.
 * Si no encuentra lugar, reintenta en el paso siguiente.
 * \param b2World* : w mundo
 * \param GlobalSetup* : gs parámetros de simulación
 * \param bool : reinyect Reinyecta si true, elimina si false
 * \return void
 * */
void do_reinyection(b2World *w, GlobalSetup *gs, bool reinyect);

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
 * \brief Función que actualiza el perfil de velocidad y pf en el orificio de
 * salida.
 * \param b2World* : w mundo
 * \param double* : vel_0 array que almacena el histograma de velocidades
 * \param double* : pf_0 array que almacena el histograma de packing fraction
 * \param size_t* : bin_count array que registra los bines no nulos para las
 * velocidades
 * \param int : n_bins número de bins que divide el orificio de salida
 * \param double : r_out radio del orificio de salida
 * */
void update_pf_vx(b2World *w, double *vel_0, size_t *pf_0, size_t *bin_count,
                  int n_bins, double r_out);

/*! \fn save_tensors
 * \brief Guarda el tensor de estrés de contacto de cada grano,
 *   sigma_ij = (1 / A_grano) sum_c f_i l_j,
 * con f la fuerza sobre el grano y l = punto de contacto - centro del grano,
 * junto con la parte debida solo a las fuerzas normales, la posición, la
 * velocidad y el número de contactos activos.
 * \param b2World* : w mundo
 * \param int : n_frame identificador de frame para nombre de archivo
 * \param GlobalSetup* : gs parámetros de la simulación
 * \param double* : pmin mínima presión del frame
 * \param double* : pmax máxima presión del frame
 * \param double tSim : tiempo de simulación
 * \param uint32_t nStep : paso de simulación
 * \return void
 * */
void save_tensors(b2World *w, int n_frame, const GlobalSetup *globalSetup,
                  double *pmin, double *pmax, double tSim, uint32_t nStep);


/*! \fn get_body_area
 * \brief Devuelve el área de un cuerpo.
 * \param b2Body* : body cuerpo sobre el que se devuelve el área.
 * \return float : área del cuerpo
 */
float get_body_area(b2Body* body);

/*! \fn get_local_time
 * \brief Función para obtener la hora local como string
 * \param void
 * \return string
 * */
std::string get_local_time();

/*! \fn compute_wall_force
 * \brief Calcula la fuerza total ejercida por los granos sobre un cuerpo de
 * pared identificado por wall_gID, sumando todos los impulsos de contacto
 * resueltos en el último paso temporal.
 * \param b2World* : w mundo
 * \param GlobalSetup* : gs parámetros de la simulación (se usa tStep)
 * \param int : wall_gID gID del cuerpo de pared sobre el que se calcula la fuerza
 * \return b2Vec2 : fuerza total (Fx, Fy) en unidades de fuerza
 * */
b2Vec2 compute_wall_force(b2World *w, const GlobalSetup *gs, int wall_gID);

/*! \fn record_pre_step
 * \brief Guarda en BodyData la velocidad lineal y angular de cada grano
 * justo antes del Step (para check_force_balance).
 * \param b2World* : w mundo
 * \return void
 * */
void record_pre_step(b2World *w);

/*! \fn check_force_balance
 * \brief Verifica, después del Step, el balance de impulso lineal y angular
 * de cada grano:
 *   m (v - v_prev) = dt F_base + sum_c J_c,
 *   I (w - w_prev) = dt tau_base + sum_c l_c x J_c,
 * con J_c los impulsos de contacto reconstruidos con contact_point_force.
 * Escribe los residuos relativos y, como control, los que se obtienen con
 * la tangente invertida (-t).
 * \param b2World* : w mundo
 * \param GlobalSetup* : gs parámetros de la simulación
 * \param double : t tiempo al inicio del Step
 * \param uint32_t : nStep paso de simulación
 * \param ofstream& : archivo de salida
 * \return void
 * */
void check_force_balance(b2World *w, const GlobalSetup *gs, double t,
                         uint32_t nStep, std::ofstream &fout);
