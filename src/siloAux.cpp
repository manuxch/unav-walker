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
#include <algorithm>
#include <map>

#ifndef GIT_HASH
#define GIT_HASH "desconocido"
#endif

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

static bool inROI(b2Vec2 p, const GlobalSetup* gs) {
  if (!gs->save_roi_only) return true;
  if (p.x < -gs->x_roi || p.x > gs->x_roi) return false;
  if (p.y < gs->y_min_roi || p.y > gs->y_max_roi) return false;
  return true;
}

ContactPointForce contact_point_force(b2Contact *c, const b2WorldManifold &wm,
                                      int i, double inv_dt) {
  const b2ManifoldPoint &mp = c->GetManifold()->points[i];
  ContactPointForce f;
  f.point = wm.points[i];
  f.normal = wm.normal;
  f.tangent = b2Cross(wm.normal, 1.0f); // (n.y, -n.x), como b2ContactSolver
  f.fn = mp.normalImpulse * inv_dt;
  f.ft = mp.tangentImpulse * inv_dt;
  f.fx = f.fn * f.normal.x + f.ft * f.tangent.x;
  f.fy = f.fn * f.normal.y + f.ft * f.tangent.y;
  return f;
}

void body_contact_wrench(b2Body *b, double inv_dt, b2Vec2 *F, double *tau) {
  double fx = 0.0, fy = 0.0, tz = 0.0;
  const b2Vec2 center = b->GetWorldCenter();
  for (b2ContactEdge *ce = b->GetContactList(); ce; ce = ce->next) {
    b2Contact *c = ce->contact;
    if (!c->IsTouching()) continue;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    double sgn = (c->GetFixtureB()->GetBody() == b) ? 1.0 : -1.0;
    for (int i = 0; i < c->GetManifold()->pointCount; ++i) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      b2Vec2 l = cpf.point - center;
      fx += sgn * cpf.fx;
      fy += sgn * cpf.fy;
      tz += sgn * (l.x * cpf.fy - l.y * cpf.fx);
    }
  }
  *F = b2Vec2(static_cast<float>(fx), static_cast<float>(fy));
  *tau = tz;
}

