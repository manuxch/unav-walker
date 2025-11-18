/*! \file siloAux.cpp
 * \brief Archivo de implementación de funciones auxiliares
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 *
 * \version 1.0 Versión inicial
 *
 * \date 2022.07.25
 */

#include "siloAux.hpp"

std::string int2str(int num) {
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(6) << num;
  return oss.str();
}

bool isActive(b2World *w) {
  // BodyData* infGr;
  for (b2Body *bd = w->GetBodyList(); bd; bd = bd->GetNext()) {
    // infGr = (BodyData*) (bd->GetUserData()).pointer;
    if (bd->IsAwake())
      return true;
    // if (infGr->isGrain && infGr->isIn && bd->IsAwake()) return true;
  }
  return false;
}

void savePart(b2World *w, int file_id, const GlobalSetup *globalSetup) {
  string file_name = "frames_" + globalSetup->dirID + "/particles_info_" +
                     int2str(file_id) + ".dat";
  std::ofstream ff;
  ff.open(file_name.c_str());
  BodyData *infGr;
  b2Vec2 p;
  float angle;
  for (b2Body *bd = w->GetBodyList(); bd; bd = bd->GetNext()) {
    infGr = (BodyData *)(bd->GetUserData()).pointer;
    if (infGr->isGrain) {
      p = bd->GetPosition();
      angle = bd->GetAngle();
      ff << infGr->gID << " " << p.x << " " << p.y << " " << angle << " "
         << endl;
    }
  }
  ff << std::flush;
  ff.close();
}

void saveFrame(b2World *w, int n_frame, int frm_id,
               const GlobalSetup *globalSetup) {
  float xtmp, ytmp;
  string file_name = "frames_" + globalSetup->dirID + "/" +
                     globalSetup->preFrameFile + "_" + int2str(n_frame) + ".xy";
  std::ofstream fileF;
  fileF.open(file_name.c_str());
  fileF << "# time: " << frm_id * globalSetup->tStep << " ";
  fileF << "# r_out: " << globalSetup->silo.r << " ";
  fileF << endl;
  for (b2Body *bd = w->GetBodyList(); bd; bd = bd->GetNext()) {
    BodyData *infGr = (BodyData *)(bd->GetUserData()).pointer;
    // if (infGr->gID == -110 || infGr->gID == -120)
    // continue;  // no guardo las coordenadas de la tapa.
    if (infGr->isGrain) {
      fileF << infGr->gID << " ";
      if (infGr->nLados > 1) { // Es un polígono
        b2Fixture *f = bd->GetFixtureList();
        b2Shape *shape = f->GetShape();
        b2PolygonShape *poly = (b2PolygonShape *)shape;
        int count = poly->m_count;
        fileF << count << " ";
        b2Vec2 *verts = (b2Vec2 *)poly->m_vertices;
        for (int i = 0; i < count; ++i) {
          xtmp = bd->GetWorldPoint(verts[i]).x;
          ytmp = bd->GetWorldPoint(verts[i]).y;
          fileF << xtmp << " " << ytmp << " ";
        }
      }
      if (infGr->nLados == 1) { // Es un círculo
        fileF << "1 ";
        b2Vec2 pos = bd->GetPosition();
        b2Fixture *f = bd->GetFixtureList();
        b2Shape *bs = (b2Shape *)f->GetShape();
        float radio = bs->m_radius;
        fileF << pos.x << " " << pos.y << " " << radio << " ";
      }
      fileF << infGr->tipo << " ";
      fileF << endl;
    } else if (infGr->gID == -110) { // tapa
      b2Fixture *f = bd->GetFixtureList();
      b2EdgeShape *s = (b2EdgeShape *)f->GetShape();
      b2Vec2 verts[2];
      verts[0] = s->m_vertex1;
      verts[1] = s->m_vertex2;
      fileF << infGr->gID << " ";
      fileF << 2 << " ";
      verts[0] = bd->GetWorldPoint(verts[0]);
      fileF << verts[0].x << " " << verts[0].y << " ";
      verts[1] = bd->GetWorldPoint(verts[1]);
      fileF << verts[1].x << " " << verts[1].y << " ";
      fileF << (infGr->gID == -110 ? "LID-F" : "LID-W") << endl;
    } else { // Es la caja
      for (b2Fixture *f = bd->GetFixtureList(); f; f = f->GetNext()) {
        // fileF << infGr->gID << " ";
        // b2ChainShape *s = (b2ChainShape *)f->GetShape();
        // b2Vec2 *verts = (b2Vec2 *)s->m_vertices;
        // fileF << s->m_count << " ";
        // for (int i = 0; i < s->m_count; ++i) {
        //   verts[i] = bd->GetWorldPoint(verts[i]);
        //   fileF << verts[i].x << " " << verts[i].y << " ";
        // }
        // fileF << "LINE" << endl;
        b2ChainShape *s = (b2ChainShape *)f->GetShape();
        b2Vec2 *verts = (b2Vec2 *)s->m_vertices;
        for (int i = 0; i < s->m_count - 1; ++i) {  // Recorro los segmentos
            fileF << infGr->gID << " 2 ";
            fileF << verts[i].x << " " << verts[i].y << " ";
            fileF << verts[i + 1].x << " " << verts[i + 1].y;
            fileF << " LINE" << endl;
        }
      }
    }
  }
  fileF.close();
}

