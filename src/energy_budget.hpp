/*! \file energy_budget.hpp
 * \brief Balance de energía de la base vibrada.
 *
 * En cada Step, Box2D integra primero las fuerzas aplicadas (v' = v0 +
 * dt F / m, w' = w0 + dt tau / I) y después resuelve los contactos. El
 * trabajo de la fricción de la base sobre un grano en el paso es entonces el
 * cambio de energía cinética de la primera etapa:
 *
 *     W_lin = dt F . v0 + dt^2 |F|^2 / (2 m),
 *     W_piv = dt tau w0 + dt^2 tau^2 / (2 I),
 *
 * con F, tau la fuerza de Karnopp y el torque de pivoteo (BodyData::f_base,
 * BodyData::tau_base). El resto del cambio de energía cinética del paso es
 * exactamente el trabajo de los contactos (granos y paredes):
 *
 *     W_cont = dK - (W_lin + W_piv).
 *
 * En el código, W_lin y W_piv se calculan como K(v') - K(v0), con v' la
 * velocidad de la primera etapa obtenida con la misma aritmética de punto
 * flotante que Box2D. Es lo mismo que las fórmulas de arriba, pero sin el
 * error de redondeo (relativo ~1e-7 por paso), de modo que W_cont es
 * exactamente el cambio de energía que producen los contactos.
 *
 * El trabajo que hace la base (la energía que sale del motor) es
 * W_b = dt F . v_b, con v_b la velocidad de la base que usó la fricción.
 * La disipación por deslizamiento contra la base es D_slide = W_b - W_lin y
 * la de pivoteo, D_piv = -W_piv.
 *
 * Entre dos Step solo la descarga y la reinyección cambian velocidades: los
 * granos reinyectados quedan en reposo y los eliminados desaparecen. Ese
 * cambio de energía es dK_reiny = K(antes del paso) - K(después del paso
 * anterior). Así el balance cierra:
 *
 *     K(t2) - K(t1) = W_lin + W_piv + W_cont + dK_reiny.
 *
 * Uso en el bucle, después de la descarga y la reinyección:
 *
 *     energy.before_step(&world, base_vel);
 *     world.Step(...);
 *     energy.after_step(&world, t, dt);   // t: tiempo al inicio del paso
 *
 * Supone que no hay amortiguamiento de Box2D ni gravedad en el plano (las
 * dos son nulas en unav-walkers) y que la base es la única fuerza aplicada.
 *
 * Energía en unidades reducidas, m g d.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#pragma once

#include <box2d/box2d.h>

#include <fstream>
#include <string>
#include <vector>

/*! \struct EnergyWork
 * \brief Trabajos y cambios de energía acumulados (un paso, un período o un
 * bin de fase).
 */
struct EnergyWork {
  double w_base = 0.0;   //!< Trabajo de la base, sum dt F . v_b
  double w_lin = 0.0;    //!< Trabajo de la fricción de Karnopp
  double w_piv = 0.0;    //!< Trabajo del torque de pivoteo
  double dk = 0.0;       //!< Cambio de energía cinética dentro de los pasos
  double dk_reiny = 0.0; //!< Cambio de K por descarga y reinyección
  double k_sum = 0.0;    //!< Suma de K total al final de cada paso
  long n_steps = 0;      //!< Pasos acumulados
  long n_grains = 0;     //!< Suma del número de granos sobre los pasos

  /// Trabajo de los contactos: dK - (W_lin + W_piv).
  double w_cont() const { return dk - (w_lin + w_piv); }
  /// Suma \p o a este acumulador.
  void add(const EnergyWork &o);
};

/*! \class EnergyBudget
 * \brief Balance de energía por paso, acumulado por período de la excitación
 * y por bin de fase (ver energy_budget.hpp).
 */
class EnergyBudget {
public:
  /*! \param freq       Frecuencia de la excitación (define los períodos,
   *                    b = floor(f t), y la fase, 2 pi f t mod 2 pi)
   *  \param phase_bins Bines de fase
   */
  explicit EnergyBudget(double freq, int phase_bins = 20);

  /*! \brief Abre las salidas: \p period_file (una línea por período) y
   * \p phase_file (trabajo por período en cada bin de fase). \p header se
   * escribe al comienzo de los dos (procedencia).
   */
  void open(const std::string &period_file, const std::string &phase_file,
            const std::string &header);

  /*! \brief Guarda el estado de los granos justo antes de b2World::Step
   * (después de la descarga y la reinyección).
   * \param w        Mundo
   * \param base_vel Velocidad de la base que usó la fricción (MovBase::v; la
   *                 base se mueve con -y)
   */
  void before_step(b2World *w, double base_vel);

  /*! \brief Calcula los trabajos del paso, justo después de b2World::Step.
   * Si las salidas están abiertas, los acumula por período y por fase.
   * \param w  Mundo
   * \param t  Tiempo al inicio del paso
   * \param dt Paso temporal
   * \return Trabajos del paso
   */
  EnergyWork after_step(b2World *w, double t, double dt);

  /// Escribe el último período (posiblemente incompleto) y el perfil en fase.
  void close();

  /// true si las salidas están abiertas.
  bool active() const { return out_.is_open(); }

private:
  struct Snapshot {
    b2Body *body;
    b2Vec2 v0;
    float w0;
    b2Vec2 force;
    float torque;
  };

  void flush_period();
  static void write_row(std::ofstream &o, const std::string &prefix,
                        const EnergyWork &a, double scale);

  double freq_;
  int phase_bins_;
  std::vector<Snapshot> snap_;
  b2Vec2 base_vel_;
  double k_before_ = 0.0;   // K al inicio del paso actual
  double k_after_ = -1.0;   // K al final del paso anterior (< 0: no hay)
  std::ofstream out_;
  std::string phase_file_;
  std::string header_;      // procedencia, para el archivo por fase
  long period_ = -1;
  double t_period_start_ = 0.0, t_period_end_ = 0.0;
  double t_first_ = -1.0, t_last_ = 0.0;
  EnergyWork cur_;
  std::vector<EnergyWork> phase_acc_;
};
