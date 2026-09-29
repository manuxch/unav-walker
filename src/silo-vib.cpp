#include <iostream>
using std::cout;
using std::endl;
#include "globalsetup.hpp"
#include "rng.hpp"
#include "siloAux.hpp"
#include <box2d/box2d.h>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include <vector>

void comprehensiveCheck(b2World* world, int step);

GlobalSetup *gs;
RNG *rng;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Error: archivo de parámetros requerido." << std::endl;
    exit(1);
  }
  cout << "# silo-vib ver. 3.0" << endl;
  cout << "# 2026.09.29" << endl;
#ifdef GIT_HASH
  cout << "# git: " << GIT_HASH << endl;
#endif
  gs = new GlobalSetup{argv[1]};
  rng = new RNG(gs->rnd_seed);
  string folder_cmd = "mkdir -p frames_" + gs->dirID;
  int sysret = system(folder_cmd.c_str());
  cout << "# Creación de directorio de frames: " << (sysret ? "NOK." : "OK.")
       << endl;
  cout << "# Creación del sistema ..." << endl;

  b2Vec2 gravedad;
  gravedad.Set(0.0f, 0.0f);
  b2World *world = new b2World(gravedad);
  world->SetContinuousPhysics(gs->continuous_physics);
  // Definición del contenedor.
  // Silo normal: cadena de 6 vértices con orificio [-r, r] en y = 0, más una
  // tapa (gID=-110) que se remueve en t = tBlock.
  // fondo_medicion: cadena en U sin fondo, más un fondo de ancho completo
  // (gID=-200) que nunca se remueve y sobre el que se mide la fuerza.
  b2BodyDef bd;
  bd.position.Set(0.0f, 0.0f);
  bd.type = b2_staticBody;
  BodyData *siloD = new BodyData;
  siloD->isGrain = false;
  siloD->gID = -100;
  bd.userData.pointer = uintptr_t(siloD);
  b2Body *silo = world->CreateBody(&bd);
  std::vector<b2Vec2> wall_poly;
  if (gs->fondo_medicion) {
    wall_poly = {b2Vec2(-gs->silo.R, 0.0f), b2Vec2(-gs->silo.R, gs->silo.H),
                 b2Vec2(gs->silo.R, gs->silo.H), b2Vec2(gs->silo.R, 0.0f)};
  } else {
    wall_poly = {b2Vec2(-gs->silo.r, 0.0f), b2Vec2(-gs->silo.R, 0.0f),
                 b2Vec2(-gs->silo.R, gs->silo.H), b2Vec2(gs->silo.R, gs->silo.H),
                 b2Vec2(gs->silo.R, 0.0f), b2Vec2(gs->silo.r, 0.0f)};
  }
  siloD->nLados = static_cast<int>(wall_poly.size()) - 1;
  const int n_wall = static_cast<int>(wall_poly.size());
  // Las cadenas colisionan de un solo lado: se crean las dos orientaciones.
  for (int side = 0; side < 2; ++side) {
    std::vector<b2Vec2> verts(wall_poly);
    if (side == 1) std::reverse(verts.begin(), verts.end());
    b2ChainShape chain;
    chain.CreateChain(verts.data(), n_wall, verts.front(), verts.back());
    b2FixtureDef silo_fix;
    silo_fix.shape = &chain;
    silo_fix.density = 0.0f;
    silo_fix.friction = gs->silo.fric;
    silo_fix.restitution = gs->silo.rest;
    silo->CreateFixture(&silo_fix);
  }
  cout << "#\t- Silo creado." << endl;

  // Tapa del orificio (gID=-110) o fondo de medición (gID=-200)
  b2BodyDef tapa_piso;
  tapa_piso.position.Set(0.0f, 0.0f);
  tapa_piso.type = b2_staticBody;
  BodyData *tapaP = new BodyData;
  tapaP->isGrain = false;
  tapaP->nLados = 1;
  tapaP->gID = gs->fondo_medicion ? -200 : -110;
  tapa_piso.userData.pointer = uintptr_t(tapaP);
  b2Body *tapa_P = world->CreateBody(&tapa_piso);
  const float x_tapa = static_cast<float>(gs->fondo_medicion ? gs->silo.R : gs->silo.r);
  b2EdgeShape tapa_p_f;
  tapa_p_f.SetTwoSided(b2Vec2(-x_tapa, 0.0f), b2Vec2(x_tapa, 0.0f));
  b2FixtureDef tapa_p_Fix;
  tapa_p_Fix.shape = &tapa_p_f;
  tapa_p_Fix.density = 0.0f;
  tapa_p_Fix.friction = gs->silo.fric;
  tapa_p_Fix.restitution = gs->silo.rest;
  tapa_P->CreateFixture(&tapa_p_Fix);
  if (gs->fondo_medicion)
    cout << "#\t- Fondo de medición creado (gID=-200)." << endl;
  else
    cout << "#\t- Tapa del orificio creada (gID=-110)." << endl;

  // Generación de granos.
  float siloInf, siloSup, siloIzq, siloDer, x, y;
  float maxRadio = 0.0f;
  for (int i = 0; i < gs->noTipoGranos; ++i) {
    if (gs->granos[i]->radio > maxRadio)
      maxRadio = gs->granos[i]->radio;
  }
  siloInf = 2.7f * maxRadio;
  siloSup = gs->silo.H - 2.1f * maxRadio;
  siloIzq = -gs->silo.R + 2.1f * maxRadio;
  siloDer = gs->silo.R - 2.1f * maxRadio;

  BodyData **gInfo;
  gInfo = new BodyData *[gs->noTipoGranos];

  int contGid = 0;
  int *sumaTipo = new int[gs->noTipoGranos]{};
  double total_grain_mass = 0.0;
  cout << "#\t- Insertando granos..." << endl;
  for (int i = 0; i < gs->noTipoGranos; i++) { // Loop sobre tipos de granos.
    gInfo[i] = new BodyData[gs->granos[i]->noGranos];
    for (int j = 0; j < gs->granos[i]->noGranos; j++) { // Loop
      // sobre el número de granos de cada tipo.
      x = rng->get_double(siloIzq, siloDer);
      // Insersión de granos uniforme para todos los tipos
      y = rng->get_double(siloInf, siloSup);
      // Segregación inicial de granos
      // y = i * rng->get_double(siloInf, 0.9 * gs->silo.H / 2.0)
      //+ (1 - i) * rng->get_double(1.1 * gs->silo.H / 2.0, siloSup);
      gInfo[i][j].tipo = i;
      gInfo[i][j].isGrain = true;
      gInfo[i][j].isIn = true;
      gInfo[i][j].nLados = gs->granos[i]->nLados;
      gInfo[i][j].fric_d = gs->granos[i]->fric_d;
      gInfo[i][j].fric_s = gs->granos[i]->fric_s;
      gInfo[i][j].gID = contGid++;
      b2BodyDef bd;
      bd.type = b2_dynamicBody;
      bd.allowSleep = true;
      bd.bullet = gs->continuous_physics;
      bd.position.Set(x, y);
      bd.angle = rng->get_double(-b2_pi, b2_pi);
      bd.userData.pointer = reinterpret_cast<uintptr_t>(&gInfo[i][j]);
      b2Body *grain = world->CreateBody(&bd);
      if (gs->granos[i]->nLados == 1) {
        b2CircleShape circle;
        circle.m_radius = gs->granos[i]->radio;
        b2FixtureDef fixDef;
        fixDef.shape = &circle;
        fixDef.density = gs->granos[i]->dens;
        fixDef.friction = gs->granos[i]->fric;
        fixDef.restitution = gs->granos[i]->rest;
        grain->CreateFixture(&fixDef);
        total_grain_mass += grain->GetMass();
      } else {
        b2PolygonShape poly;
        int32 vertexCount = gs->granos[i]->nLados;
        b2Vec2 vertices[8];
        for (int k = 0; k < gs->granos[i]->nLados; k++)
          vertices[k].Set(gs->granos[i]->vertices[k][0],
                          gs->granos[i]->vertices[k][1]);
        poly.Set(vertices, vertexCount);
        b2FixtureDef fixDef;
        fixDef.shape = &poly;
        fixDef.density = gs->granos[i]->dens;
        fixDef.friction = gs->granos[i]->fric;
        fixDef.restitution = gs->granos[i]->rest;
        grain->CreateFixture(&fixDef);
        total_grain_mass += grain->GetMass();
      }
      if (j == 0) {
        cout << "#\t- Grano de tipo " << i << " creado con masa "
             << grain->GetMass() << " kg." << endl;
      }
    } // Fin loop sobre el número de granos de cada tipo.
  } // Fin loop sobre tipo de granos
  cout << "#\t- Insersión de granos finalizada." << endl;
  cout << "#\t- Masa total de granos = " << total_grain_mass << " kg." << endl;

  // Preparo parámetros de simulación
  double tStep = gs->tStep;
  int pIter = gs->pIter;
  int vIter = gs->vIter;
  double freqHz = gs->silo.frec;
  double w = 2 * b2_pi * freqHz;
  double gamma = gs->silo.Gamma;
  uint32_t nStep = 0;
  double t = 0.0;
  unsigned int deltaG = 0, nGranosDesc = 0;
  double epsilon_v = gs->silo.zero_tol;
  bool saveFrm = (gs->saveFrameFreq > 0 ? true : false);
  bool saveVE = (gs->save_ve_freq > 0 ? true : false);
  bool saveFlux = (gs->fluxFreq > 0 ? true : false);
  bool savePF = (gs->pf_freq > 0 ? true : false);
  int n_frame = 0;
  std::vector<size_t> pf_0(gs->n_bin_perfiles, 0);  /*!< Histograma de pf */
  std::vector<double> vel_0(gs->n_bin_perfiles, 0.0); /*!< Histograma de vel. */
  std::vector<size_t> bin_count(gs->n_bin_perfiles, 0); /*!< Bines no nulos */

  // Preparo salida de flujo
  std::ofstream fileFlux;
  if (saveFlux) {
    fileFlux.open((gs->fluxFile).c_str());
    fileFlux << "# r_out: " << gs->silo.r << endl;
    fileFlux << "# grainDesc type time ";
    for (int i = 0; i < gs->noTipoGranos; ++i) {
      fileFlux << "totalType_" << i + 1 << " ";
    }
    fileFlux << " Total" << endl;
  }
  // Salida de fuerza sobre el fondo de medición
  std::ofstream wallForceFile;
  if (gs->fondo_medicion) {
    string wf_filename =
        "frames_" + gs->dirID + "/wall_force_" + gs->preFrameFile + ".dat";
    wallForceFile.open(wf_filename.c_str());
    wallForceFile << "# Fuerzas de contacto de granos sobre el fondo (gID=-200)"
                  << endl;
    wallForceFile << "# Fondo: borde horizontal de x=" << -gs->silo.R
                  << " a x=" << gs->silo.R << " en y=0" << endl;
    wallForceFile << provenance_header(gs, 0.0, 0, 0);
    wallForceFile << "# t Fx Fy |F|" << endl;
    wallForceFile << std::scientific << std::uppercase << std::setprecision(5);
    cout << "# Archivo de fuerzas sobre fondo: " << wf_filename << endl;
  }
  // Salida del chequeo de balance de fuerzas
  std::ofstream balanceFile;
  if (gs->check_balance_freq) {
    string bf = "frames_" + gs->dirID + "/balance_" + gs->preFrameFile + ".dat";
    balanceFile.open(bf.c_str());
    balanceFile << provenance_header(gs, 0.0, 0, 0);
    balanceFile << "# Residuos relativos del balance de impulso por grano "
                   "(sum|res| / sum|ref|).\n"
                   "# *_flip: mismo cálculo con la tangente invertida "
                   "(control: debe dar residuos grandes).\n"
                   "# max_rel_*, n_bad: por grano, con la referencia acotada "
                   "por abajo por el impulso de fricción con la base\n"
                   "#   (dt mu_d g m; por r para el balance angular). n_bad: "
                   "granos con residuo relativo > 1e-2.\n";
    balanceFile << "# t nStep n_grains rel_lin max_rel_lin rel_ang max_rel_ang "
                   "n_bad rel_lin_flip rel_ang_flip" << endl;
  }
  // Preparo salida de packing fraction
  std::ofstream filePF;
  if (savePF) {
    filePF.open((gs->pf_file).c_str());
    filePF << "# r_out: " << gs->silo.r << endl;
    filePF << "# time pf_bulk pf_out" << endl;
  }

  // Resuelvo los overlap iniciales
  cout << "# Iniciando resolución de overlaps ..." << endl;
  int overlap_steps = 20;
  while (overlap_steps--) {
    world->Step(tStep, pIter, vIter);
  }
  cout << "# Fin de resolución de overlaps." << endl;

  // Bucle de simulación. Mientras t < tBlock el orificio está tapado
  // (deposición); luego se remueve la tapa y comienza la descarga. Con
  // fondo_medicion el fondo nunca se remueve y no hay descarga.
  auto start_time = std::chrono::high_resolution_clock::now();
  cout << "# Fecha y hora de comienzo: " << get_local_time() << endl;
  cout << "# Iniciando deposición sobre fondo ..." << endl;
  size_t n_reg = 0;
  double p_min = 1.0e8;
  double p_max = -1.0e8; // Presiones mínima y máxima durante la simulación.
  bool stop_by_grains = (gs->maxGranosDesc > 0);
  bool blocked = true;
  const bool any_output = saveFrm || saveVE || gs->save_contact_freq ||
                          gs->save_tensors_freq;
  while (t < gs->maxT &&
         !(stop_by_grains && nGranosDesc >= (unsigned int)gs->maxGranosDesc)) {
    if (blocked && t >= gs->tBlock) {
      blocked = false;
      if (!gs->fondo_medicion) {
        world->DestroyBody(tapa_P);
        tapa_P = nullptr;
        cout << "# Tapa removida, inicio de la descarga." << endl;
      } else {
        cout << "# Fin de la deposición; el fondo de medición permanece."
             << endl;
      }
    }
    auto [bpos, bvel, bac] = exitacion_mm(t, gamma, w, gs);
    do_base_force(world, bvel, bac, epsilon_v, gs->g, tStep);
    if (t >= gs->t_register) { // Guardamos a partir de t_register
      // n_frame cuenta pasos desde t_register: todos los archivos de un mismo
      // instante comparten el mismo número de frame.
      if (any_output) ++n_frame;
      if (saveFrm && !(nStep % gs->saveFrameFreq)) {
        saveFrame(world, n_frame, nStep, gs);
      }
      if (savePF && !(nStep % gs->pf_freq)) {
        save_pf(world, gs, t, filePF);
      }
      if (saveVE && !(nStep % gs->save_ve_freq)) {
        printVE(n_frame, t, nStep, world, gs);
      }
      if (gs->freq_perfiles && !(nStep % gs->freq_perfiles)) {
        update_pf_vx(world, vel_0.data(), pf_0.data(), bin_count.data(),
                     gs->n_bin_perfiles, gs->silo.r);
        n_reg++;
      }
      if (gs->save_contact_freq && !(nStep % gs->save_contact_freq)) {
        saveContacts(world, t, nStep, n_frame, gs);
      }
      if (gs->save_tensors_freq && !(nStep % gs->save_tensors_freq)) {
        save_tensors(world, n_frame, gs, &p_min, &p_max, t, nStep);
      }
    }
    // Cálculo de descarga y reinyección
    if (!blocked && !gs->fondo_medicion) {
      deltaG = countDesc(world, sumaTipo, nStep, fileFlux, gs);
      nGranosDesc += deltaG;
      do_reinyection(world, gs, gs->reinyection);
    }
    const bool check_now =
        gs->check_balance_freq && !(nStep % gs->check_balance_freq);
    if (check_now) record_pre_step(world);
    world->Step(tStep, pIter, vIter);
    if (check_now) check_force_balance(world, gs, t, nStep, balanceFile);
    if (gs->fondo_medicion) {
      b2Vec2 wf = compute_wall_force(world, gs, -200);
      wallForceFile << t << " " << wf.x << " " << wf.y << " " << wf.Length()
                    << "\n";
    }
    world->ClearForces();
    t += tStep;
    nStep++;
  } // Fin bucle principal de simulación
  // Motivo de parada
  if (stop_by_grains && nGranosDesc >= (unsigned int)gs->maxGranosDesc)
    cout << "# Simulación finalizada por número de granos descargados (N = "
         << nGranosDesc << ")." << endl;
  else
    cout << "# Simulación finalizada por tiempo máximo." << endl;
  //
  // Guardado de perfiles de velocidad y packing-fraction
  if (gs->freq_perfiles) {
    cout << "# r pf_0 vel_0" << endl;
    double delta_r = 2.0 * gs->silo.r / gs->n_bin_perfiles;
    for (int i = 0; i < gs->n_bin_perfiles; ++i) {
      cout << i * delta_r + delta_r / 2.0 - gs->silo.r << " "
           << pf_0[i] / double(n_reg) << " " << vel_0[i] / double(bin_count[i])
           << endl;
    }
  }
  filePF.close();
  fileFlux.close();
  wallForceFile.close();
  balanceFile.close();
  cout << "# Simulación finalizada." << endl;
  auto end_time = std::chrono::high_resolution_clock::now();
  auto elapsed_time =
      duration_cast<std::chrono::duration<double>>(end_time - start_time);
  int hours = elapsed_time.count() / 3600;
  int minutes = (elapsed_time.count() - hours * 3600) / 60;
  int seconds = elapsed_time.count() - hours * 3600 - minutes * 60;
  cout << "# Fecha y hora de finalización: " << get_local_time() << endl;
  cout << "# Tiempo transcurrido: " << hours << " horas, " << minutes
       << " minutos, " << seconds << " segundos." << endl;
  if (gs->save_tensors_freq) {
    cout << "# Presión mínima registrada: " << p_min << endl;
    cout << "# Presión máxima registrada: " << p_max << endl;
  }
  return 0;
}


