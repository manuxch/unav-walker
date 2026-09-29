/*! \file output.cpp
 * \brief Archivos de salida por frame y cabecera de procedencia.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 */

#include "output.hpp"

#include "body_data.hpp"
#include "contacts.hpp"

#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <map>
#include <numbers>
#include <sstream>

#ifndef GIT_HASH
#define GIT_HASH "desconocido"
#endif

using std::endl;
using std::string;

namespace {

/*! true si el punto está en el ROI (o si no se usa ROI). */
bool in_roi(b2Vec2 p, const GlobalSetup &gs) {
  if (!gs.save_roi_only) return true;
  if (p.x < -gs.x_roi || p.x > gs.x_roi) return false;
  if (p.y < gs.y_min_roi || p.y > gs.y_max_roi) return false;
  return true;
}

/*! Ruta de un archivo de frame: frames_<dirID>/<prefijo><pre>_<frame><ext>. */
string frame_path(const GlobalSetup &gs, const string &prefix, int n_frame,
                  const string &ext) {
  return "frames_" + gs.dir_id + "/" + prefix + gs.pre_frame_file + "_" +
         frame_id_string(n_frame) + ext;
}

} // namespace

string frame_id_string(int num) {
  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(6) << num;
  return oss.str();
}

string local_time_string() {
  auto now = std::chrono::system_clock::now();
  auto now_time = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
  return ss.str();
}

string provenance_header(const GlobalSetup &gs, double t, uint32_t n_step,
                         int n_frame) {
  constexpr double pi = std::numbers::pi;
  const double omega = 2.0 * pi * gs.silo.frec;
  double phase = std::fmod(omega * t, 2.0 * pi);
  std::ostringstream oss;
  oss << std::setprecision(10);
  oss << "# nStep: " << n_step << " n_frame: " << n_frame << " t: " << t
      << " dt: " << gs.dt << " fase: " << phase << " (w t mod 2pi, rad)\n";
  oss << "# git: " << GIT_HASH << " params: " << gs.params_file
      << " params_hash: " << gs.params_hash << "\n";
  return oss.str();
}

void save_frame(b2World *w, int n_frame, int n_step, const GlobalSetup &gs) {
  std::ofstream fout(frame_path(gs, "", n_frame, ".xy"));
  fout << "# time: " << n_step * gs.dt << " ";
  fout << "# r_out: " << gs.silo.r << " ";
  fout << endl;
  fout << provenance_header(gs, n_step * gs.dt, static_cast<uint32_t>(n_step),
                            n_frame);
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    const BodyData *bd = body_data(b);
    if (bd->is_grain) {
      if (!in_roi(b->GetPosition(), gs)) continue;
      // Disco: "gid 1 x y radio tipo"
      fout << bd->gid << " ";
      fout << "1 ";
      b2Vec2 pos = b->GetPosition();
      float radio = b->GetFixtureList()->GetShape()->m_radius;
      fout << pos.x << " " << pos.y << " " << radio << " ";
      fout << bd->tipo << " ";
      fout << endl;
    } else if (bd->gid == kGidLid || bd->gid == kGidMeasuringFloor) {
      // Tapa o fondo de medición (b2EdgeShape): "gid 2 x1 y1 x2 y2 etiqueta"
      const b2EdgeShape *s =
          static_cast<const b2EdgeShape *>(b->GetFixtureList()->GetShape());
      b2Vec2 v1 = b->GetWorldPoint(s->m_vertex1);
      b2Vec2 v2 = b->GetWorldPoint(s->m_vertex2);
      fout << bd->gid << " ";
      fout << 2 << " ";
      fout << v1.x << " " << v1.y << " ";
      fout << v2.x << " " << v2.y << " ";
      fout << (bd->gid == kGidLid ? "LID-F" : "FLOOR") << endl;
    } else {
      // Paredes del silo (cadenas): un segmento por línea
      for (b2Fixture *f = b->GetFixtureList(); f; f = f->GetNext()) {
        const b2ChainShape *s =
            static_cast<const b2ChainShape *>(f->GetShape());
        const b2Vec2 *verts = s->m_vertices;
        for (int i = 0; i < s->m_count - 1; ++i) {
          fout << bd->gid << " 2 ";
          fout << verts[i].x << " " << verts[i].y << " ";
          fout << verts[i + 1].x << " " << verts[i + 1].y;
          fout << " LINE" << endl;
        }
      }
    }
  }
}

void save_velocities(int n_frame, double t, uint32_t n_step, b2World *w,
                     const GlobalSetup &gs) {
  std::ofstream fout(frame_path(gs, "", n_frame, ".ve"));
  fout << " ## sim_time: " << t << endl;
  fout << provenance_header(gs, t, n_step, n_frame);
  fout << "# gID type x y vx vy w E_kin_lin E_kin_rot" << endl;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    const BodyData *bd = body_data(b);
    if (!bd->is_grain) continue;
    b2Vec2 pos = b->GetPosition();
    if (!in_roi(pos, gs)) continue;
    b2Vec2 vel = b->GetLinearVelocity();
    float v_mod = vel.Length();
    float omega = b->GetAngularVelocity();
    float mass = b->GetMass();
    float inertia = b->GetInertia();
    fout << bd->gid << " " << bd->tipo << " " << pos.x << " " << pos.y << " "
         << vel.x << " " << vel.y << " " << omega << " "
         << 0.5 * mass * v_mod * v_mod << " " << 0.5 * inertia * omega * omega
         << endl;
  }
}

