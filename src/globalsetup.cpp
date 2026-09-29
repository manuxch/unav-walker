/** \file globalSetup.cpp
 * \brief Archivo de implementación para la clase GlobalSetup.
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 * \date 2023.12.05
 * \version 0.1
 */

#include "globalsetup.hpp"
#include <map>

/*! \fn GlobalSetup::GlobalSetup(string iFile)
 *  Constructor de la clase GlobalSetup
 *  \param string Nombre del archivo de lectura de los parámetros de control.
 */
GlobalSetup::GlobalSetup(string iFile) : input_par_file(iFile) {
  load(input_par_file);
  printGlobalSetup();
}

/*! \fn ~GlobalSetup::GlobalSetup()
    Destructor de la clase GlobalSetup
    */
GlobalSetup::~GlobalSetup() {}

/*! \fn GlobalSetup::load(string inputFile)
 * Función que lee el archivo de parámetros de control.
 *
 * Formato: una línea "clave: valor" por parámetro. Los comentarios empiezan
 * con '#' o '//' y pueden ir al final de una línea. La clave "noTipoGranos: N"
 * debe estar seguida de N líneas con los datos de cada tipo de grano.
 *
 * La lectura es estricta: una clave desconocida, repetida o mal formada, la
 * ausencia de un parámetro obligatorio o un valor fuera de rango terminan el
 * programa con un mensaje de error.
 * \param string inputFile
 * \return void
 */
namespace {

[[noreturn]] void param_error(const string &msg) {
  cout << "ERROR (parámetros): " << msg << endl;
  exit(1);
}

string strip_comment(const string &line) {
  size_t cut = line.size();
  size_t p = line.find('#');
  if (p != string::npos) cut = p;
  p = line.find("//");
  if (p != string::npos && p < cut) cut = p;
  string s = line.substr(0, cut);
  size_t i = s.find_first_not_of(" \t\r");
  if (i == string::npos) return "";
  size_t j = s.find_last_not_of(" \t\r");
  return s.substr(i, j - i + 1);
}

double to_double(const string &key, const string &val) {
  size_t pos = 0;
  double d = 0.0;
  try {
    d = std::stod(val, &pos);
  } catch (const std::exception &) {
    param_error("valor no numérico para " + key + " '" + val + "'");
  }
  if (pos != val.size())
    param_error("valor no numérico para " + key + " '" + val + "'");
  return d;
}

long to_long(const string &key, const string &val) {
  size_t pos = 0;
  long l = 0;
  try {
    l = std::stol(val, &pos);
  } catch (const std::exception &) {
    param_error("valor entero inválido para " + key + " '" + val + "'");
  }
  if (pos != val.size())
    param_error("valor entero inválido para " + key + " '" + val + "'");
  return l;
}

bool to_bool(const string &key, const string &val) {
  if (val == "T" || val == "t") return true;
  if (val == "F" || val == "f") return false;
  param_error("valor lógico inválido para " + key + " '" + val +
              "' (use T o F)");
}

string fnv1a_hex(const string &data) {
  uint64_t h = 1469598103934665603ULL;
  for (unsigned char c : data) {
    h ^= c;
    h *= 1099511628211ULL;
  }
  std::ostringstream oss;
  oss << std::hex << std::setw(16) << std::setfill('0') << h;
  return oss.str();
}

} // namespace