// Ejemplo de chequeo extensivo
void comprehensiveCheck(b2World* world, int step) {
    cout << "=== Check paso " << step << " ===" << endl;
    
    for (b2Body* b = world->GetBodyList(); b; b = b->GetNext()) {
        BodyData* bd = (BodyData*)b->GetUserData().pointer;
        if (!bd || !bd->isGrain) continue;
        
        b2Vec2 pos = b->GetPosition();
        b2Vec2 vel = b->GetLinearVelocity();
        
        bool hasNaN = false;
        if (std::isnan(pos.x)) { cout << "gID " << bd->gID << ": pos.x NaN" << endl; hasNaN = true; }
        if (std::isnan(pos.y)) { cout << "gID " << bd->gID << ": pos.y NaN" << endl; hasNaN = true; }
        if (std::isnan(vel.x)) { cout << "gID " << bd->gID << ": vel.x NaN" << endl; hasNaN = true; }
        if (std::isnan(vel.y)) { cout << "gID " << bd->gID << ": vel.y NaN" << endl; hasNaN = true; }
        
        if (hasNaN) {
            cout << "  Posición: (" << pos.x << ", " << pos.y << ")" << endl;
            cout << "  Velocidad: (" << vel.x << ", " << vel.y << ")" << endl;
        }
    }
}
