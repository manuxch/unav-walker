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
#include <tuple>

GlobalSetup *gs;
RNG *rng;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cout << "Error: archivo de parámetros requerido." << std::endl;
    exit(1);
  }
  cout << "# silo-vib ver. 2.1" << endl;
  cout << "# 2025.02.10" << endl;
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
  // Definición del contenedor
  b2BodyDef bd;
  bd.position.Set(0.0f, 0.0f);
  bd.type = b2_staticBody;
  BodyData *siloD = new BodyData;
  siloD->isGrain = false;
  siloD->nLados = 5;
  siloD->gID = -100;
  bd.userData.pointer = uintptr_t(siloD);
  b2Body *silo = world->CreateBody(&bd);
  b2Vec2 wall_poly[6];
  wall_poly[0].Set(-gs->silo.r, 0.0f);
  wall_poly[1].Set(-gs->silo.R, 0.0f);
  wall_poly[2].Set(-gs->silo.R, gs->silo.H);
  wall_poly[3].Set(gs->silo.R, gs->silo.H);
  wall_poly[4].Set(gs->silo.R, 0.0);
  wall_poly[5].Set(gs->silo.r, 0.0);

  b2ChainShape siloShape_poly;
  siloShape_poly.CreateChain(wall_poly, 6, wall_poly[0], wall_poly[5]);
  b2FixtureDef silo_fix;
  silo_fix.shape = &siloShape_poly;
  silo_fix.density = 0.0f;
  silo_fix.friction = gs->silo.fric;
  silo->CreateFixture(&silo_fix);
  b2Vec2 wall_poly_2[6];
  for (int i = 0; i < 6; ++i)
    wall_poly_2[i] = wall_poly[5 - i];
  b2ChainShape siloShape_poly_2;
  siloShape_poly_2.CreateChain(wall_poly_2, 6, wall_poly_2[0], wall_poly_2[5]);
  b2FixtureDef silo_fix_2;
  silo_fix_2.shape = &siloShape_poly_2;
  silo_fix_2.density = 0.0f;
  silo_fix_2.friction = gs->silo.fric;
  silo->CreateFixture(&silo_fix_2);
  cout << "#\t- Silo creado." << endl;

  // Tapa
  b2BodyDef tapa_piso;
  tapa_piso.position.Set(0.0f, 0.0f);
  tapa_piso.type = b2_staticBody;
  BodyData *tapaP = new BodyData;
  tapaP->isGrain = false;
  tapaP->nLados = 1;
  tapaP->gID = -110;
  tapa_piso.userData.pointer = uintptr_t(tapaP);
  b2Body *tapa_P = world->CreateBody(&tapa_piso);
  b2Vec2 v1(-gs->silo.r, 0.0f);
  b2Vec2 v2(gs->silo.r, 0.0f);
  b2EdgeShape tapa_p_f;
  tapa_p_f.SetTwoSided(v1, v2);
  b2FixtureDef tapa_p_Fix;
  tapa_p_Fix.shape = &tapa_p_f;
  tapa_p_Fix.density = 0.0f;
  tapa_p_Fix.friction = gs->silo.fric;
  tapa_P->CreateFixture(&tapa_p_Fix);
  cout << "#\t- Tapa oricifio creada." << endl;

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
      bd.bullet = true;
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
  size_t pf_0[gs->n_bin_perfiles]{0}; /*!< Histograma de acumulación de pf */
  double vel_0[gs->n_bin_perfiles]{
      0.0}; /*!< Histograma de acumulación de velocidades */
  size_t bin_count[gs->n_bin_perfiles]{
      0}; /*!< Histograma de conteo de bines no nulos */

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

  // Deposición en el fondo
  cout << "# Iniciando deposición sobre fondo ..." << endl;
  while (t < gs->tBlock) {
    auto [bpos, bvel, bac] = exitacion_mm(t, gamma, w, gs);
    do_base_force(world, bvel, epsilon_v, gs->g);
    do_rot_friction(world, gs);
    // Si es necesario, guardo el frame para graficar
    /*if (saveFrm && !(nStep % gs->saveFrameFreq)) {*/
    /*  saveFrame(world, ++n_frame, nStep, gs);*/
    /*}*/
    // Si es necesario, guardamos las fuerzas de contacto
    /*if (gs->save_contact_freq && !(nStep % gs->save_contact_freq)) {*/
    /*  saveContacts(world, t, n_frame, gs);*/
    /*}*/
    // Si es necesario, guardamos el tensor de estrés
    /*if (gs->save_tensors_freq && !(nStep % gs->save_tensors_freq)) {*/
    /*  save_tensors(world, n_frame, gs);*/
    /*}*/
    // Si es necesario, guardamos el velocidades y energías
    /*if (gs->save_ve_freq && !(nStep % gs->save_ve_freq)) {*/
    /*  printVE(nStep, t, world, gs);*/
    /*}*/
    world->Step(tStep, pIter, vIter);
    world->ClearForces();
    t += tStep;
    nStep++;
  }
  cout << "# Fin tiempo de bloqueo del silo." << endl;

  // Eliminación de la tapa
  world->DestroyBody(tapa_P);
  tapa_P = NULL;
  cout << "# Tapa removida, inicio de la descarga." << endl;

  // Bucle de simulación
  t = 0.0;
  size_t n_reg = 0;
  cout << "# Inicio de la simulación ... " << endl;
  auto start_time = std::chrono::high_resolution_clock::now();
  cout << "# Fecha y hora de comienzo (UTC): " << start_time << endl;
  while (t < gs->maxT) {
    auto [bpos, bvel, bac] = exitacion_mm(t, gamma, w, gs);
    do_base_force(world, bvel, epsilon_v, gs->g);
    do_rot_friction(world, gs);
    if (t >= gs->t_register) { // Guardamos a partir de t_register
      // Si es necesario, guardo el frame para graficar
      if (saveFrm && !(nStep % gs->saveFrameFreq)) {
        saveFrame(world, ++n_frame, nStep, gs);
      }
      // Si es necesario, guardamos el pack_fraction
      if (savePF && !(nStep % gs->pf_freq)) {
        save_pf(world, gs, t, filePF);
      }

      // Si es necesario, guardamos el velocidades y energías
      if (saveVE && !(nStep % gs->save_ve_freq)) {
        printVE(n_frame, t, world, gs);
      }
      // Si es necesario, guardamos los histos pf_0 y vel_0
      if (gs->freq_perfiles && !(nStep % gs->freq_perfiles)) {
        update_pf_vx(world, vel_0, pf_0, bin_count, gs->n_bin_perfiles,
                     gs->silo.r);
        n_reg++;
      }
      // Si es necesario, guardamos las fuerzas de contacto
      if (gs->save_contact_freq && !(nStep % gs->save_contact_freq)) {
        saveContacts(world, t, n_frame, gs);
      }
      // Si es necesario, guardamos el tensor de estrés
      if (gs->save_tensors_freq && !(nStep % gs->save_tensors_freq)) {
        save_tensors(world, n_frame, gs);
      }
    }
    // Cálculo de descarga y reinyección
    deltaG = countDesc(world, sumaTipo, nStep, fileFlux, gs);
    nGranosDesc += deltaG;
    do_reinyection(world, gs);
    world->Step(tStep, pIter, vIter);
    world->ClearForces();
    t += tStep;
    nStep++;
  } // Fin bucle principal de simulación
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
  cout << "# Simulación finalizada." << endl;
  auto end_time = std::chrono::high_resolution_clock::now();
  auto elapsed_time =
      duration_cast<std::chrono::duration<double>>(end_time - start_time);
  int hours = elapsed_time.count() / 3600;
  int minutes = (elapsed_time.count() - hours * 3600) / 60;
  int seconds = elapsed_time.count() - hours * 3600 - minutes * 60;
  cout << "# Fecha y hora de finalización (UTC): " << end_time << endl;
  cout << "# Tiempo transcurrido: " << hours << " horas, " << minutes
       << " minutos, " << seconds << " segundos." << endl;
  return 0;
}
