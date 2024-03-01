def obtener_amplitud(archivo):
  """
  Función que lee un archivo hasta encontrar la línea que contiene
  "#      Amplitud de la excitación armónica (reducida - Gamma): 0.4"
  y devuelve el valor 0.4.

  Parámetros:
    archivo: Ruta al archivo que se desea leer.

  Retorno:
    El valor 0.4 como un float.
  """

  with open(archivo, "r") as f:
    for linea in f:
        l = linea.split(":")
        if 'Gamma' in l[0]:
            return float(l[-1])
            break
  return None


def obtener_fase(archivo):
  """
  Función que lee un archivo hasta encontrar la línea que contiene
  "#       Diferencia de fase phi entre armónicos: 0.2"
  y devuelve el valor 0.2.

  Parámetros:
    archivo: Ruta al archivo que se desea leer.

  Retorno:
    El valor 0.4 como un float.
  """

  with open(archivo, "r") as f:
    for linea in f:
        l = linea.split(":")
        if 'fase' in l[0]:
            return float(l[-1])
            break
  return None