std::string provenance_header(const GlobalSetup *gs, double t, uint32_t nStep,
                              int n_frame) {
  const double omega = 2.0 * PI * gs->silo.frec;
  double phase = std::fmod(omega * t, 2.0 * PI);
  std::ostringstream oss;
  oss << std::setprecision(10);
  oss << "# nStep: " << nStep << " n_frame: " << n_frame << " t: " << t
      << " dt: " << gs->tStep << " fase: " << phase
      << " (w t mod 2pi, rad)\n";
  oss << "# git: " << GIT_HASH << " params: " << gs->input_par_file
      << " params_hash: " << gs->params_hash << "\n";
  return oss.str();
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

void saveFrame(b2World *w, int n_frame, int nStep,
               const GlobalSetup *globalSetup) {
  float xtmp, ytmp;
  string file_name = "frames_" + globalSetup->dirID + "/" +
                     globalSetup->preFrameFile + "_" + int2str(n_frame) + ".xy";
  std::ofstream fileF;
  fileF.open(file_name.c_str());
  fileF << "# time: " << nStep * globalSetup->tStep << " ";
  fileF << "# r_out: " << globalSetup->silo.r << " ";
  fileF << endl;
  fileF << provenance_header(globalSetup, nStep * globalSetup->tStep,
                             static_cast<uint32_t>(nStep), n_frame);
  for (b2Body *bd = w->GetBodyList(); bd; bd = bd->GetNext()) {
    BodyData *infGr = (BodyData *)(bd->GetUserData()).pointer;
    // if (infGr->gID == -110 || infGr->gID == -120)
    // continue;  // no guardo las coordenadas de la tapa.
    if (infGr->isGrain) {
      if (!inROI(bd->GetPosition(), globalSetup)) continue;
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
    } else if (infGr->gID == -110 || infGr->gID == -200) { // cuerpos con EdgeShape
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
      fileF << (infGr->gID == -110 ? "LID-F" : "FLOOR") << endl;
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

void printVE(const int frm_id, const double timeS, uint32_t nStep, b2World *w,
             const GlobalSetup *gs) {
  b2Vec2 pi, vi;
  float wi, mi, Ii, vim;
  b2Vec2 vt(0.0, 0.0);
  string file_name = "frames_" + gs->dirID + "/" + gs->preFrameFile + "_" +
                     int2str(frm_id) + ".ve";
  std::ofstream fileF;
  fileF.open(file_name.c_str());
  fileF << " ## sim_time: " << timeS << endl;
  fileF << provenance_header(gs, timeS, nStep, frm_id);
  fileF << "# gID type x y vx vy w E_kin_lin E_kin_rot" << endl;
  for (b2Body *bi = w->GetBodyList(); bi; bi = bi->GetNext()) {
    BodyData *igi = (BodyData *)(bi->GetUserData()).pointer;
    if (!igi->isGrain) {
      continue;
    }
    pi = bi->GetPosition();
    if (!inROI(pi, gs)) continue;
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

void saveContacts(b2World *w, double t, uint32_t nStep, int n_frame,
                  const GlobalSetup *globalSetup) {
  string file_name = "frames_" + globalSetup->dirID + "/fc_" +
                     globalSetup->preFrameFile + "_" + int2str(n_frame) +
                     ".dat";
  std::ofstream ff;
  ff.open(file_name.c_str());
  ff << "# Time: " << std::setprecision(10) << t << endl;
  ff << provenance_header(globalSetup, t, nStep, n_frame);
  ff << "# Fn, Ft: componentes normal y tangencial (impulso / dt) de la fuerza "
        "sobre B.\n"
        "# Fuerza sobre B = Fn (nx, ny) + Ft (ny, -nx); sobre A, la opuesta. "
        "La normal apunta de A a B.\n"
        "# (xA, yA), (xB, yB): centros de masa de A y B; para una pared se "
        "escribe el punto de contacto.\n"
        "# n_pc: puntos del manifold. tipo: GG grano-grano, GW grano-pared.\n";
  ff << "# gID_A gID_B cp.x cp.y Fn Ft nx ny xA yA xB yB n_pc tipo" << endl;
  ff << std::scientific << std::uppercase << std::setprecision(6);
  const double inv_dt = 1.0 / globalSetup->tStep;
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching())
      continue;
    b2Body *bodyA = c->GetFixtureA()->GetBody();
    b2Body *bodyB = c->GetFixtureB()->GetBody();
    BodyData *bdgdA = (BodyData *)(bodyA->GetUserData()).pointer;
    BodyData *bdgdB = (BodyData *)(bodyB->GetUserData()).pointer;
    // Se guardan todos los contactos de los granos cuyo centro está en el ROI
    bool selA = bdgdA->isGrain && inROI(bodyA->GetPosition(), globalSetup);
    bool selB = bdgdB->isGrain && inROI(bodyB->GetPosition(), globalSetup);
    if (!selA && !selB) continue;
    const char *tipo = (bdgdA->isGrain && bdgdB->isGrain) ? "GG" : "GW";
    int numPoints = c->GetManifold()->pointCount;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    for (int i = 0; i < numPoints; i++) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      b2Vec2 cA = bdgdA->isGrain ? bodyA->GetWorldCenter() : cpf.point;
      b2Vec2 cB = bdgdB->isGrain ? bodyB->GetWorldCenter() : cpf.point;
      ff << bdgdA->gID << " " << bdgdB->gID << " " << cpf.point.x << " "
         << cpf.point.y << " " << cpf.fn << " " << cpf.ft << " "
         << cpf.normal.x << " " << cpf.normal.y << " " << cA.x << " " << cA.y
         << " " << cB.x << " " << cB.y << " " << numPoints << " " << tipo
         << "\n";
    }
  }
  ff << std::flush;
  ff.close();
}

b2Vec2 compute_wall_force(b2World *w, const GlobalSetup *gs, int wall_gID) {
  double fx = 0.0, fy = 0.0;
  const double inv_dt = 1.0 / gs->tStep;
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching()) continue;
    b2Body *bodyA = c->GetFixtureA()->GetBody();
    b2Body *bodyB = c->GetFixtureB()->GetBody();
    BodyData *bdA = (BodyData *)(bodyA->GetUserData()).pointer;
    BodyData *bdB = (BodyData *)(bodyB->GetUserData()).pointer;
    bool A_is_wall = (bdA->gID == wall_gID);
    bool B_is_wall = (bdB->gID == wall_gID);
    if (!A_is_wall && !B_is_wall) continue;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    // contact_point_force da la fuerza sobre B; sobre A es la opuesta.
    double sgn = A_is_wall ? -1.0 : 1.0;
    for (int i = 0; i < c->GetManifold()->pointCount; ++i) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      fx += sgn * cpf.fx;
      fy += sgn * cpf.fy;
    }
  }
  return b2Vec2(static_cast<float>(fx), static_cast<float>(fy));
}

