/*! \file global_setup.hpp
 * \brief Parámetros de la simulación, leídos del archivo de parámetros.
 *
 * El formato del archivo y la lista de claves están documentados en
 * README.md y en params.in.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

/*! \struct Contenedor
 * \brief Geometría y material del silo, y parámetros de la excitación.
 */
struct Contenedor {
  double H = 0.0;        //!< Altura del silo
  double R = 0.0;        //!< Semiancho del silo
  double r = 0.0;        //!< Semiancho del orificio de salida (D = 2 r)
  double rest = 0.0;     //!< Coeficiente de restitución de las paredes
  double fric = 0.0;     //!< Coeficiente de fricción de las paredes
  double frec = 0.0;     //!< Frecuencia de la excitación de la base
  double gamma = 0.0;    //!< Aceleración reducida de la excitación (Gamma)
  double zero_tol = 0.0; //!< Umbral de velocidad para la adherencia (Cero_tol)
  double rho = 0.0;      //!< Fracción de amplitud del primer armónico
  double phi = 0.0;      //!< Diferencia de fase entre armónicos
};

/*! \struct TipoGrano
 * \brief Propiedades de un tipo de grano (disco).
 */
struct TipoGrano {
  int n_granos = 0;    //!< Cantidad de granos de este tipo
  double radio = 0.0;  //!< Radio del disco
  double dens = 0.0;   //!< Densidad (masa por unidad de área)
  double fric = 0.0;   //!< Coeficiente de fricción grano-grano
  double fric_s = 0.0; //!< Coeficiente de fricción estática con la base
  double fric_d = 0.0; //!< Coeficiente de fricción dinámica con la base
  double rest = 0.0;   //!< Coeficiente de restitución
};

/*! \class GlobalSetup
 * \brief Parámetros de control del programa.
 *
 * El constructor lee y valida el archivo de parámetros y los imprime en la
 * salida estándar. Ante cualquier error termina el programa con un mensaje.
 */
class GlobalSetup {
public:
  /*! Lee, valida e imprime los parámetros de \p params_file. */
  explicit GlobalSetup(const std::string &params_file);

  /*! Imprime los parámetros en la salida estándar (líneas con '#'). */
  void print() const;

  // Contenedor, excitación y granos
  Contenedor silo;               //!< Silo y excitación
  std::vector<TipoGrano> granos; //!< Tipos de granos

  // Control de la simulación
  double dt = 0.0;             //!< Paso temporal (timeStep)
  double t_max = 0.0;          //!< Tiempo máximo de simulación (tMax)
  double t_block = 0.0;        //!< Tiempo con el orificio tapado (tBlock)
  double t_register = 0.0;     //!< Inicio de los registros (t_Register)
  int p_iter = 0;              //!< Iteraciones de posición de Box2D (pIter)
  int v_iter = 0;              //!< Iteraciones de velocidad de Box2D (vIter)
  double g = 0.0;              //!< Gravedad: carga normal sobre la base (m g)
  bool reinyeccion = false;    //!< Reinyectar los granos descargados
  int max_granos_desc = 0;     //!< Granos descargados para terminar (0: no)
  bool fondo_medicion = false; //!< Silo cerrado con fondo de medición
  bool continuous_physics = false; //!< Detección continua de colisiones (TOI)

  // Registros
  std::string dir_id;                 //!< Sufijo del directorio frames_<dir_id>
  std::string pre_frame_file;         //!< Prefijo de los archivos de frames
  std::string flux_file;              //!< Archivo de granos descargados
  std::string pf_file = "pf_out.dat"; //!< Archivo de packing fraction
  uint32_t rnd_seed = 0;              //!< Semilla del generador aleatorio
  int save_frame_freq = 0;            //!< Frecuencia de .xy (pasos; 0: no)
  int flux_freq = 0;                  //!< Habilita el archivo de flujo (> 0)
  int pf_freq = 0;                    //!< Frecuencia del packing fraction
  int freq_perfiles = 0;              //!< Frecuencia de perfiles en el orificio
  int n_bin_perfiles = 1;             //!< Bines de los perfiles en el orificio
  int save_ve_freq = 0;               //!< Frecuencia de .ve
  int save_contact_freq = 0;          //!< Frecuencia de fc_*.dat
  int save_tensors_freq = 0;          //!< Frecuencia de .sxy
  int check_balance_freq = 0;         //!< Frecuencia del chequeo de balance

  // Región de interés (ROI) para las salidas por grano y por contacto
  bool save_roi_only = false; //!< false: todo el sistema
  double x_roi = 0.0;         //!< Semiancho del ROI: |x| <= x_roi
  double y_min_roi = 0.0;     //!< Límite inferior del ROI en y
  double y_max_roi = 0.0;     //!< Límite superior del ROI en y

  // Procedencia
  std::string params_file; //!< Nombre del archivo de parámetros
  std::string params_hash; //!< Hash FNV-1a (hex) del archivo de parámetros

private:
  void load(const std::string &params_file);
};
