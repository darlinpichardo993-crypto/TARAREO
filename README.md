# TARAREO  ·  Made By ORYMY And YMY Records

Plugin VST3 para FL Studio: tarareas, y la IA escribe la letra y la canta en tu tonalidad y melodia.

## Obtener el .exe (sin instalar nada en tu PC)
1. Crea un repositorio en GitHub y sube esta carpeta.
2. Pestana **Actions** -> "Build TARAREO (Windows)" (corre solo al subir).
3. Descarga el artefacto **TARAREO-Setup** -> `TARAREO-Setup.exe`.
4. Ejecutalo: instala el VST3 en `C:\Program Files\Common Files\VST3\`.
5. En FL Studio: Options -> Manage plugins -> Find installed plugins.

## Backend (la IA)
    cd backend
    pip install -r requirements.txt
    uvicorn server:app --host 127.0.0.1 --port 8000
Opcional: `set ANTHROPIC_API_KEY=tu_clave` para letras reales. Para usar un servidor remoto: variable `TARAREO_SERVER=https://tu-servidor`.

## Estado
- Plugin, interfaz, grabacion, deteccion de notas/tonalidad, letra y flujo de arrastrar audio: listos.
- La voz es un **sintetizador de prueba** (`sing()` en backend/server.py). Falta conectar un motor de voz cantada real (DiffSinger / RVC con tu voz).