b2Vec2 karnopp(b2Vec2 v_rel, b2Vec2 F_ext, b2Vec2 a_base, double m, double dt,
               double v_tol, double mu_s, double mu_d, double N) {
  const double v_norm = v_rel.Length();
  const double v_stick = std::max(v_tol, mu_d * N * dt / m);
  if (v_norm < v_stick) {
    // Adherencia: fuerza para que v_rel = 0 al final del paso
    double fx = m * a_base.x - F_ext.x - m * v_rel.x / dt;
    double fy = m * a_base.y - F_ext.y - m * v_rel.y / dt;
    double f_norm = std::sqrt(fx * fx + fy * fy);
    double f_max = mu_s * N;
    if (f_norm > f_max) { // Ruptura: se satura en mu_s N
      fx *= f_max / f_norm;
      fy *= f_max / f_norm;
    }
    return b2Vec2(static_cast<float>(fx), static_cast<float>(fy));
  }
  // Deslizamiento
  double k = -mu_d * N / v_norm;
  return b2Vec2(static_cast<float>(k * v_rel.x), static_cast<float>(k * v_rel.y));
}

double pivot_friction(double w, double tau_ext, double I, double R, double dt,
                      double v_tol, double mu_s, double mu_d, double N) {
  const double arm = 2.0 / 3.0 * R; // brazo efectivo con presión uniforme
  const double tau_d = mu_d * N * arm;
  const double w_stick = std::max(v_tol / R, tau_d * dt / I);
  if (std::fabs(w) < w_stick) {
    // Adherencia: torque para que w = 0 al final del paso
    double tau = -tau_ext - I * w / dt;
    double tau_max = mu_s * N * arm;
    if (std::fabs(tau) > tau_max) tau = std::copysign(tau_max, tau);
    return tau;
  }
  return -std::copysign(tau_d, w); // Deslizamiento
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

void do_base_force(b2World *w, double bvel, double bacc, double epsilon_v,
                   double g, double dt) {
  BodyData *bdata;
  // La base se mueve en -y con la velocidad y aceleración de exitacion_mm.
  const b2Vec2 base_vel_vec(0.0f, static_cast<float>(-bvel));
  const b2Vec2 base_acc_vec(0.0f, static_cast<float>(-bacc));
  const double inv_dt = 1.0 / dt;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) {
      continue;
    }
    double m = b->GetMass();
    double N = g * m;
    bdata = reinterpret_cast<BodyData *>(b->GetUserData().pointer);
    b2Vec2 vrel = b->GetLinearVelocity() - base_vel_vec;
    // Resto de las fuerzas y torques sobre el grano: contactos del último Step
    b2Vec2 F_ext;
    double tau_ext;
    body_contact_wrench(b, inv_dt, &F_ext, &tau_ext);
    b2Vec2 F_roce = karnopp(vrel, F_ext, base_acc_vec, m, dt, epsilon_v,
                            bdata->fric_s, bdata->fric_d, N);
    double R = b->GetFixtureList()->GetShape()->m_radius;
    double tau_roce = pivot_friction(b->GetAngularVelocity(), tau_ext,
                                     b->GetInertia(), R, dt, epsilon_v,
                                     bdata->fric_s, bdata->fric_d, N);
    bdata->F_base = F_roce;
    bdata->tau_base = static_cast<float>(tau_roce);
    b->ApplyForceToCenter(F_roce, true);
    b->ApplyTorque(static_cast<float>(tau_roce), true);
  }
  return;
}