int countDesc(b2World *w, int *st, int paso, std::ofstream &fluxFile,
              const GlobalSetup *gs) {
  int granoDesc = 0; // Granos totales descargados en la trayectoria
  double tStep = gs->tStep;
  for (int i = 0; i < gs->noTipoGranos; ++i)
    granoDesc += st[i];
  BodyData *infGr;
  b2Vec2 p, pv;
  double y_min = 0.0, radio;
  int nGranos = 0; // granos descargados en este check
  // int sumaTotal = 0;
  for (b2Body *bd = w->GetBodyList(); bd; bd = bd->GetNext()) {
    infGr = (BodyData *)(bd->GetUserData()).pointer;
    if (infGr->isGrain && infGr->isIn) {
      p = bd->GetPosition();
      b2Fixture *fixt = bd->GetFixtureList();
      b2Shape *shape = fixt->GetShape();
      if (infGr->nLados == 1) {
        radio = shape->m_radius;
      } else {
        b2PolygonShape *poly = (b2PolygonShape *)shape;
        b2Vec2 *verts = (b2Vec2 *)poly->m_vertices;
        pv = b2Vec2(verts[0].x - p.x, verts[0].y - p.y);
        radio = pv.Length();
      }
      if (p.y > y_min - radio)
        continue;
      infGr->isIn = false;
      nGranos++;
      granoDesc++;
      st[infGr->tipo]++;
      fluxFile << granoDesc << " " << infGr->tipo << " ";
      fluxFile << std::setprecision(8) << paso * tStep << " ";
      for (int i = 0; i < gs->noTipoGranos; ++i) {
        // sumaTotal += st[i];
        fluxFile << st[i] << " ";
      }
      fluxFile << granoDesc << endl;
    }
  }
  fluxFile.flush();
  return nGranos;
}

void printVE(const int frm_id, const float timeS, b2World *w,
             const GlobalSetup *gs) {
  b2Vec2 pi, vi;
  float wi, mi, Ii, vim;
  b2Vec2 vt(0.0, 0.0);
  string file_name = "frames_" + gs->dirID + "/" + gs->preFrameFile + "_" +
                     int2str(frm_id) + ".ve";
  std::ofstream fileF;
  fileF.open(file_name.c_str());
  fileF << "# gID type x y vx vy w E_kin_lin E_kin_rot "
        << " ## sim_time: " << timeS << endl;
  for (b2Body *bi = w->GetBodyList(); bi; bi = bi->GetNext()) {
    BodyData *igi = (BodyData *)(bi->GetUserData()).pointer;
    if (!igi->isGrain) {
      continue;
    }
    pi = bi->GetPosition();
    vi = bi->GetLinearVelocity();
    vim = vi.Length();
    wi = bi->GetAngularVelocity();
    mi = bi->GetMass();
    Ii = bi->GetInertia();
    fileF << igi->gID << " " << igi->tipo << " " << pi.x << " " << pi.y << " "
          << vi.x << " " << vi.y << " " << wi << " " << 0.5 * mi * vim * vim
          << " " << 0.5 * Ii * wi * wi << endl;
  }
  fileF.close();
}

