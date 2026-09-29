/*! \file main.cpp
 * \brief unav-walkers: descarga de un silo bidimensional de discos apoyados
 * sobre una base vibrada, simulada con Box2D.
 *
 * Uso: `unav-walkers archivo_de_parámetros`
 *
 * Secuencia:
 *  1. Lectura de parámetros (GlobalSetup) y construcción del sistema
 *     (build_silo).
 *  2. Algunos pasos para resolver las superposiciones iniciales.
 *  3. Bucle principal. En cada paso: fricción de la base, salidas (desde
 *     t_Register), conteo de descargados y reinyección, y b2World::Step.
 *     Mientras t < tBlock el orificio está tapado (deposición); después se
 *     remueve la tapa. Con fondo_medicion el fondo nunca se remueve, no hay
 *     descarga y se registra la fuerza sobre el fondo en cada paso.
 *
 * Las salidas se describen en output.hpp y README.md.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "base_friction.hpp"
#include "body_data.hpp"
#include "contacts.hpp"
#include "diagnostics.hpp"
#include "discharge.hpp"
#include "git_version.hpp" // GIT_HASH, generado por CMake
#include "global_setup.hpp"
#include "output.hpp"
#include "rng.hpp"
#include "silo_builder.hpp"

#include <box2d/box2d.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using std::cout;
using std::endl;
using std::string;

namespace {

/*! Archivo de granos descargados (fluxFile). */
void open_flux_file(std::ofstream &f, const GlobalSetup &gs) {
  f.open(gs.flux_file);
  f << "# r_out: " << gs.silo.r << endl;
  f << "# grainDesc type time ";
  for (size_t i = 0; i < gs.granos.size(); ++i) {
    f << "totalType_" << i + 1 << " ";
  }
  f << " Total" << endl;
}

/*! Archivo de fuerza sobre el fondo de medición (fondo_medicion: T). */
void open_wall_force_file(std::ofstream &f, const GlobalSetup &gs) {
  string name =
      "frames_" + gs.dir_id + "/wall_force_" + gs.pre_frame_file + ".dat";
  f.open(name);
  f << "# Fuerzas de contacto de granos sobre el fondo (gID=-200)" << endl;
  f << "# Fondo: borde horizontal de x=" << -gs.silo.R << " a x=" << gs.silo.R
    << " en y=0" << endl;
  f << provenance_header(gs, 0.0, 0, 0);
  f << "# t Fx Fy |F|" << endl;
  f << std::scientific << std::uppercase << std::setprecision(5);
  cout << "# Archivo de fuerzas sobre fondo: " << name << endl;
}

/*! Archivo del chequeo de balance de impulso (check_balance_freq > 0). */
void open_balance_file(std::ofstream &f, const GlobalSetup &gs) {
  f.open("frames_" + gs.dir_id + "/balance_" + gs.pre_frame_file + ".dat");
  f << provenance_header(gs, 0.0, 0, 0);
  f << "# Residuos relativos del balance de impulso por grano "
       "(sum|res| / sum|ref|).\n"
       "# *_flip: mismo cálculo con la tangente invertida "
       "(control: debe dar residuos grandes).\n"
       "# max_rel_*, n_bad: por grano, con la referencia acotada "
       "por abajo por el impulso de fricción con la base\n"
       "#   (dt mu_d g m; por r para el balance angular). n_bad: "
       "granos con residuo relativo > 1e-2.\n";
  f << "# t nStep n_grains rel_lin max_rel_lin rel_ang max_rel_ang "
       "n_bad rel_lin_flip rel_ang_flip"
    << endl;
}

/*! Archivo de packing fraction (packing_fraction_out_freq > 0). */
void open_pf_file(std::ofstream &f, const GlobalSetup &gs) {
  f.open(gs.pf_file);
  f << "# r_out: " << gs.silo.r << endl;
  f << "# time pf_bulk pf_out" << endl;
}

/*! Tiempo transcurrido, en horas, minutos y segundos. */
void print_elapsed(std::chrono::high_resolution_clock::time_point start) {
  auto end = std::chrono::high_resolution_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
  int hours = elapsed.count() / 3600;
  int minutes = (elapsed.count() - hours * 3600) / 60;
  int seconds = elapsed.count() - hours * 3600 - minutes * 60;
  cout << "# Fecha y hora de finalización: " << local_time_string() << endl;
  cout << "# Tiempo transcurrido: " << hours << " horas, " << minutes
       << " minutos, " << seconds << " segundos." << endl;
}

} // namespace