// void do_reinyection(b2World *w, GlobalSetup *gs) {
//   b2Vec2 pos;
//   BodyData *infGr;
//   // double r_elim = -gs->silo.R;
//   double r_elim = -10.0;
//   double x_new, y_new, angle;
//   for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
//     if (b->GetType() != b2_dynamicBody) {
//       continue;
//     }
//     infGr = (BodyData *)(b->GetUserData()).pointer;
//     if (infGr->isIn)
//       continue;
//     pos = b->GetPosition();
//     if (pos.y > r_elim)
//       continue;
//     angle = b->GetAngle();
//     x_new = rng->get_double(-0.9 * gs->silo.R, 0.9 * gs->silo.R);
//     y_new = rng->get_double(0.75 * gs->silo.H, 0.95 * gs->silo.H);
//     b2Vec2 new_pos(x_new, y_new);
//     b->SetTransform(new_pos, angle);
//     infGr->isIn = true;
//   }
//   return;
// }

namespace {
// Detecta si un disco de prueba se superpone con algún fixture del mundo.
class OverlapQuery : public b2QueryCallback {
public:
  b2CircleShape probe;
  b2Transform xf;
  const b2Body *self = nullptr;
  bool hit = false;
  bool ReportFixture(b2Fixture *f) override {
    if (f->GetBody() == self) return true;
    const b2Shape *s = f->GetShape();
    for (int32 ch = 0; ch < s->GetChildCount(); ++ch) {
      if (b2TestOverlap(&probe, 0, s, ch, xf, f->GetBody()->GetTransform())) {
        hit = true;
        return false;
      }
    }
    return true;
  }
};

bool find_free_spot(b2World *w, const b2Body *self, float radius,
                    const GlobalSetup *gs, b2Vec2 *pos) {
  const int max_tries = 100;
  const float gap = 1.02f; // pequeña separación para evitar contactos iniciales
  for (int k = 0; k < max_tries; ++k) {
    b2Vec2 p(static_cast<float>(rng->get_double(-0.9 * gs->silo.R, 0.9 * gs->silo.R)),
             static_cast<float>(rng->get_double(0.75 * gs->silo.H, 0.95 * gs->silo.H)));
    OverlapQuery q;
    q.probe.m_radius = gap * radius;
    q.xf.Set(p, 0.0f);
    q.self = self;
    b2AABB aabb;
    aabb.lowerBound = p - b2Vec2(gap * radius, gap * radius);
    aabb.upperBound = p + b2Vec2(gap * radius, gap * radius);
    w->QueryAABB(&q, aabb);
    if (!q.hit) {
      *pos = p;
      return true;
    }
  }
  return false;
}
} // namespace

