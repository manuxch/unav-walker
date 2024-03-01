#include "globalsetup.hpp"
#include "rng.hpp"
#include <box2d/box2d.h>
#include <iostream>
using std::cout; using std::endl;
#include <cmath>
#include <stdexcept>
#include <tuple>

GlobalSetup *gs;
RNG *rng;
struct Exit {
    double x;
    double v;
    double a;
};

b2Vec2 karnopp(b2Vec2 v, double v_tol, double mu_s, double mu_d, double p);
b2Vec2 smooth_coulomb(b2Vec2 v, double v_d, double mu_d, double p);
b2Vec2 smooth_coulomb_2(b2Vec2 v, double v_d, double v_s, double mu_d, double mu_s, double p);
Exit exitacion(double t, double gamma, double w);
Exit exitacion_2f(double t, double gamma, double w);
Exit exitacion_mm(double t, double gamma, double w);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Error: archivo de parámetros requerido." << std::endl;
        exit(1);
    }
    gs = new GlobalSetup{argv[1]};
    rng = new RNG(gs->rnd_seed);
    b2Vec2 gravedad;
    gravedad.Set(0.0f, 0.0f);
    b2World *world = new b2World(gravedad);
    b2BodyDef movil_def;
    movil_def.type = b2_dynamicBody;
    movil_def.position.Set(0.0f, 0.0f);
    movil_def.allowSleep = false;
    movil_def.bullet = true;
    b2Body *movil = world->CreateBody(&movil_def);
    b2CircleShape disco;
    disco.m_radius = gs->granos[0]->radio;
    b2FixtureDef movil_fix_def;
    movil_fix_def.shape = &disco;
    movil_fix_def.density = gs->granos[0]->dens;
    movil_fix_def.friction = gs->granos[0]->fric;
    movil_fix_def.restitution = gs->granos[0]->rest;
    movil->CreateFixture(&movil_fix_def);
    double tStep = gs->tStep;
    int pIter = gs->pIter;
    int vIter = gs->vIter;

    double freqHz = gs->caja.frec;
    double w = 2 * b2_pi * freqHz;
    double gamma = gs->caja.Gamma;
    uint32_t nStep = 0;
    double t = 0.0;
    double mu_s = gs->caja.friccion_s;
    double mu_d = gs->caja.friccion_d;
    double peso_movil = gs->g * movil->GetMass();
    double epsilon_v = gs->caja.zero_tol;
    b2Vec2 pos_movil, vel_movil, vrel, ac_movil, F_roce;
    b2Vec2 base_vel_vec(0.0f, 0.0f);
    b2Vec2 prev_movil_vel(0.0f, 0.0f);
    cout << "# t[1] pos_base[2](mm) pos_movil[3](mm) vel_base[4](m/s) vel_movil[5](m/s) ac_base[6](m/s^2) ac_movil[7](m/s^2) vrel.x[8](m/s) f_roce[9](N)" << endl;
    while (t < gs->maxT) {
        //auto [base_pos, base_vel, base_ac] = exitacion_2f(t, gamma, w);
        auto [base_pos, base_vel, base_ac] = exitacion_mm(t, gamma, w);
        pos_movil = movil->GetPosition();
        vel_movil = movil->GetLinearVelocity();
        base_vel_vec.Set(base_vel, 0.0f);
        vrel = vel_movil - base_vel_vec;
        ac_movil = (1.0f / tStep) * (vel_movil - prev_movil_vel);
        F_roce = karnopp(vrel, epsilon_v, mu_s, mu_d, peso_movil);
        //F_roce = smooth_coulomb(vrel, epsilon_v, mu_d, peso_movil);
        //F_roce = smooth_coulomb_2(vrel, epsilon_v, 0.03, mu_d, mu_s, peso_movil);

        cout << t << " " 
             << base_pos*1000 << " " << movil->GetPosition().x*1000 << " "
             << base_vel << " " << vel_movil.x << " "
             << base_ac << " " << ac_movil.x << " "
             << vrel.x << " " << F_roce.x << " "
             << endl;
        movil->ApplyForceToCenter(F_roce, true);
        world->Step(tStep, pIter, vIter);
        world->ClearForces();
        t += tStep;
        nStep++;
        prev_movil_vel = vel_movil;
    }
    return 0;
}