void GlobalSetup::load(string inputFile) {
  ifstream fin(inputFile.c_str());
  if (!fin.is_open()) {
    cout << "ERROR: No se puede abrir el archivo " << inputFile << endl;
    exit(1);
  }
  std::stringstream buffer;
  buffer << fin.rdbuf();
  const string content = buffer.str();
  params_hash = fnv1a_hex(content);

  // Claves admitidas: true = obligatoria, false = opcional
  const std::map<string, bool> known = {
      {"rand_seed:", true},
      {"altura_silo:", true},
      {"radio_silo:", true},
      {"radio_out_silo:", true},
      {"restitucion_silo:", true},
      {"friccion_silo:", true},
      {"Amplitud_exitacion_gamma:", true},
      {"Frecuencia_exitacion:", true},
      {"Cero_tol:", true},
      {"rho:", true},
      {"fase_phi:", false},
      {"noTipoGranos:", true},
      {"timeStep:", true},
      {"tMax:", true},
      {"tBlock:", true},
      {"t_Register:", true},
      {"maxGranosDesc:", false},
      {"pIter:", true},
      {"vIter:", true},
      {"g:", true},
      {"do_reinyection:", true},
      {"fondo_medicion:", false},
      {"continuous_physics:", false},
      {"dirID:", true},
      {"preFrameFile:", true},
      {"saveFrameFreq:", true},
      {"fluxFile:", true},
      {"fluxFreq:", true},
      {"packing_fraction_out_freq:", false},
      {"pf_file:", false},
      {"freq_perfiles:", false},
      {"n_bin_perfiles:", false},
      {"save_ve_freq:", false},
      {"freq_save_contacts:", false},
      {"save_tensors_freq:", false},
      {"check_balance_freq:", false},
      {"save_roi_only:", false},
      {"x_roi:", false},
      {"y_min_roi:", false},
      {"y_max_roi:", false},
  };

  std::map<string, string> vals;
  vector<string> grain_lines;
  int pending_grains = 0;
  std::istringstream lines(content);
  string raw;
  int n_line = 0;
  while (std::getline(lines, raw)) {
    ++n_line;
    string line = strip_comment(raw);
    if (line.empty()) continue;
    if (pending_grains > 0) {
      grain_lines.push_back(line);
      --pending_grains;
      continue;
    }
    std::istringstream iss(line);
    string key, val, extra;
    iss >> key >> val;
    if (key.empty() || key.back() != ':' || val.empty() || (iss >> extra))
      param_error("línea " + std::to_string(n_line) + " mal formada: '" + line +
                  "'");
    if (key == "atenuacion_rotacional:")
      param_error("atenuacion_rotacional ya no se usa (línea " +
                  std::to_string(n_line) +
                  "): la rotación se amortigua por fricción de pivoteo con la "
                  "base, con fric_b_s y fric_b_d. Elimine la línea.");
    if (!known.count(key))
      param_error("clave desconocida '" + key + "' en la línea " +
                  std::to_string(n_line));
    if (vals.count(key))
      param_error("clave repetida '" + key + "' en la línea " +
                  std::to_string(n_line));
    vals[key] = val;
    if (key == "noTipoGranos:") {
      long n = to_long(key, val);
      if (n <= 0) param_error("noTipoGranos debe ser > 0.");
      pending_grains = static_cast<int>(n);
    }
  }
  if (pending_grains > 0)
    param_error("faltan " + std::to_string(pending_grains) +
                " líneas de tipos de granos.");
  for (const auto &[key, required] : known) {
    if (required && !vals.count(key))
      param_error("falta el parámetro obligatorio '" + key + "'");
  }

  auto has = [&](const string &k) { return vals.count(k) > 0; };
  auto D = [&](const string &k) { return to_double(k, vals.at(k)); };
  auto L = [&](const string &k) { return to_long(k, vals.at(k)); };
  auto B = [&](const string &k) { return to_bool(k, vals.at(k)); };
  auto check = [](bool ok, const string &msg) {
    if (!ok) param_error(msg);
  };

  long seed = L("rand_seed:");
  check(seed > 0, "rand_seed debe ser > 0.");
  rnd_seed = static_cast<uint32_t>(seed);

  // Contenedor y excitación
  silo.H = D("altura_silo:");
  silo.R = D("radio_silo:");
  silo.r = D("radio_out_silo:");
  silo.rest = D("restitucion_silo:");
  silo.fric = D("friccion_silo:");
  silo.Gamma = D("Amplitud_exitacion_gamma:");
  silo.frec = D("Frecuencia_exitacion:");
  silo.zero_tol = D("Cero_tol:");
  silo.rho = D("rho:");
  silo.phi = has("fase_phi:") ? D("fase_phi:") : 0.0;
  check(silo.H > 0, "altura_silo debe ser > 0.");
  check(silo.R > 0, "radio_silo debe ser > 0.");
  check(silo.r > 0 && silo.r < silo.R,
        "radio_out_silo debe cumplir 0 < radio_out_silo < radio_silo.");
  check(silo.rest >= 0 && silo.rest <= 1,
        "restitucion_silo debe estar en [0, 1].");
  check(silo.fric >= 0, "friccion_silo debe ser >= 0.");
  check(silo.Gamma > 0, "Amplitud_exitacion_gamma debe ser > 0.");
  check(silo.frec > 0, "Frecuencia_exitacion debe ser > 0.");
  check(silo.zero_tol > 0, "Cero_tol debe ser > 0.");
  check(silo.rho > 0 && silo.rho < 1, "rho debe cumplir 0 < rho < 1.");

  // Granos: noGranos radio nLados dens fric fric_b_s fric_b_d rest
  noTipoGranos = static_cast<int>(L("noTipoGranos:"));
  granos = new tipoGrano *[noTipoGranos];
  for (int i = 0; i < noTipoGranos; i++) {
    std::istringstream iss(grain_lines[i]);
    tipoGrano *gr = new tipoGrano{};
    string extra;
    if (!(iss >> gr->noGranos >> gr->radio >> gr->nLados >> gr->dens >>
          gr->fric >> gr->fric_s >> gr->fric_d >> gr->rest) ||
        (iss >> extra))
      param_error("línea de tipo de grano " + std::to_string(i + 1) +
                  " mal formada: '" + grain_lines[i] +
                  "' (se esperan 8 valores: noGranos radio nLados dens fric "
                  "fric_b_s fric_b_d rest)");
    const string id = "grano tipo " + std::to_string(i + 1) + ": ";
    check(gr->noGranos >= 0, id + "el número de granos debe ser >= 0.");
    check(gr->radio >= 0.01, id + "el radio debe ser >= 0.01.");
    check(gr->nLados == 1,
          id + "por ahora solo se admiten discos (nLados = 1).");
    check(gr->dens > 0, id + "la densidad debe ser > 0.");
    check(gr->fric >= 0, id + "el coeficiente de rozamiento debe ser >= 0.");
    check(gr->fric_s >= 0 && gr->fric_d >= 0,
          id + "los coeficientes de fricción con la base deben ser >= 0.");
    check(gr->fric_s >= gr->fric_d,
          id + "debe cumplirse fric_b_s >= fric_b_d.");
    check(gr->rest >= 0 && gr->rest <= 1,
          id + "la restitución debe estar en [0, 1].");
    gr->vertices = nullptr;
    granos[i] = gr;
  }

  // Control de la simulación
  tStep = D("timeStep:");
  maxT = D("tMax:");
  tBlock = D("tBlock:");
  t_register = D("t_Register:");
  pIter = static_cast<int>(L("pIter:"));
  vIter = static_cast<int>(L("vIter:"));
  g = D("g:");
  reinyection = B("do_reinyection:");
  maxGranosDesc =
      has("maxGranosDesc:") ? static_cast<int>(L("maxGranosDesc:")) : 0;
  fondo_medicion = has("fondo_medicion:") ? B("fondo_medicion:") : false;
  continuous_physics =
      has("continuous_physics:") ? B("continuous_physics:") : false;
  check(tStep > 0, "timeStep debe ser > 0.");
  check(maxT > 0, "tMax debe ser > 0.");
  check(tBlock >= 0 && tBlock <= maxT, "tBlock debe estar en [0, tMax].");
  check(t_register >= 0 && t_register <= maxT,
        "t_Register debe estar en [0, tMax].");
  check(pIter > 0 && vIter > 0, "pIter y vIter deben ser > 0.");
  check(g >= 0, "g debe ser >= 0.");
  check(maxGranosDesc >= 0, "maxGranosDesc debe ser >= 0.");

  // Salidas
  dirID = vals.at("dirID:");
  preFrameFile = vals.at("preFrameFile:");
  fluxFile = vals.at("fluxFile:");
  saveFrameFreq = static_cast<int>(L("saveFrameFreq:"));
  fluxFreq = static_cast<int>(L("fluxFreq:"));
  auto opt_freq = [&](const string &k) {
    int f = has(k) ? static_cast<int>(L(k)) : 0;
    check(f >= 0, k + " debe ser >= 0.");
    return f;
  };
  pf_freq = opt_freq("packing_fraction_out_freq:");
  freq_perfiles = opt_freq("freq_perfiles:");
  save_ve_freq = opt_freq("save_ve_freq:");
  save_contact_freq = opt_freq("freq_save_contacts:");
  save_tensors_freq = opt_freq("save_tensors_freq:");
  check_balance_freq = opt_freq("check_balance_freq:");
  check(saveFrameFreq >= 0, "saveFrameFreq debe ser >= 0.");
  check(fluxFreq >= 0, "fluxFreq debe ser >= 0.");
  if (has("pf_file:")) pf_file = vals.at("pf_file:");
  if (has("n_bin_perfiles:"))
    n_bin_perfiles = static_cast<int>(L("n_bin_perfiles:"));
  check(n_bin_perfiles > 0, "n_bin_perfiles debe ser > 0.");

  // ROI
  save_roi_only = has("save_roi_only:") ? B("save_roi_only:") : false;
  if (save_roi_only) {
    check(has("x_roi:") && has("y_min_roi:") && has("y_max_roi:"),
          "con save_roi_only: T se requieren x_roi, y_min_roi e y_max_roi.");
    x_roi = D("x_roi:");
    y_min_roi = D("y_min_roi:");
    y_max_roi = D("y_max_roi:");
    check(x_roi > 0, "x_roi debe ser > 0.");
    check(y_max_roi > y_min_roi, "y_max_roi debe ser > y_min_roi.");
  }
} // Fin función load()