void do_reinyection(b2World *w, GlobalSetup *gs, bool reinyect) {
  b2Vec2 pos;
  BodyData *infGr;
  double r_elim = -10.0;
  static unsigned long n_fail = 0;

  b2Body *b = w->GetBodyList();
  while (b) {
    // Guardar el siguiente cuerpo ANTES de cualquier posible eliminación
    b2Body *nextBody = b->GetNext();

    if (b->GetType() != b2_dynamicBody) {
      b = nextBody;
      continue;
    }

    infGr = (BodyData *)(b->GetUserData()).pointer;
    if (infGr->isIn) {
      b = nextBody;
      continue;
    }

    pos = b->GetPosition();
    if (std::isnan(pos.x) || std::isnan(pos.y)) {
      cout << "ERROR: Grano " << infGr->gID << " tiene posición inválida (NaN)" << endl;
      w->DestroyBody(b);
      b = nextBody;
      continue;
    }

    if (pos.y > r_elim) {
      b = nextBody;
      continue;
    }

    if (reinyect) {
      b2Vec2 new_pos;
      float radius = b->GetFixtureList()->GetShape()->m_radius;
      if (find_free_spot(w, b, radius, gs, &new_pos)) {
        b->SetTransform(new_pos, b->GetAngle());
        b->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
        b->SetAngularVelocity(0.0f);
        b->SetAwake(true);
        infGr->isIn = true;
      } else if (++n_fail % 1000 == 1) {
        cout << "# AVISO: sin lugar libre para reinyectar (fallos acumulados: "
             << n_fail << "); se reintenta en el paso siguiente." << endl;
      }
    }
    else {
        w->DestroyBody(b);
    }
    b = nextBody;
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
    // Un grano superpuesto con el borde del orificio puede exceder el rango
    i_inf = std::max(i_inf, 0);
    i_sup = std::min(i_sup, n_bins - 1);
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
                  double *pmin, double *pmax, double tSim, uint32_t nStep) {
  string file_name = "frames_" + globalSetup->dirID + "/" +
                     globalSetup->preFrameFile + "_" + int2str(n_frame) +
                     ".sxy";
  std::ofstream fout;
  fout.open(file_name.c_str());
  fout << "# tSim: " << std::setprecision(10) << tSim << endl;
  fout << provenance_header(globalSetup, tSim, nStep, n_frame);
  fout << "# s_ij = (1/A_grano) sum_c f_i l_j, f: fuerza de contacto sobre el "
          "grano, l: punto de contacto - centro. Compresión < 0.\n"
          "# sn_ij: parte debida solo a las fuerzas normales (la tangencial es "
          "s - sn). Incluye contactos con paredes.\n"
          "# z_gg, z_gw: puntos de contacto activos (Fn > 0) grano-grano y "
          "grano-pared. m: masa, w: velocidad angular.\n";
  fout << "# gID sxx sxy syx syy snxx snxy snyx snyy x y r m vx vy w z_gg z_gw"
       << endl;

  struct GrainStress {
    b2Body *body = nullptr;
    double area = 1.0;
    double s[4] = {0.0, 0.0, 0.0, 0.0};  // xx, xy, yx, yy
    double sn[4] = {0.0, 0.0, 0.0, 0.0}; // parte normal
    int z_gg = 0, z_gw = 0;
  };
  std::map<int, GrainStress> stress;
  for (b2Body *body = w->GetBodyList(); body; body = body->GetNext()) {
    if (body->GetType() != b2_dynamicBody) continue;
    BodyData *bd = (BodyData *)(body->GetUserData()).pointer;
    if (!bd->isGrain || !inROI(body->GetPosition(), globalSetup)) continue;
    GrainStress gsr;
    gsr.body = body;
    gsr.area = get_body_area(body);
    stress[bd->gID] = gsr;
  }

  const double inv_dt = 1.0 / globalSetup->tStep;
  auto add = [&](b2Body *body, BodyData *bd, const ContactPointForce &cpf,
                 double sgn, bool with_wall) {
    if (!bd->isGrain) return;
    auto it = stress.find(bd->gID);
    if (it == stress.end()) return;
    GrainStress &g = it->second;
    b2Vec2 l = cpf.point - body->GetWorldCenter();
    double fx = sgn * cpf.fx, fy = sgn * cpf.fy;
    double fnx = sgn * cpf.fn * cpf.normal.x, fny = sgn * cpf.fn * cpf.normal.y;
    g.s[0] += fx * l.x / g.area;
    g.s[1] += fx * l.y / g.area;
    g.s[2] += fy * l.x / g.area;
    g.s[3] += fy * l.y / g.area;
    g.sn[0] += fnx * l.x / g.area;
    g.sn[1] += fnx * l.y / g.area;
    g.sn[2] += fny * l.x / g.area;
    g.sn[3] += fny * l.y / g.area;
    if (cpf.fn > 0.0) {
      if (with_wall) ++g.z_gw;
      else ++g.z_gg;
    }
  };
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching()) continue;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    b2Body *body_A = c->GetFixtureA()->GetBody();
    b2Body *body_B = c->GetFixtureB()->GetBody();
    BodyData *bd_A = (BodyData *)(body_A->GetUserData()).pointer;
    BodyData *bd_B = (BodyData *)(body_B->GetUserData()).pointer;
    bool with_wall = !(bd_A->isGrain && bd_B->isGrain);
    for (int32 i = 0; i < c->GetManifold()->pointCount; ++i) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      add(body_A, bd_A, cpf, -1.0, with_wall); // sobre A: -F
      add(body_B, bd_B, cpf, 1.0, with_wall);  // sobre B: +F
    }
  }

  fout << std::scientific << std::setprecision(6);
  for (const auto &[gID, g] : stress) {
    b2Vec2 p = g.body->GetWorldCenter();
    b2Vec2 v = g.body->GetLinearVelocity();
    float r = g.body->GetFixtureList()->GetShape()->m_radius;
    fout << gID;
    for (double x : g.s) fout << " " << x;
    for (double x : g.sn) fout << " " << x;
    fout << " " << p.x << " " << p.y << " " << r << " " << g.body->GetMass()
         << " " << v.x << " " << v.y << " " << g.body->GetAngularVelocity()
         << " " << g.z_gg << " " << g.z_gw << "\n";
    double pressure = -0.5 * (g.s[0] + g.s[3]);
    if (pressure < *pmin) *pmin = pressure;
    if (pressure > *pmax) *pmax = pressure;
  }
  fout << std::flush;
  fout.close();
}