void saveContacts(b2World *w, float ts, int file_id,
                  const GlobalSetup *globalSetup) {
  string file_name = "frames_" + globalSetup->dirID + "/force_contact_" +
                     int2str(file_id) + ".dat";
  std::ofstream ff;
  ff.open(file_name.c_str());
  ff << "# Time:  " << ts << endl;
  ff << "# gID_A gID_B cp.x cp.y norm tan n_pc " << endl;
  float norm, tang;
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching())
      continue;
    int numPoints = c->GetManifold()->pointCount;
    b2WorldManifold worldManifold;
    c->GetWorldManifold(&worldManifold);
    b2Body *bodyA = c->GetFixtureA()->GetBody();
    b2Body *bodyB = c->GetFixtureB()->GetBody();
    BodyData *bdgdA, *bdgdB;
    bdgdA = (BodyData *)(bodyA->GetUserData()).pointer;
    bdgdB = (BodyData *)(bodyB->GetUserData()).pointer;
    for (int i = 0; i < numPoints; i++) {
      ff << bdgdA->gID << " " << bdgdB->gID << " " << worldManifold.points[i].x
         << " " << worldManifold.points[i].y << " ";
      norm = (c->GetManifold())->points[i].normalImpulse;
      tang = (c->GetManifold())->points[i].tangentImpulse;
      ff << norm / ts << " " << tang / ts << " CP" << numPoints << endl;
    }
  }
  ff << std::flush;
  ff.close();
}

b2Vec2 karnopp(b2Vec2 v, double v_tol, double mu_s, double mu_d, double p) {
  double v_norm = v.Length();
  double fr_norm = 0.0f;
  if (v_norm < v_tol) {
    try {
      fr_norm = mu_s * p / v_norm;
    } catch (const ::std::overflow_error &e) {
      std::cerr << "Error de overflow: " << e.what() << std::endl;
    } catch (const ::std::domain_error &e) {
      std::cerr << "Error de division por cero: " << e.what() << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Error desconocido: " << e.what() << endl;
      exit(1);
    }
    fr_norm = 0.0f;
  } else {
    fr_norm = mu_d * p / v_norm;
  }
  return -fr_norm * v;
}

b2Vec2 smooth_coulomb(b2Vec2 v, double v_d, double mu_d, double p) {
  double v_norm = v.Length();
  double fr_norm = 0.0f;
  try {
    fr_norm = mu_d * p * std::tanh(v_norm / v_d) / v_norm;
  } catch (const ::std::overflow_error &e) {
    std::cerr << "Error de overflow: " << e.what() << std::endl;
    fr_norm = 0.0f;
  } catch (const ::std::domain_error &e) {
    std::cerr << "Error de division por cero: " << e.what() << std::endl;
    fr_norm = 0.0f;
  } catch (const std::exception &e) {
    std::cerr << "Error desconocido: " << e.what() << endl;
    exit(1);
  }
  return -fr_norm * v;
}

b2Vec2 smooth_coulomb_2(b2Vec2 v, double v_d, double v_s, double mu_d,
                        double mu_s, double p) {
  double v_norm = v.Length();
  double fr_norm = 0.0f;
  try {
    fr_norm = mu_d * p * std::tanh(v_norm / v_d) / v_norm +
              (mu_s - mu_d) * v_norm / v_s *
                  std::exp(-(v_norm / v_s) * (v_norm / v_s));
  } catch (const ::std::overflow_error &e) {
    std::cerr << "Error de overflow: " << e.what() << std::endl;
    fr_norm = 0.0f;
  } catch (const ::std::domain_error &e) {
    std::cerr << "Error de division por cero: " << e.what() << std::endl;
    fr_norm = 0.0f;
  } catch (const std::exception &e) {
    std::cerr << "Error desconocido: " << e.what() << endl;
    exit(1);
  }
  return -fr_norm * v;
}

