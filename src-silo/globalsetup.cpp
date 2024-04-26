/** \file globalSetup.cpp
 * \brief Archivo de implementación para la clase GlobalSetup. 
 *
 * \author Manuel Carlevaro <manuel@iflysib.unlp.edu.ar>
 * \date 2023.12.05
 * \version 0.1
 */

#include "globalsetup.hpp"

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
GlobalSetup::~GlobalSetup() {
}

/*! \fn GlobalSetup::load(string inputFile)
    * Función que lee el archivo de parámetros de control
    * \param string inputFile
    * \return void
    */
void GlobalSetup::load(string inputFile) {
    ifstream fin(inputFile.c_str());
    string ident;
    if (!fin.is_open()) {
        cout << "ERROR: No se puede abrir el archivo " << inputFile << endl;
        exit(1);
    }
    while (!fin.eof()) {
        fin >> ident;
        if (ident == "rand_seed:") {
            fin >> rnd_seed;
            if (rnd_seed <= 0) {
                cout << "ERROR: La semilla del generador de números aleatorios debe" 
                        " ser > 0" << endl;
                exit(1);
            }
        }
        // Lectura de parámetros de Box2D 
        if (ident == "altura_silo:") {
            fin >> silo.H;
            if (silo.H <= 0) {
                cout << "ERROR: la altura del silo debe ser > 0." << endl;
                exit(1);
            }
        }
        if (ident == "radio_silo:") {
            fin >> silo.R;
            if (silo.R <= 0) {
                cout << "ERROR: el radio del silo debe ser > 0." << endl;
                exit(1);
            }
        }
        if (ident == "radio_out_silo:") {
            fin >> silo.r;
            if (silo.r <= 0) {
                cout << "ERROR: El radio del orificio de salida debe ser > 0." << endl;
                exit(1);
            }
        }
        if (ident == "restitucion_silo:") {
            fin >> silo.rest;
            if (silo.rest <= 0) {
                cout << "ERROR: el coeficiente de restitución del silo debe ser > 0." << endl;
                cout << "   Valor input: " << silo.rest << endl;
                exit(1);
            }
        }
        if (ident == "friccion_silo:") {
            fin >> silo.fric;
            if (silo.fric <= 0) {
                cout << "ERROR: el coeficiente de fricción del silo debe ser > 0." << endl;
                cout << "   Valor input: " << silo.fric << endl;
                exit(1);
            }
        }
        if (ident == "atenuacion_rotacional:") {
            fin >> silo.at_rotac;
            if (silo.at_rotac <= 0 || silo.at_rotac > 1) {
                cout << "ERROR: el coeficiente de atenuación rotacional debe ser 0 <= c_rot <= 1." << endl;
                cout << "   Valor input: " << silo.at_rotac << endl;
                exit(1);
            }
        }
        if (ident == "Amplitud_exitacion_gamma:") {
            fin >> silo.Gamma;
            if (silo.Gamma <= 0) {
                cout << "ERROR: la amplitud de excitación del silo debe ser > 0." << endl;
                exit(1);
            }
        }
        if (ident == "Frecuencia_exitacion:") {
            fin >> silo.frec;
            if (silo.frec <= 0) {
                cout << "ERROR: la frecuencia de la excitación armónica debe ser > 0." << endl;
                exit(1);
            }
        }
        if (ident == "Cero_tol:") {
            fin >> silo.zero_tol;
            if (silo.zero_tol <= 0) {
                cout << "ERROR: la tolerancia para comparar con cero debe ser > 0." << endl;
                exit(1);
            }
        }
        if (ident == "rho:") {
            fin >> silo.rho;
            if (silo.rho <= 0 || silo.rho >=1) {
                cout << "ERROR: la constante eta debe ser 0 <= rho <= 1." << endl;
                exit(1);
            }
        }
        if (ident == "fase_phi:") {
            fin >> silo.phi;
        }

        // Parámetros de los granos
        if (ident == "noTipoGranos:") {
            fin >> noTipoGranos;
            if (noTipoGranos <= 0) {
                cout << "ERROR: El número de tipos granos debe ser > 0." 
                     << endl;
                exit(1);
            }
            granos = new tipoGrano*[noTipoGranos];
            for (int i = 0; i < noTipoGranos; i++) {
                granos[i] = new tipoGrano;
                fin >> granos[i]->noGranos;
                fin >> granos[i]->radio;
                fin >> granos[i]->nLados;
                fin >> granos[i]->dens;
                fin >> granos[i]->fric;
                fin >> granos[i]->fric_s;
                fin >> granos[i]->fric_d;
                fin >> granos[i]->rest;
                if (granos[i]->noGranos < 0) {
                    cout << "ERROR: El número de granos debe ser >= 0." << endl;
                exit(1);
                }
                if (granos[i]->radio < 0.01) {
                cout << "ERROR: El radio de los granos debe ser > 0.01" << endl;
                exit(1);
                }
                if ((granos[i]->nLados != 1) && 
                        (granos[i]->nLados < 1 || granos[i]->nLados > 8)) {
                            cout << "ERROR: El número de lados debe ser 1 o 2 ";
                            cout << "< nLados < 8." << endl;
                            cout << "   VALOR input: " << granos[i]->nLados << endl;
                exit(1);
                }
                if (granos[i]->dens <= 0) {
                    cout << "ERROR: La densidad de los granos debe ser > 0." 
                        << endl;
                    exit(1);
                }
                if (granos[i]->fric< 0.0) {
                    cout << "ERROR: El coeficiente de rozamiento debe ser ";
                    cout << "0 <= rozam." << endl;
                    exit(1);
                }
                if (granos[i]->rest < 0.0 || granos[i]->rest > 1.0) {
                    cout << "ERROR: El coeficiente de restitución debe ser ";
                    cout << "0 <= rest <= 1.0." << endl;
                    exit(1);
                }
                if (granos[i]->nLados > 1) {
                    granos[i]->vertices = new double*[granos[i]->nLados];
                    double x,y,theta;
                    theta = 2.0 * PI / granos[i]->nLados;
                    for (int j = 0; j < granos[i]->nLados; j++) {
                        x = granos[i]->radio*cos(j*theta); 
                        y = granos[i]->radio*sin(j*theta); 
                        granos[i]->vertices[j] = new double[2];
                        granos[i]->vertices[j][0] = x;
                        granos[i]->vertices[j][1] = y;
                    }
                }
            }
        }
        // Fin parámetros de granos
        // Parámetros de control de la simulación
        if (ident == "timeStep:") {
            fin >> tStep;
            if (tStep < 0.0) {
                cout << "ERROR: El paso de integración debe ser >= 0." << endl;
                exit(1);
            }
        }
        if (ident == "tMax:") {
            fin >> maxT;
            if (maxT < 0.0) {
                cout << "ERROR: el tiempo máximo de simulación debe ser > 0." 
                     << endl;
                exit(1);
            }
        }
        if (ident == "tBlock:") {
            fin >> tBlock;
            if (tBlock < 0.0) {
                cout << "ERROR: el tiempo de simulación con el silo bloqueado debe ser > 0." 
                     << endl;
                exit(1);
            }
        }
        if (ident == "pIter:") {
            fin >> pIter;
            if (pIter < 0) {
                cout << "ERROR: El número de iteraciones de posicion debe ser ";
                cout << " > 0." << endl;
                exit(1);
            }
        }
        if (ident == "vIter:") {
            fin >> vIter;
            if (vIter < 0) {
                cout << "ERROR: El número de iteraciones de velocidad debe ser";
                cout << " > 0." << endl;
                exit(1);
            }
        }
        if (ident == "g:") {
            fin >> g;
            if (g < 0) {
                cout << "ERROR: la magnitud de g (vertical) debe ser >= 0" << endl;
                exit(1);
            }
        }
        if (ident == "preFrameFile:") {
            fin >> preFrameFile;
        }
        if (ident == "dirID:") {
            fin >> dirID;
        }
        if (ident == "saveFrameFreq:") {
            fin >> saveFrameFreq;
            if (saveFrameFreq < 0) {
            cout << "ERROR: la frecuencia de guardado debe ser >= 0." << endl;
            exit(1);
            }
        }
        if (ident == "fluxFile:") {
            fin >> fluxFile;
        }
        if (ident == "fluxFreq:") {
            fin >> fluxFreq;
            if (fluxFreq < 0) {
                cout << "ERROR: la frecuencia de guardado del flujo debe ser ";
                cout << ">= 0." << endl;
                exit(1);
            }
        }    
        if (ident == "packing_fraction_out_freq:") {
            fin >> pf_freq;
            if (pf_freq < 0) {
            cout << "ERROR: la frecuencia de guardado del PF debe ser >= 0." << endl;
            exit(1);
            }
        }
        if (ident == "pf_file:") {
            fin >> pf_file;
        }

    } //fin bucle de lectura de inputFile
} // Fin función load()