void record_pre_step(b2World *w) {
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    BodyData *bd = (BodyData *)(b->GetUserData()).pointer;
    bd->v_prev = b->GetLinearVelocity();
    bd->w_prev = b->GetAngularVelocity();
  }
}

void check_force_balance(b2World *w, const GlobalSetup *gs, double t,
                         uint32_t nStep, std::ofstream &fout) {
  const double dt = gs->tStep;
  // Acumuladores: [0] tangente de Box2D, [1] tangente invertida (control)
  double res_lin[2] = {0, 0}, ref_lin[2] = {0, 0};
  double res_ang[2] = {0, 0}, ref_ang[2] = {0, 0};
  double max_rel_lin = 0.0, max_rel_ang = 0.0;
  int n_grains = 0, n_bad = 0;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    BodyData *bd = (BodyData *)(b->GetUserData()).pointer;
    if (!bd->isGrain) continue;
    const double m = b->GetMass();
    const double I = b->GetInertia();
    const b2Vec2 center = b->GetWorldCenter();
    const b2Vec2 dv = b->GetLinearVelocity() - bd->v_prev;
    const double dw = b->GetAngularVelocity() - bd->w_prev;
    for (int k = 0; k < 2; ++k) {
      const double tsgn = (k == 0) ? 1.0 : -1.0;
      double Jx = 0.0, Jy = 0.0, Lz = 0.0, sumJ = 0.0, sumL = 0.0;
      for (b2ContactEdge *ce = b->GetContactList(); ce; ce = ce->next) {
        b2Contact *c = ce->contact;
        if (!c->IsTouching()) continue;
        b2WorldManifold wm;
        c->GetWorldManifold(&wm);
        double sgn = (c->GetFixtureB()->GetBody() == b) ? 1.0 : -1.0;
        for (int i = 0; i < c->GetManifold()->pointCount; ++i) {
          ContactPointForce cpf = contact_point_force(c, wm, i, 1.0);
          double jx = sgn * (cpf.fn * cpf.normal.x + tsgn * cpf.ft * cpf.tangent.x);
          double jy = sgn * (cpf.fn * cpf.normal.y + tsgn * cpf.ft * cpf.tangent.y);
          b2Vec2 l = cpf.point - center;
          double lz = l.x * jy - l.y * jx;
          Jx += jx;
          Jy += jy;
          Lz += lz;
          sumJ += std::sqrt(jx * jx + jy * jy);
          sumL += std::fabs(lz);
        }
      }
      double rx = m * dv.x - dt * bd->F_base.x - Jx;
      double ry = m * dv.y - dt * bd->F_base.y - Jy;
      double rl = std::sqrt(rx * rx + ry * ry);
      double refl = m * dv.Length() + dt * bd->F_base.Length() + sumJ;
      double ra = std::fabs(I * dw - dt * bd->tau_base - Lz);
      double refa = std::fabs(I * dw) + dt * std::fabs(bd->tau_base) + sumL;
      res_lin[k] += rl;
      ref_lin[k] += refl;
      res_ang[k] += ra;
      ref_ang[k] += refa;
      if (k == 0) {
        // Piso de la referencia: impulso típico de la fricción con la base
        // (evita residuos relativos espurios en granos casi sin fuerzas).
        const double r = b->GetFixtureList()->GetShape()->m_radius;
        const double j0 = dt * bd->fric_d * gs->g * m;
        double rel_l = rl / std::max(refl, j0);
        double rel_a = ra / std::max(refa, r * j0);
        max_rel_lin = std::max(max_rel_lin, rel_l);
        max_rel_ang = std::max(max_rel_ang, rel_a);
        if (rel_l > 1e-2 || rel_a > 1e-2) ++n_bad;
      }
    }
    ++n_grains;
  }
  auto ratio = [](double a, double b) { return (b > 0) ? a / b : 0.0; };
  fout << std::setprecision(10) << t << " " << nStep << " " << n_grains
       << std::scientific << std::setprecision(4) << " "
       << ratio(res_lin[0], ref_lin[0]) << " " << max_rel_lin << " "
       << ratio(res_ang[0], ref_ang[0]) << " " << max_rel_ang << " " << n_bad
       << " " << ratio(res_lin[1], ref_lin[1]) << " "
       << ratio(res_ang[1], ref_ang[1]) << std::defaultfloat << "\n";
}

float get_body_area(b2Body *body) {
  float totalArea = 0.0f;
  b2Fixture *fixt = body->GetFixtureList();
  b2Shape *shape = fixt->GetShape();
  BodyData *infGr = (BodyData *)(body->GetUserData()).pointer;
  if (!infGr->isGrain) return 1.0f; // paredes estáticas: área no aplicable
  if (infGr->nLados == 1) {
    float radio = shape->m_radius;
    totalArea = M_PI * radio * radio;
  } else {
    b2PolygonShape *poly = (b2PolygonShape *)shape;
    int count = poly->m_count;
    b2Vec2 *verts = (b2Vec2 *)poly->m_vertices;
    float area = 0.0f;
    for (int i = 0; i < count; ++i) {
      int j = (i + 1) % count;
      area += verts[i].x * verts[j].y - verts[j].x * verts[i].y;
    }
    totalArea += std::fabs(area) * 0.5f;
  }
  return totalArea;
}

std::string get_local_time() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