// Función que produce una exitación bi-armónica (desde aceleración como en el
// paper MM)
Mov_Base exitacion_mm(double t, double gamma, double w, const GlobalSetup *gs) {
  double y, vy, ay, rho = gs->silo.rho, phi = gs->silo.phi;
  double Aa = gamma * gs->g;
  double Av = Aa / w;
  double Ax = Av / w;
  y = -rho * Ax * std::sin(w * t) -
      (1 - rho) / 4.0 * Ax * std::sin(2 * w * t + phi);
  vy = -rho * Av * std::cos(w * t) -
       (1 - rho) / 2.0 * Av * std::cos(2 * w * t + phi);
  ay = rho * Aa * std::sin(w * t) + (1 - rho) * Aa * std::sin(2 * w * t + phi);
  return {y, vy, ay};
}

void do_base_force(b2World *w, double bvel, double epsilon_v, double g) {
  double mu_ss, mu_dd, peso_grano;
  BodyData *bdata;
  b2Vec2 vel_movil, vrel, F_roce;
  b2Vec2 base_vel_vec(0.0f, -bvel);
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) {
      continue;
    }
    vel_movil = b->GetLinearVelocity();
    peso_grano = g * b->GetMass();
    bdata = reinterpret_cast<BodyData *>(b->GetUserData().pointer);
    mu_ss = bdata->fric_s;
    mu_dd = bdata->fric_d;
    vrel = vel_movil - base_vel_vec;
    F_roce = karnopp(vrel, epsilon_v, mu_ss, mu_dd, peso_grano);
    b->ApplyForceToCenter(F_roce, true);
  }
  return;
}

void do_rot_friction(b2World *w, const GlobalSetup *gs) {
  double fw = gs->silo.at_rotac;
  double w_vel;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) {
      continue;
    }
    w_vel = b->GetAngularVelocity();
    if (abs(w_vel) < gs->silo.zero_tol) {
      /*b->SetAngularVelocity(0.0f);*/
      continue;
    } else {
      b->SetAngularVelocity(fw * w_vel);
    }
  }
  return;
}

void do_reinyection(b2World *w, GlobalSetup *gs) {
  b2Vec2 pos;
  BodyData *infGr;
  // double r_elim = -gs->silo.R;
  double r_elim = -10.0;
  double x_new, y_new, angle;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) {
      continue;
    }
    infGr = (BodyData *)(b->GetUserData()).pointer;
    if (infGr->isIn)
      continue;
    pos = b->GetPosition();
    if (pos.y > r_elim)
      continue;
    angle = b->GetAngle();
    x_new = rng->get_double(-0.9 * gs->silo.R, 0.9 * gs->silo.R);
    y_new = rng->get_double(0.75 * gs->silo.H, 0.95 * gs->silo.H);
    b2Vec2 new_pos(x_new, y_new);
    b->SetTransform(new_pos, angle);
    infGr->isIn = true;
  }
  return;
}

void save_pf(b2World *w, GlobalSetup *gs, double t, std::ofstream &fout) {
  b2Vec2 pos, pv;
  BodyData *infGr;
  double y_inf = gs->silo.R;
  double y_sup = 2 * gs->silo.R;
  double radio;
  double pf_out = 0.0;
  double pf_bulk = 0.0;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) {
      continue;
    }
    pos = b->GetPosition();
    b2Fixture *fixt = b->GetFixtureList();
    b2Shape *shape = fixt->GetShape();
    infGr = (BodyData *)(b->GetUserData()).pointer;
    if (infGr->nLados == 1) {
      radio = shape->m_radius;
    } else {
      b2PolygonShape *poly = (b2PolygonShape *)shape;
      b2Vec2 *verts = (b2Vec2 *)poly->m_vertices;
      pv = b2Vec2(verts[0].x - pos.x, verts[0].y - pos.y);
      radio = pv.Length();
    }
    if (pos.y - radio > y_sup)
      continue;                  // Arriba de y_sup
    if (pos.y + radio > y_inf) { // Entre y_sup + r y y_inf - r
      pf_bulk += get_clipped_area(y_inf, y_sup, pos.y, radio);
    }
    if (abs(pos.x) > gs->silo.r)
      continue; // Centro afuera del radio de salida
    if (abs(pos.y) > radio)
      continue; // Centro lejos de y = 0
    pf_out += 2.0 * sqrt(radio * radio - pos.y * pos.y);
  }
  pf_out /= 2.0 * gs->silo.r;
  pf_bulk /= 2.0 * gs->silo.R * (y_sup - y_inf);
  fout << t << " " << pf_bulk << " " << pf_out << endl;
  return;
}