b2Vec2 karnopp(b2Vec2 v, double v_tol, double mu_s, double mu_d, double p) {
    double v_norm = v.Length();
    double fr_norm = 0.0f;
    if (v_norm < v_tol) {
        try {
            fr_norm = mu_s * p / v_norm;
        } catch (const::std::overflow_error& e) {
            std::cerr << "Error de overflow: " << e.what() << std::endl;
        }
        catch (const::std::domain_error& e) {
            std::cerr << "Error de division por cero: " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Error desconocido: " << e.what() << endl;
            exit(1);
        }
        fr_norm = 0.0f;
    }
    else {
        fr_norm = mu_d * p / v_norm; 
    }
    return -fr_norm * v;
}

b2Vec2 smooth_coulomb(b2Vec2 v, double v_d, double mu_d, double p) {
    double v_norm = v.Length();
    double fr_norm = 0.0f;
    try {
        fr_norm = mu_d * p * std::tanh(v_norm / v_d) / v_norm;
    } 
    catch (const::std::overflow_error& e) {
        std::cerr << "Error de overflow: " << e.what() << std::endl;
    fr_norm = 0.0f;
    }
    catch (const::std::domain_error& e) {
        std::cerr << "Error de division por cero: " << e.what() << std::endl;
    fr_norm = 0.0f;
    }
    catch (const std::exception& e) {
        std::cerr << "Error desconocido: " << e.what() << endl;
        exit(1);
    }
    return -fr_norm * v;
}

b2Vec2 smooth_coulomb_2(b2Vec2 v, double v_d, double v_s, double mu_d, double mu_s, double p) {
    double v_norm = v.Length();
    double fr_norm = 0.0f;
    try {
        fr_norm = mu_d * p * std::tanh(v_norm / v_d) / v_norm + (mu_s - mu_d) * v_norm / v_s * std::exp(-(v_norm/v_s) * (v_norm/v_s));
    } 
    catch (const::std::overflow_error& e) {
        std::cerr << "Error de overflow: " << e.what() << std::endl;
    fr_norm = 0.0f;
    }
    catch (const::std::domain_error& e) {
        std::cerr << "Error de division por cero: " << e.what() << std::endl;
    fr_norm = 0.0f;
    }
    catch (const std::exception& e) {
        std::cerr << "Error desconocido: " << e.what() << endl;
        exit(1);
    }
    return -fr_norm * v;
}

// Función que produce una exitación armónica
Exit exitacion(double t, double gamma, double w) {
    double x, v, a;
    double Aa = gamma * gs->g;
    double Av = Aa / w;
    double Ax = Av / w;
    x = Ax * std::sin(w * t);
    v = Av * std::cos(w * t);
    a = -Aa * std::sin(w * t);
    return { x, v, a };
}

// Función que produce una exitación bi-armónica
Exit exitacion_2f(double t, double gamma, double w) {
    double x, v, a, rho = gs->caja.rho, phi = 0.5;
    double Aa = gamma * gs->g;
    double Av = Aa / w;
    double Ax = Av / w;
    x = rho * Ax * std::sin(w * t) + (1 - rho) * Ax * std::sin(2*w*t + phi);
    v = rho * Av * std::cos(w * t) + (1 - rho) * 2 * Av * std::cos(2*w*t + phi);
    a = -rho * Aa * std::sin(w * t) - 4 * (1 - rho) * Aa * std::sin(2*w*t + phi) ;
    return { x, v, a };
}

// Función que produce una exitación bi-armónica (desde aceleración como en el paper MM)
Exit exitacion_mm(double t, double gamma, double w) {
    double x, v, a, rho = gs->caja.rho, phi = gs->caja.phi;
    double Aa = gamma * gs->g;
    double Av = Aa / w;
    double Ax = Av / w;
    x = -rho * Ax * std::sin(w * t) - (1 - rho) / 4.0 * Ax * std::sin(2 * w * t + phi);
    v = -rho * Av * std::cos(w * t) - (1 - rho) / 2.0 * Av * std::cos(2 * w * t + phi);
    a = rho * Aa * std::sin(w * t) + (1 - rho) * Aa * std::sin(2 * w * t + phi) ;
    return { x, v, a };
}


