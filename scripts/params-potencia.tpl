# Archivo de parámetros de unav-walkers para la validación de la potencia de
# contacto (propuesta de 2026-12). Ver README.md y
# docs/validacion-potencia. Marcadores: __pre__ (prefijo), __radio__ (r),
# __paredes__ (T/F), __N0__, __H__, __Gamma__, __rho__, __phi__, __mus__,
# __mud__, __tMax__; start-potencia.sh los completa.
# Formato: "clave: valor"; los comentarios empiezan con '#' o '//'.
# Unidades reducidas: diámetro de grano d = 1 (radio 0.5), g = 1.
#
# Semilla del generador de números aleatorios (la simulación es reproducible)
rand_seed: 1729
#
# Contenedor (el silo ocupa |x| <= R, 0 <= y <= H; el orificio, |x| <= r en y = 0)
# [altura_silo]: altura H
# [radio_silo]: semiancho R
# [radio_out_silo]: semiancho del orificio r (D = 2 r)
# [restitucion_silo]: restitución de las paredes. Box2D combina restituciones
#     con el máximo, así que domina la del grano si es mayor.
# [friccion_silo]: fricción de las paredes. Box2D combina fricciones como
#     sqrt(f_grano * f_silo): con f_silo = f_grano la fricción grano-pared es
#     igual a la grano-grano.
# Excitación de la base: a(t) = Gamma g [rho sin(w t) + (1 - rho) sin(2 w t + phi)]
# [Amplitud_exitacion_gamma]: aceleración reducida Gamma
# [Frecuencia_exitacion]: frecuencia f del primer armónico (w = 2 pi f)
# [Cero_tol]: umbral de velocidad relativa para la adherencia a la base
# [rho]: peso del primer armónico (0 < rho < 1)
# [fase_phi]: fase del segundo armónico (opcional, por defecto 0)
altura_silo: __H__
radio_silo: 18.6
radio_out_silo: __radio__
restitucion_silo: 0.1
friccion_silo: 0.13
Amplitud_exitacion_gamma: __Gamma__
Frecuencia_exitacion: 0.45175395
Cero_tol: 0.00001
rho: __rho__
fase_phi: __phi__

# Granos
#	[noTipoGranos]: cantidad de tipos de granos, seguida de una línea por tipo:
#	noGranos radio nLados dens fric fric_b_s fric_b_d rest
#	    [noGranos]: cantidad de granos de ese tipo
#	    [radio]: radio del disco
#	    [nLados]: 1 (por ahora solo se admiten discos)
#	    [dens]: densidad (masa por unidad de área); 1.273239545 = 4/pi da m = 1
#	    [fric]: coeficiente de fricción grano-grano (>= 0)
#	    [fric_b_s]: coeficiente de fricción estática con la base (>= fric_b_d)
#	    [fric_b_d]: coeficiente de fricción dinámica con la base (>= 0)
#	    [rest]: coeficiente de restitución grano-grano (0 <= rest <= 1)
noTipoGranos: 1
__N0__ 0.5 1 1.273239545 0.13 __mus__ __mud__ 0.5
#
# Control de la simulación
# [timeStep]: paso de integración dt
# [tMax]: tiempo máximo de simulación
# [tBlock]: tiempo con el orificio cerrado (deposición)
# [t_Register]: comienzo de los registros
# [maxGranosDesc]: detener al descargar este número de granos (opcional; 0 = no)
# [pIter], [vIter]: iteraciones de posición y de velocidad del solver de Box2D
# [g]: gravedad; solo fija la carga normal m g sobre la base
# [do_reinyection]: T = reinyectar los granos descargados; F = eliminarlos
timeStep: 0.004427188738  // T/500, T = 1/0.45175395 = 2.2135944 (500 pasos por período; un cuadro a 2000 fps = 5 pasos)
tMax: __tMax__  // cota: la simulación termina antes, tras periodos_registro períodos
tBlock: 750.0
t_Register: 750.0  // mínimo; el registro empieza según granos_registro
maxGranosDesc: 0  // 0 = deshabilitado; >0 = detener cuando salgan N granos
pIter: 60
vIter: 60
g: 1.0
do_reinyection: F  // sin reinyección: los granos se eliminan en y_eliminacion
# [fondo_medicion]: T = silo cerrado con fondo de medición (gID=-200) y sin
#     descarga; la fuerza sobre el fondo se guarda en cada paso en
#     frames_<dirID>/wall_force_<preFrameFile>.dat. F = silo con orificio.
fondo_medicion: F
# [continuous_physics]: (opcional, por defecto F) T = detección continua de
#     colisiones (TOI) y granos "bullet". Los impulsos de los subpasos TOI no
#     quedan registrados en los contactos, así que las fuerzas guardadas quedan
#     incompletas. Con dt = 0.005 el desplazamiento por paso es ~1e-3 << radio.
continuous_physics: F