double get_clipped_area(double y_inf, double y_sup, double y, double r) {
  double h, a;
  if (abs(y - y_sup) < r) {
    h = r - abs(y - y_sup);
  } else if (abs(y - y_inf) < r) {
    h = r - abs(y - y_inf);
  } else {
    a = PI * r * r;
    return a;
  }
  a = r * r * acos(1.0 - h / r) - (r - h) * sqrt(r * r - (r - h) * (r - h));
  if (y > y_sup || y < y_inf) {
    return a;
  }
  return PI * r * r - a;
}

void update_pf_vx(b2World *w, double *vel_0, size_t *pf_0, size_t *bin_count,
                  int n_bins, double r_out) {
  b2Vec2 pos, vel, pv;
  BodyData *infGr;
  double x_inf, x_sup, tmp;
  int i_inf, i_sup;
  double radio;
  double delta_r = 2.0 * r_out / n_bins;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) {
      continue;
    }
    pos = b->GetPosition();
    if (abs(pos.x) > r_out)
      continue;
    b2Fixture *fixt = b->GetFixtureList();
    b2Shape *shape = fixt->GetShape();
    infGr = (BodyData *)(b->GetUserData()).pointer;
    if (infGr->nLados == 1) {
      radio = shape->m_radius;
    } else {
      b2PolygonShape *poly = (b2PolygonShape *)shape;
      b2Vec2 *verts = (b2Vec2 *)poly->m_vertices;
      pv = b2Vec2(verts[0].x - pos.x, verts[0].y - pos.y);
      radio = pv.Length();
    }
    if (abs(pos.y) > radio)
      continue;
    tmp = sqrt(radio * radio - pos.y * pos.y);
    x_inf = pos.x - tmp;
    x_sup = pos.x + tmp;
    i_inf = floor((x_inf + r_out) / delta_r);
    i_sup = floor((x_sup + r_out) / delta_r);
    if ((i_inf < 0) || (i_sup >= n_bins)) {
      cout << "ERROR: Fuera de límites en perfiles! " << endl;
      cout << "i_inf: " << i_inf << " " << x_inf << endl;
      cout << "i_sup: " << i_sup << " " << x_sup << endl;
      cout << pos.x << " " << pos.y << endl;
      exit(1);
    }
    // if (i_sup > 20) {
    // cout << "i_sup: " << i_sup << " " << x_sup << endl;
    // cout << pos.x << " " << pos.y << endl;
    //}
    vel = b->GetLinearVelocity();
    for (int i = i_inf; i <= i_sup; ++i) {
      vel_0[i] += vel.y; // \TODO Verficar si debo hacer la suma
      pf_0[i] += 1;
      bin_count[i] += 1;
      //    vel_0[i] += vel.y;
      //    pf_0[i] += 1;
    }
  }
  return;
}

