import csv
import time
import serial


PUERTO = "COM9"
BAUDIOS = 9600
ARCHIVO_CSV = "datos_sensores.csv"
DURACION_CAPTURA = 60


print("\nIniciando captura de datos...")
print(f"Puerto: {PUERTO}")
print(f"Tiempo de captura: {DURACION_CAPTURA} segundos")
print(f"Archivo de salida: {ARCHIVO_CSV}\n")


arduino = serial.Serial(PUERTO, BAUDIOS, timeout=1)
time.sleep(2)

archivo = open(ARCHIVO_CSV, "w", newline="", encoding="utf-8")
writer = csv.writer(archivo)
writer.writerow(["fecha_hora", "tiempo_s", "temperatura", "humedad", "distancia"])

inicio = time.time()
contador_registros = 0

try:
    while True:
        tiempo_actual = time.time()
        tiempo_transcurrido = int(tiempo_actual - inicio)

        if tiempo_transcurrido >= DURACION_CAPTURA:
            break

        linea = arduino.readline().decode("utf-8", errors="ignore").strip()

        if not linea:
            continue

        fecha_hora = time.strftime("%Y-%m-%d %H:%M:%S")

        try:
            if linea.startswith("DATA"):
                partes = linea.split(",")
                temperatura = float(partes[1])
                humedad = float(partes[2])

                writer.writerow([fecha_hora, tiempo_transcurrido, temperatura, humedad, ""])
                archivo.flush()
                contador_registros += 1

                print(f"[{tiempo_transcurrido:4d}s] Temp: {temperatura:.2f} C | Hum: {humedad:.2f} %")

            elif linea.startswith("[Distancia]"):
                distancia = float(linea.split(":")[1])

                writer.writerow([fecha_hora, tiempo_transcurrido, "", "", distancia])
                archivo.flush()
                contador_registros += 1

                print(f"[{tiempo_transcurrido:4d}s] Distancia: {distancia:.2f} cm")

        except (ValueError, IndexError):
            print("Linea ignorada:", linea)

except KeyboardInterrupt:
    print("\nCaptura detenida manualmente.")

arduino.close()
archivo.close()

print(f"\nCaptura finalizada. Registros guardados: {contador_registros}")
