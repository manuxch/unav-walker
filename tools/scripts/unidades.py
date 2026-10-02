"""Conversión de unidades de la simulación (reducidas) a las experimentales.

Único lugar donde se definen los factores de conversión; los scripts de
gráficos deben importarlos de aquí. Valores de utils/reduced_units.ods:

    longitud   d = 0.005 m        (diámetro de los discos; en la simulación, 1)
    masa       m = 2.10e-4 kg     (masa de un disco; en la simulación, 1)
    tiempo     t = sqrt(d / g) = 0.0225876975726313 s
    velocidad  d / t = 0.221359436211787 m/s
    fuerza     m g = 0.002058 N
    estrés 2D  F / L = 0.4116 N/m
    energía    m g d = 1.029e-5 J

Coordenadas: el eje y de la simulación es opuesto al de los experimentos, y
el origen está en el orificio. Al invertir y, cambian de signo las
componentes con un único índice y: v_y y las componentes xy, yx de los
tensores. Los errores (magnitudes positivas) no cambian de signo.
"""

import re

import numpy as np

LONGITUD = 0.005  # m
MASA = 2.10e-4  # kg
TIEMPO = 0.0225876975726313  # s
VELOCIDAD = LONGITUD / TIEMPO  # m/s
FUERZA = MASA * LONGITUD / TIEMPO**2  # N (= m g)
ESTRES_2D = FUERZA / LONGITUD  # N/m
ENERGIA = FUERZA * LONGITUD  # J (= m g d)

CM = 100.0  # m -> cm


def x_a_cm(x):
    """Coordenada x de la simulación a cm."""
    return np.asarray(x, dtype=float) * LONGITUD * CM


def y_a_cm(y):
    """Coordenada y de la simulación a cm, con el eje invertido."""
    return -np.asarray(y, dtype=float) * LONGITUD * CM


def vx_a_cm_s(vx):
    """Velocidad v_x de la simulación a cm/s."""
    return np.asarray(vx, dtype=float) * VELOCIDAD * CM


def vy_a_cm_s(vy):
    """Velocidad v_y de la simulación a cm/s, con el eje invertido."""
    return -np.asarray(vy, dtype=float) * VELOCIDAD * CM


def w_a_rad_s(w):
    """Velocidad angular de la simulación a rad/s, con el eje y invertido.

    Invertir y es una reflexión: cambia el sentido de giro, así que w cambia
    de signo.
    """
    return -np.asarray(w, dtype=float) / TIEMPO


def energia_a_J(e):
    """Energía de la simulación a J."""
    return np.asarray(e, dtype=float) * ENERGIA


def fuerza_a_N(f):
    """Fuerza (o módulo de fuerza) de la simulación a N."""
    return np.asarray(f, dtype=float) * FUERZA


def es_tensor_cruzado(nombre):
    """True para las componentes xy o yx de un tensor (sxy, snyx, kxy, ...)."""
    return re.fullmatch(r"(s|sn|st|k)(xy|yx)", nombre) is not None


def es_estres(nombre):
    """True para las columnas con unidades de estrés (tensores y errores)."""
    return re.fullmatch(r"(e_)?(s|sn|st|k)(xx|xy|yx|yy)", nombre) is not None


def convertir_perfil(datos, experimentales=True, continuo=False):
    """Convierte un perfil de stress_profile (dict columna -> array).

    - experimentales: y en cm, con el eje invertido; velocidades en m/s;
      estrés en N/m. Si es False, se dejan las unidades de la simulación.
    - continuo: multiplica el estrés (y sus errores) por phi, para obtener
      el estrés medio del bin en lugar del estrés sobre los granos.
    Devuelve un dict nuevo.
    """
    out = {k: np.array(v, dtype=float) for k, v in datos.items()}
    if continuo:
        for k in out:
            if es_estres(k):
                out[k] = out[k] * out["phi"]
    if not experimentales:
        return out
    out["y"] = -out["y"] * LONGITUD * CM
    out["vx"] = out["vx"] * VELOCIDAD
    out["vy"] = -out["vy"] * VELOCIDAD
    for k in list(out):
        if es_estres(k):
            out[k] = out[k] * ESTRES_2D
            if es_tensor_cruzado(k):
                out[k] = -out[k]
    return out