int main(int argc, char *argv[]) {
  if (argc != 2) {
    cout << "Error: archivo de parámetros requerido.\n"
         << "Uso: " << argv[0] << " <archivo_de_parámetros>" << endl;
    return 1;
  }
  cout << "# unav-walkers ver. 3.3" << endl;
  cout << "# 2026.09.29" << endl;
  cout << "# git: " << GIT_HASH << endl;
  const GlobalSetup gs(argv[1]);
  RNG rng(gs.rnd_seed);
  std::error_code ec;
  std::filesystem::create_directories("frames_" + gs.dir_id, ec);
  cout << "# Creación de directorio de frames: " << (ec ? "NOK." : "OK.")
       << endl;
  cout << "# Creación del sistema ..." << endl;

  // Los BodyData de `bodies` deben vivir más que el mundo: se declara antes.
  SiloBodies bodies;
  b2World world(b2Vec2(0.0f, 0.0f)); // sin gravedad en el plano
  world.SetContinuousPhysics(gs.continuous_physics);
  build_silo(&world, gs, rng, bodies);

  // Parámetros del bucle
  const double dt = gs.dt;
  const double omega = 2 * b2_pi * gs.silo.frec; // frecuencia angular
  uint32_t n_step = 0;
  double t = 0.0;
  unsigned int n_granos_desc = 0;
  std::vector<int> suma_tipo(gs.granos.size(), 0); // descargados por tipo
  int n_frame = 0; // pasos desde t_Register: numera los archivos de salida
  const bool save_frm = gs.save_frame_freq > 0;
  const bool save_ve = gs.save_ve_freq > 0;
  const bool save_pf = gs.pf_freq > 0;
  // Perfiles de ocupación y velocidad sobre el orificio
  std::vector<size_t> pf_0(gs.n_bin_perfiles, 0);
  std::vector<double> vel_0(gs.n_bin_perfiles, 0.0);
  std::vector<size_t> bin_count(gs.n_bin_perfiles, 0);

  // Archivos de salida continuos
  std::ofstream flux_file, wall_force_file, balance_file, pf_file;
  if (gs.flux_freq > 0) open_flux_file(flux_file, gs);
  if (gs.fondo_medicion) open_wall_force_file(wall_force_file, gs);
  if (gs.check_balance_freq) open_balance_file(balance_file, gs);
  if (save_pf) open_pf_file(pf_file, gs);

  // Resolución de las superposiciones iniciales
  cout << "# Iniciando resolución de overlaps ..." << endl;
  for (int k = 0; k < 20; ++k) {
    world.Step(dt, gs.p_iter, gs.v_iter);
  }
  cout << "# Fin de resolución de overlaps." << endl;

  auto start_time = std::chrono::high_resolution_clock::now();
  cout << "# Fecha y hora de comienzo: " << local_time_string() << endl;
  cout << "# Iniciando deposición sobre fondo ..." << endl;
  size_t n_reg = 0;      // registros de los perfiles en el orificio
  double p_min = 1.0e8;  // presión mínima registrada en los .sxy
  double p_max = -1.0e8; // presión máxima registrada en los .sxy
  const bool stop_by_grains = gs.max_granos_desc > 0;
  bool blocked = true;
  const bool any_output =
      save_frm || save_ve || gs.save_contact_freq || gs.save_tensors_freq;
  auto every = [&n_step](int freq) { return freq && !(n_step % freq); };

  while (t < gs.t_max &&
         !(stop_by_grains &&
           n_granos_desc >= static_cast<unsigned int>(gs.max_granos_desc))) {
    if (blocked && t >= gs.t_block) {
      blocked = false;
      if (!gs.fondo_medicion) {
        world.DestroyBody(bodies.lid);
        bodies.lid = nullptr;
        cout << "# Tapa removida, inicio de la descarga." << endl;
      } else {
        cout << "# Fin de la deposición; el fondo de medición permanece."
             << endl;
      }
    }
    MovBase base = base_excitation(t, gs.silo.gamma, omega, gs);
    apply_base_friction(&world, base.v, base.a, gs.silo.zero_tol, gs.g, dt);
    if (t >= gs.t_register) {
      if (any_output) ++n_frame;
      if (save_frm && every(gs.save_frame_freq))
        save_frame(&world, n_frame, n_step, gs);
      if (save_pf && every(gs.pf_freq))
        save_packing_fraction(&world, gs, t, pf_file);
      if (save_ve && every(gs.save_ve_freq))
        save_velocities(n_frame, t, n_step, &world, gs);
      if (every(gs.freq_perfiles)) {
        update_outlet_profiles(&world, vel_0.data(), pf_0.data(),
                               bin_count.data(), gs.n_bin_perfiles, gs.silo.r);
        n_reg++;
      }
      if (every(gs.save_contact_freq))
        save_contacts(&world, t, n_step, n_frame, gs);
      if (every(gs.save_tensors_freq))
        save_stress(&world, n_frame, gs, &p_min, &p_max, t, n_step);
    }
    // Descarga y reinyección
    if (!blocked && !gs.fondo_medicion) {
      n_granos_desc += count_discharged(
          &world, suma_tipo, static_cast<int>(n_step), flux_file, gs);
      reinject_grains(&world, gs, rng, gs.reinyeccion);
    }
    const bool check_now = every(gs.check_balance_freq);
    if (check_now) record_pre_step(&world);
    world.Step(dt, gs.p_iter, gs.v_iter);
    if (check_now) check_force_balance(&world, gs, t, n_step, balance_file);
    if (gs.fondo_medicion) {
      b2Vec2 wf = wall_force(&world, dt, kGidMeasuringFloor);
      wall_force_file << t << " " << wf.x << " " << wf.y << " " << wf.Length()
                      << "\n";
    }
    world.ClearForces();
    t += dt;
    n_step++;
  }

  if (stop_by_grains &&
      n_granos_desc >= static_cast<unsigned int>(gs.max_granos_desc))
    cout << "# Simulación finalizada por número de granos descargados (N = "
         << n_granos_desc << ")." << endl;
  else
    cout << "# Simulación finalizada por tiempo máximo." << endl;

  // Perfiles de packing fraction y velocidad sobre el orificio
  if (gs.freq_perfiles) {
    cout << "# r pf_0 vel_0" << endl;
    double delta_r = 2.0 * gs.silo.r / gs.n_bin_perfiles;
    for (int i = 0; i < gs.n_bin_perfiles; ++i) {
      cout << i * delta_r + delta_r / 2.0 - gs.silo.r << " "
           << pf_0[i] / double(n_reg) << " " << vel_0[i] / double(bin_count[i])
           << endl;
    }
  }
  pf_file.close();
  flux_file.close();
  wall_force_file.close();
  balance_file.close();
  cout << "# Simulación finalizada." << endl;
  print_elapsed(start_time);
  if (gs.save_tensors_freq) {
    cout << "# Presión mínima registrada: " << p_min << endl;
    cout << "# Presión máxima registrada: " << p_max << endl;
  }
  return 0;
}