# Eliminación de los granos descargados (sin paredes laterales aguas abajo)
# [y_eliminacion]: se eliminan debajo de esta altura. Se monitorean 40 d
#     debajo del orificio; 10 d más abajo, para no perturbar lo medido.
# [granos_registro]: el registro empieza al inicio del primer período con
#     esta cantidad de granos o menos en el silo.
# [periodos_registro]: la simulación termina tras estos períodos registrados.
# [paredes_moviles]: T = el silo y la tapa se mueven con la base.
y_eliminacion: -50
granos_registro: 2000
periodos_registro: 40
paredes_moviles: __paredes__
#
# Registros (frecuencias en pasos; 0 = deshabilitado). Los archivos se escriben
# en frames_<dirID>/ y los de un mismo instante comparten el número de frame
# (pasos desde t_Register).
# [dirID]: sufijo del directorio de salida
# [preFrameFile]: prefijo de los archivos de frames
# [saveFrameFreq]: configuración (.xy)
# [save_tensors_freq]: tensor de estrés por grano (.sxy)
# [fluxFile]: archivo de granos descargados
# [fluxFreq]: > 0 habilita el archivo de granos descargados
# [packing_fraction_out_freq]: packing fraction en el bulk y en el orificio
# [pf_file]: archivo del packing fraction
# [freq_perfiles]: perfiles de ocupación y velocidad sobre el orificio
# [n_bin_perfiles]: bines de esos perfiles
# [freq_save_contacts]: fuerzas de contacto (fc_*.dat)
# [save_ve_freq]: velocidades y energías (.ve)
# [check_balance_freq]: chequeo de balance de impulso por grano
#     (frames_<dirID>/balance_<preFrameFile>.dat)
# [save_energy]: balance de energía de la base por período y por fase
#     (frames_<dirID>/energy_<preFrameFile>.dat y energy_fase_<...>.dat)
dirID: walker
preFrameFile: __pre__
saveFrameFreq: 0
save_tensors_freq: 5
fluxFile: flx-__pre__.dat
fluxFreq: 5
packing_fraction_out_freq: 0
pf_file: pf-__pre__.dat
freq_perfiles: 0
n_bin_perfiles: 40
freq_save_contacts: 5
save_ve_freq: 5
check_balance_freq: 0
save_energy: T
# [save_power]: potencia exacta de los contactos (columnas nuevas en energy_*)
# [save_fz_freq]: <pre>_<frame>.fz por grano (fuerzas y potencias promedio)
# [save_power_maps]: potencia por celdas y fase, y flujos entre regiones
# [comprimir_salidas]: archivos por frame en gzip, con 9 cifras
save_power: T
save_fz_freq: 5
save_power_maps: T
comprimir_salidas: T
#
# Región de interés (ROI) para las salidas por grano y por contacto
# [save_roi_only]: F = se guarda todo el sistema; T = solo el rectángulo
#     |x| <= x_roi, y_min_roi <= y <= y_max_roi (centros de los granos; para los
#     contactos, todos los de los granos cuyo centro está en el ROI).
save_roi_only: F
x_roi: __radio__
y_min_roi: -2.0
y_max_roi: 2.0