/*! \fn GlobalSetup::printGlobalSetup()
    Función que imprime las variables contenidas en GlobalSetup
    */
void GlobalSetup::printGlobalSetup() {
  cout << "# Archivo de parámetros: " << input_par_file << endl;
  cout << "#\tHash FNV-1a del archivo de parámetros: " << params_hash << endl;
  cout << "#\tValor de la semilla del generador de números aleatorios: "
       << rnd_seed << endl;
  cout << "# Parámetros de la simulación con Box2D: " << endl;
  cout << "# Contenedor: " << endl;
  cout << "#\tAltura del silo: " << silo.H << endl;
  cout << "#\tRadio del silo: " << silo.R << endl;
  cout << "#\tRadio del orificio de salida del silo: " << silo.r << endl;
  cout << "#\tCoeficiente de restitución del contenedor: " << silo.rest << endl;
  cout << "#\tCoeficiente de fricción del contenedor: " << silo.fric << endl;
  cout << "#\tFrecuencia de la excitación armónica: " << silo.frec << " Hz."
       << endl;
  cout << "#\tAmplitud de la excitación armónica (reducida - Gamma): "
       << silo.Gamma << endl;
  cout << "#\tTolerancia para comparación con cero de velocidad: "
       << silo.zero_tol << endl;
  cout << "#\tProporción de amplitudes entre armónicos (rho): " << silo.rho
       << endl;
  cout << "#\tDiferencia de fase phi entre armónicos: " << silo.phi << endl;
  cout << "# Granos: " << endl;
  cout << "# \tNúmero de tipos de granos: " << noTipoGranos << endl;
  for (int i = 0; i < noTipoGranos; i++) {
    cout << "# \tGrano tipo " << i + 1 << ":" << endl;
    cout << "# \t   Número de granos: " << granos[i]->noGranos << endl;
    cout << "# \t   Radio = " << granos[i]->radio << " [m]" << endl;
    cout << "# \t   Densidad = " << granos[i]->dens << " [kg/m²]" << endl;
    cout << "# \t   Coeficiente de fricción = " << granos[i]->fric << endl;
    cout << "# \t   Coeficiente de fricción estática c/base = "
         << granos[i]->fric_s << endl;
    cout << "# \t   Coeficiente de fricción dinámica c/base = "
         << granos[i]->fric_d << endl;
    cout << "# \t   Coeficiente de fricción = " << granos[i]->fric << endl;
    cout << "# \t   Coeficiente de restitución = " << granos[i]->rest << endl;
    cout << "# \t   Geometría: ";
    if (granos[i]->nLados == 1)
      cout << "Disco." << endl;
    else {
      cout << "# Polígono de " << granos[i]->nLados << " lados." << endl;
      cout << "# \tVértices: " << endl;
      for (int j = 0; j < granos[i]->nLados; j++) {
        cout << "# \t\t(" << fixed << setw(4) << granos[i]->vertices[j][0]
             << ", " << fixed << setw(4) << granos[i]->vertices[j][1] << "), "
             << endl;
      }
    }
  }
  cout << "# Parámetros de control de la simulación:" << endl;
  cout << "# \t Paso de integración: " << tStep << " s." << endl;
  cout << "# \t Tiempo de simulación con salida bloqueada: " << tBlock << " s."
       << endl;
  cout << "# \t Tiempo máximo de simulación: " << maxT << " s." << endl;
  cout << "# \t Máx. granos descargados para parar: ";
  if (maxGranosDesc > 0)
    cout << maxGranosDesc << endl;
  else
    cout << "(deshabilitado)" << endl;
  cout << "# \t Iteraciones para restricciones de posición: " << pIter << endl;
  cout << "# \t Iteraciones para restricciones de velocidad: " << vIter << endl;
  cout << "# \t Magnitud de g (hacia -y):" << g << endl;
  cout << "# \t Se realiza reinyección de granos? ";
  cout << (reinyection ? "Si." : "No.") << endl;
  cout << "# \t Detección continua de colisiones (TOI, bullets)? "
       << (continuous_physics ? "Si." : "No.") << endl;
  cout << "# \t Silo cerrado con fondo de medición (gID=-200)? "
       << (fondo_medicion ? "Si." : "No.") << endl;

  cout << "# Parámetros de estadísticas y registros:" << endl;
  cout << "# \t Identificador de carpeta y archivos: " << dirID << endl;
  cout << "# \t Tiempo de inicio de registros: " << t_register << endl;
  cout << "# \t Prefijo de archivos de frames: " << preFrameFile << endl;
  cout << "# \t Frecuencia de guardado de frames: " << saveFrameFreq << endl;
  cout << "# \t Frecuencia de guardado del packing fraction en la salida: "
       << pf_freq << endl;
  cout << "# \t Archivo de guardado del packing fraction: " << pf_file << endl;
  cout << "# \t Frecuencia de actualización de perfiles pf-v: " << freq_perfiles
       << endl;
  cout << "# \t Número de bins en perfiles pf-v: " << n_bin_perfiles << endl;
  cout << "# \t Frecuencia de guardado de velocidades y energías: "
       << save_ve_freq << endl;
  cout << "# \t Frecuencia de guardado de tensores de estrés: "
       << save_tensors_freq << endl;
  cout << "# \t Frecuencia de guardado de fuerzas de contacto: "
       << save_contact_freq << endl;
  cout << "# \t Frecuencia del chequeo de balance de fuerzas: "
       << check_balance_freq << endl;
  cout << "# \t Guardar solo partículas en ROI: "
       << (save_roi_only ? "Si" : "No") << endl;
  if (save_roi_only) {
    cout << "# \t ROI: x = [-" << x_roi << ", " << x_roi << "], y = ["
         << y_min_roi << ", " << y_max_roi << "]" << endl;
  }

  cout << "# Fin lectura de parámetros." << endl;
}
