# Input file for agdamper
# Author: Manuel Carlevaro
# Date: 2023.12.05 
#
# Semilla del generador de números aleatorios
rand_seed: 1729
#
# Parámetros de Box2D
# Nota: Box2D utiliza el sistema métrico MKS
#
# Parámetros del contenedor
# [altura_silo]: alto de la silo (dirección y)
# [base_silo]: ancho de la silo (dirección x=
# [friccion_silo]: coeficiente de fricción de la silo
# [restitucion_silo]: coeficiente de restitución del silo
# [atenuacion_rotacional]: coeficiente de atenuación de la velocidad de rotación de los granos
#   Control de la excitación
# [Amplitud_exitacion]: amplitud de la excitación armónica
# [Frecuencia_exitacion]: frecuencia de la excitación armónica (Hz)
# [K_resorte]: constante elástica del resorte
# [resorte_L0]: longitud del resorte en reposo
# [Viscosidad_amortiguador]: constante de amortiguación viscosa
altura_silo: 40
radio_silo: 7
radio_out_silo: __rOut__
restitucion_silo: 0.1
atenuacion_rotacional: 0.95
Amplitud_exitacion_gamma: 1.0
Frecuencia_exitacion: 1.10656667
Cero_tol: 0.0001  
rho: 0.5
fase_phi: 0.0

# Granos
#	[noTipoGranos]: cantidad de tipos de granos diferentes
#	Por cada tipo de grano:
#	[noGranos radio nLados dens fric fric_b_s fric_b_d rest ]
#	(int double int double double double)
#	Notas:
#	    [noGranos]: Cantidad de granos de cada tipo
#		[radio] = m 
#		[nLados] = 1 es un disco, o 3 <= nLados <= 8
#       [dens] = kg/m¹
#		[fric]: Coeficiente de fricción grano-grano 0 <= fric 
#		[fric_b_s]: Coeficiente de fricción estática del grano con la base 0 <= fric_s 
#		[fric_b_d]: Coeficiente de fricción dinámica del grano con la base 0 <= fric_d 
#		[rest]: Coeficiente de restitución grano-grano <= rest <= 1
noTipoGranos: 1
450 0.5 1 1.273239545 0.16 0.2 0.16 0.5 
# Notas:
#   - Radio de triángulo con = área que disco de r = 1: 0.777560
#   - Radio de cuadrado con = área que disco de r = 1:  0.626657
#
# Control de la simulación
# [timeStep]: Paso de integración del movimiento (en s)
# [tMax]: Máximo tiempo de simulación (en s)
# [tBlock]: Tiempo de simulación con bloqueo de salida (en s)
# [pIter]: iteraciones de posicion para la satisfacción de restricciones
# [vIter]: iteraciones de velocidad para la satisfacción de restricciones
# [g]: Magnitud de la aceleración de la gravedad (hacia -y) kg m / s²
# [n_periodos_medida]: Cantidad de ciclos para mediciones de fuerza, posición, etc.
timeStep: 0.005 
tMax: 5000.0
tBlock: 500.0
pIter: 30
vIter: 30
g: 1.0

# Parámetros de registro
# [dirID]: sufijo del nombre del directorio de guardado de frames
# [preFrameFile]: prefijo del archivo de salida de frames
# [saveFrameFreq]: Paso de guardado de los frames
dirID: walker
preFrameFile: frm
saveFrameFreq: 0
fluxFile: flx-__C__.dat
fluxFreq: 5
#