/*! \fn GlobalSetup::printGlobalSetup()
    Función que imprime las variables contenidas en GlobalSetup
    */
void GlobalSetup::printGlobalSetup(){
    cout << "# Archivo de parámetros: " << input_par_file << endl;
    cout << "#\tValor de la semilla del generador de números aleatorios: " << rnd_seed << endl;
    cout << "# Parámetros de la simulación con Box2D: " << endl;
    cout << "# Contenedor: " << endl;
    cout << "#\tAltura del silo: " << silo.H << endl;
    cout << "#\tRadio del silo: " << silo.R << endl;
    cout << "#\tRadio del orificio de salida del silo: " << silo.r << endl;
    cout << "#\tCoeficiente de restitución del contenedor: " << silo.rest << endl;
    cout << "#\tCoeficiente de fricción del contenedor: " << silo.fric << endl;
    cout << "#\tCoeficiente de atenuación de rotaciones: " << silo.at_rotac << endl;
    cout << "#\tFrecuencia de la excitación armónica: " << silo.frec << " Hz." << endl;
    cout << "#\tAmplitud de la excitación armónica (reducida - Gamma): " << silo.Gamma << endl;
    cout << "#\tTolerancia para comparación con cero de velocidad: " << silo.zero_tol << endl;
    cout << "#\tProporción de amplitudes entre armónicos (rho): " << silo.rho << endl;
    cout << "#\tDiferencia de fase phi entre armónicos: " << silo.phi << endl;
    cout << "# Granos: " << endl;
    cout << "# \tNúmero de tipos de granos: " << noTipoGranos << endl;
    for (int i = 0; i < noTipoGranos; i++) {
        cout << "# \tGrano tipo " << i + 1 << ":" << endl;
        cout << "# \t   Número de granos: " << granos[i]->noGranos << endl;
        cout << "# \t   Radio = " << granos[i]->radio << " [m]" << endl;
        cout << "# \t   Densidad = " << granos[i]->dens << " [kg/m²]" << endl;
        cout << "# \t   Coeficiente de fricción = " << granos[i]->fric << endl;
        cout << "# \t   Coeficiente de fricción estática c/base = " << granos[i]->fric_s << endl;
        cout << "# \t   Coeficiente de fricción dinámica c/base = " << granos[i]->fric_d << endl;
        cout << "# \t   Coeficiente de fricción = " << granos[i]->fric << endl;
        cout << "# \t   Coeficiente de restitución = " << granos[i]->rest 
             << endl;
        cout << "# \t   Geometría: ";
        if (granos[i]->nLados == 1) cout << "Disco." << endl;
        else {
            cout << "# Polígono de " << granos[i]->nLados << " lados." << endl;
            cout <<"# \tVértices: " << endl;
            for (int j = 0; j < granos[i]->nLados; j++) {
                cout << "# \t\t(" << fixed << setw(4) 
                     << granos[i]->vertices[j][0] << ", " << fixed << setw(4) 
                     << granos[i]->vertices[j][1] << "), " << endl;
            }
        }
    }
    cout << "# Parámetros de control de la simulación:" << endl;
    cout << "# \t Paso de integración: " << tStep << " s."<< endl;
    cout << "# \t Tiempo de simulación con salida bloqueada: " << tBlock << " s." << endl;
    cout << "# \t Tiempo máximo de simulación: " << maxT << " s." << endl;
    cout << "# \t Iteraciones para restricciones de posición: " << pIter << endl;
    cout << "# \t Iteraciones para restricciones de velocidad: " << vIter 
         << endl;
    cout << "# \t Magnitud de g (hacia -y):" << g << endl;
    cout << "# Parámetros de estadísticas y registros:" << endl;
    cout << "#\t Identificador de carpeta y archivos: " << dirID << endl;
    cout << "# \t Prefijo de archivos de frames: " << preFrameFile << endl;
    cout << "# \t Frecuencia de guardado de frames: " << saveFrameFreq << endl;
    cout << "# \t Frecuencia de guardado del packing fraction en la salida: " << pf_freq << endl;
    cout << "# \t Archivo de guardado del packing fraction: " << pf_file << endl;

    cout << "# Fin lectura de parámetros." << endl;
}