void save_tensors(b2World *w, int n_frame, const GlobalSetup *globalSetup,
                  double *pmin, double *pmax) {
  string file_name = "frames_" + globalSetup->dirID + "/" +
                     globalSetup->preFrameFile + "_" + int2str(n_frame) +
                     ".sxy";
  std::ofstream fout;
  fout.open(file_name.c_str());
  fout << "# gID stres.xx stres.xy stres.yx stres.yy " << endl;
  b2Body *body_A, *body_B;
  BodyData *bd_data_A, *bd_data_B;
  unsigned int n_grains = 0;
  b2Vec2 l_A, l_B, force_N, force_T, force, c_point;
  float normal_impulse, tangential_impulse, area_a, area_b;
  for (b2Body *body = w->GetBodyList(); body; body = body->GetNext()) {
    if (body->GetType() != b2_dynamicBody) {
      continue;
    }
    bd_data_A = (BodyData *)(body->GetUserData()).pointer;
    if (bd_data_A->isGrain)
      n_grains++;
  }
  std::vector<Tensor> stress_tensors;
  stress_tensors.resize(n_grains, {0, 0, 0, 0});
  double pressure;
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (c->IsTouching()) {
      b2WorldManifold world_manifold;
      c->GetWorldManifold(&world_manifold);
      body_A = c->GetFixtureA()->GetBody();
      body_B = c->GetFixtureB()->GetBody();
      bd_data_A = (BodyData *)(body_A->GetUserData()).pointer;
      bd_data_B = (BodyData *)(body_B->GetUserData()).pointer;
      area_a = get_body_area(body_A);
      area_b = get_body_area(body_B);

      for (int32 i = 0; i < c->GetManifold()->pointCount; ++i) {
        c_point = world_manifold.points[i];
        normal_impulse = c->GetManifold()->points[i].normalImpulse;
        tangential_impulse = c->GetManifold()->points[i].tangentImpulse;
        // Normal apunta del body_A al body_B
        force_N = -normal_impulse * world_manifold.normal;
        force_T = -tangential_impulse *
                  b2Vec2(-world_manifold.normal.y, world_manifold.normal.x);
        force = (1.0 / globalSetup->tStep) *
                (force_N + force_T); // impulso -> fuerza
        l_A = c_point - body_A->GetWorldCenter();
        l_B = c_point - body_B->GetWorldCenter();
        if (bd_data_A->isGrain) {
          stress_tensors[bd_data_A->gID].xx += force.x * l_A.x / area_a;
          stress_tensors[bd_data_A->gID].xy += force.x * l_A.y / area_a;
          stress_tensors[bd_data_A->gID].yx += force.y * l_A.x / area_a;
          stress_tensors[bd_data_A->gID].yy += force.y * l_A.y / area_a;
        }
        if (bd_data_B->isGrain) {
          stress_tensors[bd_data_B->gID].xx -= force.x * l_B.x / area_b;
          stress_tensors[bd_data_B->gID].xy -= force.x * l_B.y / area_b;
          stress_tensors[bd_data_B->gID].yx -= force.y * l_B.x / area_b;
          stress_tensors[bd_data_B->gID].yy -= force.y * l_B.y / area_b;
        }
      }
    }
  }
  for (uint32 i = 0; i < n_grains; ++i) {
    fout << i << " " << stress_tensors[i].xx << " " << stress_tensors[i].xy
         << " " << stress_tensors[i].yx << " " << stress_tensors[i].yy << " "
         << endl;
    pressure = -0.5 * (stress_tensors[i].xx + stress_tensors[i].yy);
    if (pressure < *pmin)
      *pmin = pressure;
    if (pressure > *pmax)
      *pmax = pressure;
  }
  fout << std::flush;
  fout.close();
}

float get_body_area(b2Body *body) {
  float totalArea = 0.0f;
  b2Fixture *fixt = body->GetFixtureList();
  b2Shape *shape = fixt->GetShape();
  BodyData *infGr = (BodyData *)(body->GetUserData()).pointer;
  if (infGr->nLados == 1) {
    float radio = shape->m_radius;
    totalArea = M_PI * radio * radio;
  } else {
    b2PolygonShape *poly = (b2PolygonShape *)shape;
    int count = poly->m_count;
    b2Vec2 *verts = (b2Vec2 *)poly->m_vertices;
    float area = 0.0f;
    for (int i = 0; i < count - 1; ++i) {
      area += verts[i].x * verts[i + 1].y - verts[i + 1].x * verts[i].y;
    }
    totalArea += std::fabs(area) * 0.5f;
  }

  return totalArea;
}