void save_contacts(b2World *w, double t, uint32_t n_step, int n_frame,
                   const GlobalSetup &gs) {
  std::ofstream fout(frame_path(gs, "fc_", n_frame, ".dat"));
  fout << "# Time: " << std::setprecision(10) << t << endl;
  fout << provenance_header(gs, t, n_step, n_frame);
  fout << "# Fn, Ft: componentes normal y tangencial (impulso / dt) de la "
          "fuerza sobre B.\n"
          "# Fuerza sobre B = Fn (nx, ny) + Ft (ny, -nx); sobre A, la "
          "opuesta. La normal apunta de A a B.\n"
          "# (xA, yA), (xB, yB): centros de masa de A y B; para una pared se "
          "escribe el punto de contacto.\n"
          "# n_pc: puntos del manifold. tipo: GG grano-grano, GW "
          "grano-pared.\n";
  fout << "# gID_A gID_B cp.x cp.y Fn Ft nx ny xA yA xB yB n_pc tipo" << endl;
  fout << std::scientific << std::uppercase << std::setprecision(6);
  const double inv_dt = 1.0 / gs.dt;
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching()) continue;
    b2Body *body_a = c->GetFixtureA()->GetBody();
    b2Body *body_b = c->GetFixtureB()->GetBody();
    const BodyData *bd_a = body_data(body_a);
    const BodyData *bd_b = body_data(body_b);
    // Se guardan todos los contactos de los granos cuyo centro está en el ROI
    bool sel_a = bd_a->is_grain && in_roi(body_a->GetPosition(), gs);
    bool sel_b = bd_b->is_grain && in_roi(body_b->GetPosition(), gs);
    if (!sel_a && !sel_b) continue;
    const char *tipo = (bd_a->is_grain && bd_b->is_grain) ? "GG" : "GW";
    int n_points = c->GetManifold()->pointCount;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    for (int i = 0; i < n_points; i++) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      b2Vec2 c_a = bd_a->is_grain ? body_a->GetWorldCenter() : cpf.point;
      b2Vec2 c_b = bd_b->is_grain ? body_b->GetWorldCenter() : cpf.point;
      fout << bd_a->gid << " " << bd_b->gid << " " << cpf.point.x << " "
           << cpf.point.y << " " << cpf.fn << " " << cpf.ft << " "
           << cpf.normal.x << " " << cpf.normal.y << " " << c_a.x << " "
           << c_a.y << " " << c_b.x << " " << c_b.y << " " << n_points << " "
           << tipo << "\n";
    }
  }
}

void save_stress(b2World *w, int n_frame, const GlobalSetup &gs, double *p_min,
                 double *p_max, double t, uint32_t n_step) {
  std::ofstream fout(frame_path(gs, "", n_frame, ".sxy"));
  fout << "# tSim: " << std::setprecision(10) << t << endl;
  fout << provenance_header(gs, t, n_step, n_frame);
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
  // Ordenado por gid, para que la salida no dependa del orden de Box2D
  std::map<int, GrainStress> stress;
  for (b2Body *b = w->GetBodyList(); b; b = b->GetNext()) {
    if (b->GetType() != b2_dynamicBody) continue;
    const BodyData *bd = body_data(b);
    if (!bd->is_grain || !in_roi(b->GetPosition(), gs)) continue;
    GrainStress gsr;
    gsr.body = b;
    gsr.area = grain_area(b);
    stress[bd->gid] = gsr;
  }

  const double inv_dt = 1.0 / gs.dt;
  // Suma la contribución de un punto de contacto al grano (si está en el ROI)
  auto add = [&](b2Body *body, const BodyData *bd, const ContactPointForce &cpf,
                 double sgn, bool with_wall) {
    if (!bd->is_grain) return;
    auto it = stress.find(bd->gid);
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
      if (with_wall)
        ++g.z_gw;
      else
        ++g.z_gg;
    }
  };
  for (b2Contact *c = w->GetContactList(); c; c = c->GetNext()) {
    if (!c->IsTouching()) continue;
    b2WorldManifold wm;
    c->GetWorldManifold(&wm);
    b2Body *body_a = c->GetFixtureA()->GetBody();
    b2Body *body_b = c->GetFixtureB()->GetBody();
    const BodyData *bd_a = body_data(body_a);
    const BodyData *bd_b = body_data(body_b);
    bool with_wall = !(bd_a->is_grain && bd_b->is_grain);
    for (int32 i = 0; i < c->GetManifold()->pointCount; ++i) {
      ContactPointForce cpf = contact_point_force(c, wm, i, inv_dt);
      add(body_a, bd_a, cpf, -1.0, with_wall); // sobre A: -F
      add(body_b, bd_b, cpf, 1.0, with_wall);  // sobre B: +F
    }
  }

  fout << std::scientific << std::setprecision(6);
  for (const auto &[gid, g] : stress) {
    b2Vec2 p = g.body->GetWorldCenter();
    b2Vec2 v = g.body->GetLinearVelocity();
    float r = g.body->GetFixtureList()->GetShape()->m_radius;
    fout << gid;
    for (double x : g.s)
      fout << " " << x;
    for (double x : g.sn)
      fout << " " << x;
    fout << " " << p.x << " " << p.y << " " << r << " " << g.body->GetMass()
         << " " << v.x << " " << v.y << " " << g.body->GetAngularVelocity()
         << " " << g.z_gg << " " << g.z_gw << "\n";
    double pressure = -0.5 * (g.s[0] + g.s[3]);
    if (pressure < *p_min) *p_min = pressure;
    if (pressure > *p_max) *p_max = pressure;
  }
}
