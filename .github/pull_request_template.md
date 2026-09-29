## Qué cambia

<!-- En una frase. Si arregla un fallo, pon aquí el número de la incidencia. -->

## Por qué

<!-- El problema que se resuelve, no la solución. Si es una idea que aún no está
     decidida, dilo: la discusión va aquí. -->

## Cómo lo he comprobado

<!-- Qué has ejecutado y qué has visto. Si puede reproducirse con
     tools/e2e-test.sh, cuenta aquí los pasos. -->

```sh
# comandos y resultado
```

- [ ] `ctest --test-dir build --output-on-failure` en verde
- [ ] `python3 tools/check-i18n.py` en verde
- [ ] `python3 tools/check-icons.py build` en verde
- [ ] Añadí o ajusté pruebas para este cambio
- [ ] Si toqué la interfaz: mantuve el orden de la barra, los atajos y los textos
- [ ] Si añadí un idioma o un icono: están en `res/` y en la lista de recursos de
      `CMakeLists.txt`
- [ ] No subí binarios de RARLAB, claves de licencia ni el `.deb` generado
- [ ] Actualicé la documentación afectada (`README.md`, `docs/`, `CHANGELOG.md`)

## Capturas

<!-- Si cambia algo visible, una captura antes y otra después. -->
