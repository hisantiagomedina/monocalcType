# MONOCALC TYPE 0.1.0
## SDD particular — Spec-Driven Development

**Documento:** SDD-TYPE-0.1.0  
**Revisión:** Borrador 0.6 — segunda sesión: corrección, cierre funcional y validación reproducible
**Contrato superior:** SDD común monocalc 0.1.0, Borrador 0.3  
**Producto:** monocalc Type (independiente de monocalc Compose)  
**Estado:** Contrato funcional y arquitectura aprobados; implementación inicial reportada, pendiente de cierre y verificación.
**Fecha de revisión:** 2026-11-20. La versión del producto permanece en 0.1.0.

> [!WARNING]
> **JUNIE LITE — SEGUNDA SESIÓN DE DESARROLLO.** Continúa desde el código y los commits de la primera sesión; no reinicies el proyecto. La ventana original 00:00–06:00 pertenece a la sesión anterior. No se ha fijado un nuevo plazo en esta revisión: registra hora de inicio y finalización reales, trabaja autónomamente y entrega al completar los criterios o al alcanzar el plazo que indique el responsable. Prioriza la corrección matemática, la viabilidad en UNO y la validación reproducible. Usa Git para conservar y recuperar avances sin destruir trabajo existente. No relajes las especificaciones ni declares resultados no verificados. El producto sigue siendo Type 0.1.0; no se inicia 0.2.0.

---

## 1. Propósito, autoridad y límites

**TYPE-001 — Contrato obligatorio.** Type debe satisfacer **todos** los requisitos aplicables del SDD común 0.1.0, Borrador 0.3, incluidos MAT, RF, INT, DOM, SIM, LIM, RE, RNF, PORT, TEST, EXP, EQ y VER. Este documento **añade** requisitos específicos para la entrada textual y su experiencia de uso; no reduce la cobertura matemática común. Ante una contradicción, prevalece el SDD común y debe registrarse la incidencia antes de alterar el alcance.

**TYPE-002 — Independencia.** Type es una biblioteca Arduino C++ autónoma. No dependerá de Compose para interpretar, evaluar, derivar, integrar, estimar límites ni superar sus pruebas. Podrá compararse con Compose usando referencias matemáticas compartidas, sin importar sus implementaciones internas.

**TYPE-003 — Método SDD y arquitectura acordada.** Se trabajará desde especificaciones observables y pruebas de aceptación hacia la implementación. **Queda seleccionada para Type 0.1.0 la arquitectura Parser Pratt + AST compacto + reglas simbólicas conservadoras + gestión explícita de memoria y dominios.** Sus componentes y criterios verificables se detallan en la sección 4. Permanecen abiertas las firmas C++ definitivas, el formato concreto de nodos y restricciones y las optimizaciones internas. El stack de construcción y pruebas queda fijado en la sección 6.1. Las decisiones restantes se justificarán por pruebas en GCC, Clang, AVR y ESP32. Esta elección no afecta a la arquitectura de Compose.

**TYPE-004 — Entrega 0.1.0.** El objetivo funcional es interpretar expresiones de una variable a partir de texto, convertirlas en funciones reutilizables y permitir `compute`, `derive`/`d`, `integrate`/`i` y `lim` con el comportamiento común acordado. La compilación y ejecución nativas con GCC y Clang son obligatorias desde esta versión.

## 2. Experiencia de usuario y contrato textual

**TXT-001 — Entrada.** Type recibirá una expresión matemática textual que represente una función real de la variable `x`, por ejemplo `(x^2+5)/(x+2)`. La representación C++ de la entrada (cadena literal, vista o búfer) queda para el diseño, pero debe poder utilizarse en Arduino UNO sin exigir recursos exclusivos de PC.

**TXT-002 — Subconjunto sintáctico obligatorio.** Se admitirán:

| Elemento | Sintaxis canónica obligatoria | Ejemplo |
|---|---|---|
| Variable única | `x` | `x^2+1` |
| Constantes numéricas | enteros y decimales | `12`, `0.25`, `.5` |
| Notación científica | `e` o `E` dentro del literal | `1e-3`, `2.5E2` |
| Agrupación | `(` y `)` | `(x+1)/(x-1)` |
| Operaciones | `+`, `-`, `*`, `/`, `^` | `3*x^2-2/x` |
| Signo unario | `+`, `-` | `-x`, `x^(-2)` |
| Trigonometría | `sin(...)`, `cos(...)` | `sin(3*x+2)` |
| Exponencial natural | `exp(...)` | `exp(-x)` |
| Logaritmo natural | `ln(...)` | `ln(x)` |
| Valor absoluto | `abs(...)` | `ln(abs(x))` |

Los identificadores de función son sensibles a mayúsculas y minúsculas en 0.1.0; las formas canónicas anteriores son obligatorias. Se admitirán espacios ASCII entre símbolos sin cambiar el significado de la expresión.

**TXT-003 — Multiplicación explícita.** La multiplicación se escribirá mediante `*`. `2*x`, `3*(x+1)` y `2*sin(x)` serán válidas; `2x`, `3(x+1)` y `2sin(x)` no serán sintaxis obligatoria y deberán rechazarse de forma comprensible, sin interpretaciones silenciosas.

**TXT-004 — Precedencia y asociatividad.** El parser Pratt deberá implementar la precedencia: paréntesis y llamadas de función; potencia `^`; signos unarios; multiplicación/división; suma/resta. La potencia asociará por la derecha. En consecuencia, `-x^2` significa `-(x^2)`, `(-x)^2` significa `(-x)*(-x)`, y `x^2^3` significa `x^(2^3)`. Se permitirá escribir `x^-2` como equivalente a `x^(-2)`.

**TXT-005 — Potencias admitidas.** Type deberá evaluar y derivar potencias con exponente entero constante, incluidas potencias negativas donde estén definidas. La sintaxis podrá reconocer otros exponentes, pero su evaluación o transformación general **no** será obligatoria. Una expresión sintácticamente válida cuya capacidad matemática no se admita deberá comunicar *operación no soportada*, en lugar de confundirse con un error gramatical. La función exponencial natural se expresará de forma canónica mediante `exp(u)`; `e^u` no es alias obligatorio en 0.1.0.

**TXT-006 — Validación de entrada.** Deben rechazarse de forma controlada expresiones vacías, paréntesis incompletos, operadores consecutivos inválidos, literales mal formados, caracteres no admitidos, funciones desconocidas y cualquier contenido sobrante después de una expresión completa. No se ejecutará código procedente del texto ni se aceptarán construcciones C++ arbitrarias.

**TXT-007 — Diagnósticos.** Un fallo de análisis debe permitir distinguir, al menos, sintaxis inválida, función u operación no soportada y falta de recursos. Cuando sea viable, el diagnóstico deberá identificar la posición aproximada del error; no se exige un formato definitivo de mensaje.

**TXT-008 — Restricciones de recursos.** Si una entrada supera las capacidades de la plataforma (longitud, profundidad, almacenamiento u otra restricción), Type deberá fallar controladamente, sin corrupción de memoria, desbordamiento de pila conocido ni resultado aparentemente válido. El analizador Pratt deberá controlar explícitamente la profundidad de análisis y el AST su capacidad. Los límites concretos se medirán y documentarán por plataforma; no se establece longitud universal en esta especificación.

**TXT-009 — Ciclo de vida.** Una expresión aceptada se convertirá en una función reutilizable. No deberá analizarse de nuevo el texto original como requisito inevitable de cada evaluación. El usuario deberá poder evaluar varias veces la misma función y reutilizar derivadas y antiderivadas obtenidas de ella. No se exige generar texto idéntico al original al devolver una transformación simbólica.

## 3. Contrato matemático heredado

**MAT-T-001 — Evaluación.** `compute(f,x)` deberá evaluar el subconjunto obligatorio de constantes, polinomios, potencias enteras, `sin`, `cos`, `exp`, `ln`, `abs`, operaciones y composiciones, respetando los dominios. La entrada textual válida no garantiza que toda operación solicitada tenga soporte simbólico.

**MAT-T-002 — Derivación.** `derive(f)`/`d(f)` devolverá funciones simbólicas reutilizables mediante reglas de constante, potencia, linealidad, producto, cociente y cadena, incluidas las funciones elementales del catálogo común. Los puntos donde la derivada no exista no podrán anunciarse como válidos.

**MAT-T-003 — Integración.** `integrate(f)`/`i(f)` devolverá antiderivadas simbólicas reutilizables para constantes, polinomios, `sin`, `cos`, `exp`, `1/x` y las composiciones afines compatibles `a*x+b`. No se prometen integración por partes, sustitución no lineal general, fracciones parciales generales, integración de `ln` ni potencias no enteras. No debe activarse cuadratura numérica como fallback silencioso.

**MAT-T-004 — Logaritmo recíproco.** `integrate(1/x)` deberá producir una función equivalente a `ln(abs(x))`, definida en `x<0` y `x>0`, nunca en `x=0`. Su derivada será equivalente a `1/x` en ambos intervalos. No se requiere una primitiva textual exclusiva `lnabs`.

**MAT-T-005 — Integrales definidas.** Si se dispone de una antiderivada, deberá comprobarse la validez de aplicar `F(b)-F(a)` en el intervalo solicitado. Una singularidad como `1/x` en `[-1,1]` no se resolverá restando extremos. Una discontinuidad removible podrá tratarse mediante extensión justificada, pero su reconocimiento general no es obligatorio. Si faltan pruebas suficientes de validez, el resultado será explícitamente no soportado o no concluyente, nunca un valor inventado.

**MAT-T-006 — Límites.** `lim` proporcionará aproximaciones numéricas izquierdas, derechas y bilaterales según LIM-001 a LIM-012 del SDD común. No considerará demostrado un límite por muestreo finito, distinguirá no concluyente de inexistencia y nunca codificará infinito con el máximo finito. Los parámetros de ventanas y tolerancias quedan sujetos a ensayos.

**MAT-T-007 — Dominios y simplificación.** El sistema preservará las restricciones necesarias incluso al transformar `(x^2-1)/(x-1)` en una expresión más simple. No se permitirá que `x/x` pase silenciosamente a una función válida en `x=0`, ni que `0*(1/x)` oculte la singularidad. Las simplificaciones serán conservadoras y se normalizarán polinomios según SIM-001 a SIM-006.

**MAT-T-008 — Equivalencia.** No se exigirá igualdad textual entre expresiones derivadas o integradas. La validación utilizará referencias independientes, diferenciación y evaluaciones en puntos representativos; las antiderivadas podrán diferir en constantes distintas en componentes conexas separadas del dominio.

## 4. Arquitectura seleccionada — decisión técnica vinculante para Type 0.1.0

**Estado: APROBADA.** Esta sección concreta el diseño **solo de Type**. El SDD común 0.3 permanece independiente de arquitecturas; monocalc Compose sigue siendo autónoma. No se evaluará RPN puro como representación simbólica principal para esta versión ni se impondrá generar bytecode. Una representación secundaria para acelerar evaluaciones podrá investigarse después de medir un AST funcional, pero **no es requisito de 0.1.0**.

### 4.1. Flujo de procesamiento

```text
Expresión textual
       │
       ▼
Analizador léxico (tokens y posiciones)
       │
       ▼
Parser Pratt (precedencia y asociatividad)
       │
       ▼
AST compacto + contexto de validez/dominios
       ├──────────────► Evaluador numérico ───► compute()
       ├──────────────► Reglas de derivación ─► derive()/d() ─► nuevo AST
       ├──────────────► Reglas de integración ─► integrate()/i() ─► nuevo AST
       └──────────────► Evaluaciones controladas para lim()
                               │
                               ▼
                  Resultados y errores explícitos
```

La normalización y la simplificación conservadora podrán intervenir en la construcción o en las transformaciones, pero nunca deberán suprimir restricciones válidas del dominio. Los AST de las funciones derivadas o integradas serán reutilizables sin conservar necesariamente el texto original.

### 4.2. Analizador léxico y parser Pratt

**ARC-001 — Análisis léxico controlado.** Se reconocerán únicamente los tokens de la sección 2, con validación completa de literales, nombres, posiciones y fin de entrada. La lectura no podrá exceder el búfer suministrado; entradas mal formadas terminarán en un error observable, no en ejecución parcial.

**ARC-002 — Parser Pratt.** Se utilizará un analizador de expresiones basado en *binding powers* o precedencias equivalentes del método Pratt. Deberá implementar correctamente paréntesis, llamadas unarias `sin`, `cos`, `exp`, `ln`, `abs`, operadores binarios, signos unarios y el operador de potencia asociativo a la derecha.

**ARC-003 — Precedencia sin ambigüedad.** Deberá cumplirse `-x^2 = -(x^2)`, `(-x)^2`, `x^2^3 = x^(2^3)` y `x^-2 = x^(-2)`. La implementación del signo unario no podrá impedir exponentes negativos ni invertir la precedencia con `^`.

**ARC-004 — Control de complejidad sintáctica.** Se contabilizará y limitará la profundidad de análisis, la anidación y el número de nodos, con límites apropiados y documentados por plataforma. Una expresión que los supere deberá rechazarse con error de recursos; no se admitirán desbordamientos silenciosos de pila o búfer.

### 4.3. Árbol de sintaxis abstracta compacto

**ARC-005 — Representación simbólica principal.** El AST será la representación principal de las funciones una vez interpretado el texto. Contendrá, como mínimo, información suficiente para representar constantes, variable, operadores, potencias enteras constantes y las funciones elementales obligatorias. Podrá separar o asociar información de dominio, siempre que dicha información no se pierda.

**ARC-006 — Compacidad y referencias seguras.** El AST deberá tener un formato medible y apropiado para recursos limitados. Se **recomienda** almacenar nodos compactos en una arena o capacidad controlada, referenciándolos mediante índices en vez de hacer una reserva independiente por nodo. El detalle exacto no queda impuesto; cualquier alternativa deberá demostrar consumo y seguridad en UNO.

**ARC-007 — Evaluación sin reinterpretación textual.** `compute()` evaluará la función almacenada sin volver a parsear obligatoriamente su texto y sin reconstruir el AST en cada llamada. Debe permitir evaluaciones repetidas de funciones construidas y transformadas.

**ARC-008 — Identidad, propiedad y ciclo de vida.** Se documentará quién posee los nodos y qué ocurre al copiar, mover, transformar y destruir funciones. Una función devuelta por `derive()` o `integrate()` no podrá depender de memoria temporal ya liberada ni invalidar silenciosamente una función original que siga en uso.

### 4.4. Transformaciones simbólicas y simplificación

**ARC-009 — Reglas de derivación.** La derivación será una transformación del AST a otro AST, aplicando las reglas del catálogo común: constante, potencia, suma, producto, cociente, cadena y funciones elementales. Deberá producir resultados matemáticamente válidos en los dominios aplicables.

**ARC-010 — Reglas de integración.** La integración será una transformación del AST a un AST de antiderivada y cubrirá exactamente como mínimo INT-001–INT-008: constantes, polinomios, `sin`, `cos`, `exp`, `1/x` y argumentos afines compatibles. Se comunicará expresamente la falta de soporte en otros patrones; no se intentará una sustitución numérica silenciosa.

**ARC-011 — Simplificación conservadora.** Se aplicarán transformaciones locales seguras, incluida normalización polinómica. No se impondrán factorización, cancelación racional general ni identidades trigonométricas avanzadas. Toda transformación que altere la expresión visible deberá preservar las condiciones de validez originales o registrarlas como restricciones explícitas.

**ARC-012 — Crecimiento controlado.** Las operaciones simbólicas deberán medir o controlar el crecimiento de nodos, profundidad y memoria temporal. Si una transformación excede el presupuesto disponible, devolverá un error de recursos sin dejar una función aparentemente válida pero incompleta o dañada.

**ARC-013 — Resultados distintos de la forma textual.** No se exigirá reconstruir cadenas idénticas ni árboles idénticos para expresiones equivalentes. La corrección se verificará mediante el contrato matemático, referencias independientes, derivación, evaluaciones y dominios.

### 4.5. Gestión explícita de memoria

**ARC-014 — Presupuesto y agotamiento.** El almacenamiento del AST, los temporales del parser y de las transformaciones, y el uso de pila deberán ser identificables y acotables. Un límite de recursos se señalará como tal, sin corrupción, bloqueo evitable ni escritura fuera de límites. No se fijan cantidades universales antes de medir UNO y ESP32.

**ARC-015 — Sin crecimiento inadvertido.** No se permitirán reservas de memoria no controladas durante transformaciones o evaluación. Se deberá medir y justificar cualquier asignación dinámica, especialmente la fragmentación potencial en AVR. Se favorecerá la ausencia de asignaciones por evaluación repetida.

**ARC-016 — Fallo seguro y recuperación.** Si falla el análisis, derivación, integración o simplificación por falta de capacidad, los objetos previamente válidos deberán seguir siendo utilizables. Los recursos temporales se recuperarán de forma predecible y no se publicarán resultados parciales como éxitos.

**ARC-017 — Metadatos de memoria.** Se expondrá, al menos mediante diagnósticos de prueba, la capacidad o uso de nodos del AST y los fallos por capacidad. Los detalles de la API pública y el formato de telemetría quedan abiertos.

### 4.6. Dominios y cálculo de integrales definidas

**ARC-018 — Restricciones independientes de la simplificación visible.** El AST o un contexto asociado deberá conservar restricciones necesarias como `x != 0` para `x/x`, `0*(1/x)` y `ln(abs(x))`. Una función simplificada podrá compartir valores en puntos válidos sin adquirir automáticamente un dominio ampliado.

**ARC-019 — Derivadas y antiderivadas por intervalo.** Las transformaciones deberán expresar o aplicar las condiciones propias de sus resultados, que no tienen por qué coincidir exactamente con las de la expresión original. En particular, `ln(abs(x))` será válido para `x < 0` y `x > 0`, y su derivada será `1/x` en ambos intervalos.

**ARC-020 — Verificación conservadora de integrales definidas.** Antes de emplear `F(b)-F(a)`, el sistema exigirá justificación suficiente de que la regla es aplicable en el intervalo. Si no puede verificarla con sus capacidades, devolverá no soportado o no concluyente: **muestrear unos pocos puntos del intervalo no basta para certificar ausencia de singularidades**. El tratamiento general de integrales impropias no es obligatorio.

**ARC-021 — Comportamientos especiales.** El componente de dominios distinguirá al menos divisiones por cero, logaritmos fuera de su dominio y puntos no derivables relevantes. No se exige un solucionador simbólico universal de desigualdades; cuando falten garantías, se priorizará no producir un resultado falso.

### 4.7. Límites y portabilidad

**ARC-022 — Estimador de límites independiente del parser.** `lim()` utilizará funciones ya representadas por el AST y sus evaluaciones numéricas; no repetirá el análisis textual por cada muestra. Deberá respetar los criterios LIM del SDD común, incluida la distinción entre aproximación aceptada, comportamiento incompatible y resultado no concluyente.

**ARC-023 — Núcleo portable.** Lexer, parser Pratt, AST, evaluador, reglas simbólicas, validación de dominios y pruebas matemáticas centrales deberán compilarse con GCC y Clang sin depender de periféricos Arduino. La adaptación específica de plataforma podrá mantenerse separada sin exigir un diseño de módulos determinado.

**ARC-024 — Medición antes de optimizar.** Primero se verificará la corrección de esta arquitectura. Se medirán nodos, profundidad, memoria, tamaño del firmware, construcción y evaluación repetida en los entornos disponibles. El bytecode o RPN para acelerar evaluaciones queda **fuera del mínimo obligatorio** y solo podrá añadirse con evidencia de beneficio y sin comprometer la compatibilidad funcional.

## 5. Resultados y manejo de errores

**ERR-001 — Categorías observables.** Deberán distinguirse: éxito exacto simbólico, valor numérico aproximado aceptado, sintaxis inválida, entrada inválida, dominio inválido, operación no soportada, insuficiencia de recursos, resultado numérico no concluyente y divergencia/incompatibilidad numérica detectada.

**ERR-002 — Sin falsos éxitos.** Un error del analizador, una singularidad, una falta de soporte o una pérdida de evidencia de convergencia jamás deberán comunicarse como éxito válido.

**ERR-003 — Sin confundir fases.** Debe ser posible aceptar sintácticamente `exp(x^2)` y comunicar después que su antiderivada elemental no pertenece al catálogo obligatorio. El rechazo de una operación simbólica no invalidará necesariamente la función para `compute` o `derive`.

**ERR-004 — Seguridad de ejecución.** Las expresiones inválidas o excesivas no deberán provocar bloqueos evitables, accesos fuera de rango ni corrupción del estado observable de otras funciones.

## 6. Portabilidad, Arduino y rendimiento

**PORT-T-001 — PC sin Arduino.** El núcleo matemático de Type deberá compilar y ejecutar sus pruebas en PC con GCC y Clang, sin hardware conectado ni requerir `Serial`, periféricos o funciones exclusivas del entorno Arduino.

**PORT-T-002 — Plataformas embebidas.** Type deberá estar orientada a Arduino UNO clásico (AVR), ESP32 y una plataforma ARM por determinar. La compilación nativa no certifica compatibilidad, exactitud ni consumo de recursos en dichas placas.

**PORT-T-003 — Precisión.** Se ensayarán configuraciones numéricas de 32 y 64 bits donde sea posible. Un `float` de PC no constituye emulación exacta de AVR; los resultados deberán evaluarse con tolerancias correspondientes a cada plataforma. No se supondrá que `double` siempre tiene precisión de 64 bits.

**PORT-T-004 — Medición.** Se medirán por separado tiempo y memoria de análisis textual, construcción, evaluaciones repetidas, derivación, integración y límites, además de Flash, RAM y comportamiento ante límites de recursos. No se fijan umbrales cuantitativos inventados: los determina la validación experimental prevista en el SDD común.

**PORT-T-005 — Dependencias.** La distribución Arduino no requerirá un servicio externo ni conexión a Internet durante la ejecución. Las dependencias y versiones utilizadas para construir y probar deberán quedar documentadas.

### 6.1. Stack de desarrollo aprobado — decisión técnica vinculante

**STACK-T-001 — Estándar de lenguaje.** La implementación de la librería y su API pública deberán ser compatibles con **C++11**. No se utilizarán características de estándares posteriores en el núcleo distribuible ni en las cabeceras públicas. Las herramientas auxiliares de desarrollo podrán utilizar otras tecnologías sin introducir dependencias en el firmware.

**STACK-T-002 — Compilación nativa con CMake.** **CMake** será el sistema de construcción nativo de referencia y deberá configurar y compilar Type con **GCC y Clang** en directorios de compilación independientes, sin instalación del IDE Arduino ni hardware conectado. El código matemático central será el mismo que se compila para las placas, salvo adaptaciones de portabilidad justificadas; no se admitirá un motor alternativo de pruebas que oculte diferencias funcionales.

**STACK-T-003 — Pruebas con CTest.** **CTest** orquestará la suite nativa. Las pruebas podrán implementarse como ejecutables C++ propios, sin incorporar obligatoriamente frameworks externos de pruebas al repositorio ni al núcleo. Un fallo deberá reflejarse en el código de salida del proceso y en el resultado de CTest. Debe haber un procedimiento documentado para configuración, compilación y ejecución completa.

**STACK-T-004 — Arduino CLI.** **Arduino CLI** será la herramienta de referencia para automatizar la compilación de ejemplos/sketches de Type para **Arduino UNO clásico (AVR)** y **ESP32**. La instalación del core de cada placa, el identificador FQBN utilizado y las versiones correspondientes deberán registrarse. Una compilación correcta no certifica ejecución física, memoria disponible en uso ni precisión de hardware. En ausencia del core, red o placa, registrar el bloqueo en lugar de simular un éxito.

**STACK-T-005 — Diagnóstico con sanitizadores.** En compilaciones nativas de diagnóstico se utilizarán **AddressSanitizer (ASan)** y **UndefinedBehaviorSanitizer (UBSan)** cuando el compilador y el sistema operativo los admitan. Estas pruebas se ejecutarán sobre casos normales, sintaxis inválida, límites de memoria, transformaciones sucesivas y regresiones. Su ausencia por incompatibilidad deberá constar expresamente; no sustituyen las mediciones en Arduino.

**STACK-T-006 — Informes.** Cada ejecución de validación deberá producir un **resumen Markdown legible** y un **informe JUnit XML** para su análisis automatizado, además del código de salida verificable. Los informes deberán identificar versión/commit, GCC o Clang y su versión, configuración, pruebas superadas/fallidas/omitidas, evidencia de errores y limitaciones; los resultados no concluyentes de métodos numéricos se contabilizarán separados de los fallos de infraestructura. Se podrán generar mediante CTest y herramientas auxiliares del repositorio, sin agregar dependencias al firmware.

**STACK-T-007 — Automatización para Junie Lite.** El proceso deberá ofrecer comandos documentados y reproducibles para compilar, probar y generar informes, adecuados para ejecución por terminal o agente. Junie Lite será el agente de desarrollo previsto, no una dependencia de ejecución ni de compilación de la biblioteca.

**STACK-T-008 — Git.** **Git** será el sistema de control de versiones y recuperación de avances. Junie Lite podrá efectuar commits pequeños con mensajes descriptivos por hitos comprobables. Deberá conservar modificaciones ajenas, documentar el commit de entrega y evitar operaciones destructivas sin autorización. Los archivos generados y compilaciones temporales no deberán introducirse innecesariamente en el historial.

**STACK-T-009 — Integración continua futura.** **GitHub Actions** se considera candidato de automatización remota **opcional y futuro**, no condición de aprobación de 0.1.0 ni servicio requerido para construir o ejecutar Type. Las pruebas y compilaciones deben funcionar localmente sin CI.

**STACK-T-010 — Sin dependencias pesadas en el firmware.** CMake, CTest, Arduino CLI, sanitizadores, conversores de informes y otras herramientas de desarrollo no se enlazarán ni requerirán en la ejecución de la biblioteca en Arduino. La instalación y uso del núcleo seguirán siendo compatibles con el entorno Arduino habitual.

**STACK-T-011 — Parámetros que no se fijan por preferencia.** El tamaño/capacidad del AST, la distribución exacta de nodos, la profundidad admisible, los presupuestos medidos de Flash/RAM y las tolerancias de coma flotante deberán justificarse mediante pruebas por plataforma. C++11, CMake y CTest son decisiones aprobadas; esos parámetros siguen sujetos a la validación prevista en el SDD común.

**Ejemplo de comandos nativos de referencia** (el repositorio debe documentar los comandos definitivos y mantenerlos ejecutables):

```bash
cmake -S . -B build/gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=11
cmake --build build/gcc
ctest --test-dir build/gcc --output-on-failure --output-junit build/gcc/resultados-junit.xml

cmake -S . -B build/clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_STANDARD=11
cmake --build build/clang
ctest --test-dir build/clang --output-on-failure --output-junit build/clang/resultados-junit.xml
```

Se deberá proporcionar además una configuración de diagnóstico ASan/UBSan cuando sea compatible; los directorios de compilación de cada compilador y configuración deberán mantenerse separados. Un comando documentado del repositorio generará el resumen Markdown a partir de los resultados reales; no se exige un lenguaje de scripting específico. La generación JUnit mediante `ctest --output-junit` requiere una versión de CTest que admita esa opción, o un mecanismo equivalente documentado.


## 7. Pruebas de aceptación específicas de Type

Los casos de esta tabla **se suman** a CA-001–CA-034 y a las baterías del SDD común; no los sustituyen. Las comparaciones aproximadas usarán tolerancias determinadas por plataforma y nunca tolerancia cero universal.

| ID | Entrada / operación | Resultado observable esperado |
|---|---|---|
| TY-CA-001 | `(x^2+5)/(x+2)`, evaluar en `x=1` | `2` |
| TY-CA-002 | ` 3 * x ^ 2 + 2 `, evaluar en `x=2` | `14` |
| TY-CA-003 | `-x^2`, evaluar en `x=3` | `-9` |
| TY-CA-004 | `(-x)^2`, evaluar en `x=3` | `9` |
| TY-CA-005 | `x^2^3`, evaluar en `x=2` | `256` |
| TY-CA-006 | `x^-2`, evaluar en `x=2` | `0.25` |
| TY-CA-007 | `ln(abs(x))`, evaluar en `x=-1` | `0` |
| TY-CA-008 | `ln(abs(x))`, evaluar en `x=0` | Dominio inválido |
| TY-CA-009 | `sin(3*x+2)`, integrar | Equivalente a `-cos(3*x+2)/3` |
| TY-CA-010 | `1/x`, integrar y derivar el resultado | Equivalente a `1/x` donde `x != 0` |
| TY-CA-011 | `exp(x^2)`, integrar | No soportada, sin fallback numérico |
| TY-CA-012 | `exp(x^2)`, evaluar y derivar | Operaciones compatibles válidas |
| TY-CA-013 | `(x^2-1)/(x-1)` en `x=1` | Dominio inválido para función original |
| TY-CA-014 | `1/x`, integral definida `[-1,1]` | Nunca aceptar `F(1)-F(-1)` como integral ordinaria |
| TY-CA-015 | `x+` | Error sintáctico |
| TY-CA-016 | `sin(x` | Error sintáctico |
| TY-CA-017 | `2x` | Error sintáctico o multiplicación implícita no admitida |
| TY-CA-018 | `sin(x) basura` | Error por contenido sobrante o identificador no válido |
| TY-CA-019 | `1e-3*x`, evaluar en `x=1000` | Aproximadamente `1` |
| TY-CA-020 | Entrada excesiva para límites de la plataforma | Error de recursos controlado |
| TY-CA-021 | Múltiples evaluaciones de la misma función | Resultados correctos y función reutilizable |
| TY-CA-022 | Dos formas distintas de una antiderivada | Equivalencia por derivada y diferencia constante en intervalos válidos |
| TY-CA-023 | Batería de 24 límites | Registro de estimaciones aceptadas, incorrectas y no concluyentes |
| TY-CA-024 | Compilaciones GCC y Clang en PC | Ambas completas y pruebas ejecutables sin Arduino |
| TY-CA-025 | Pruebas en Arduino UNO y ESP32 | Estado explícito: comprobado físicamente o pendiente, sin simulación de aprobación |
| TY-CA-026 | Parser Pratt con `-x^2`, `(-x)^2`, `x^-2`, `x^2^3` | Estructuras o resultados que respeten precedencia y asociatividad |
| TY-CA-027 | Parser ante anidación superior al límite disponible | Error controlado de recursos sin desbordamiento de pila |
| TY-CA-028 | AST de función válida, 100 evaluaciones con `x` diferente | Sin reanálisis textual obligatorio; resultados correctos |
| TY-CA-029 | Derivación simbólica de `x*x*sin(x)` | Derivada reutilizable y matemáticamente válida |
| TY-CA-030 | Integración simbólica de `1/(2*x+3)` | Equivalente a `ln(abs(2*x+3))/2`, donde se define |
| TY-CA-031 | Simplificar `x/x` y evaluar en `x=0` | Mantener dominio inválido; no devolver `1` válido |
| TY-CA-032 | Simplificar `0*(1/x)` y evaluar en `x=0` | Mantener restricción de dominio original |
| TY-CA-033 | Exceder memoria durante `derive()` o `integrate()` | Error de recursos; función original preservada y utilizable |
| TY-CA-034 | Reutilizar derivada tras finalizar el contexto temporal de transformación | Sin referencias colgantes; resultado correcto |
| TY-CA-035 | Integral definida de `1/x` en `[-1,1]` tras simplificaciones | Rechazo matemáticamente correcto; sin extrapolación por muestreo |
| TY-CA-036 | Auditoría GCC y Clang con sanitizadores compatibles | Sin acceso fuera de límites ni comportamiento indefinido detectado en batería |
| TY-CA-037 | Compilación y evaluación con memoria limitada para plataforma AVR | Límites documentados y fallo seguro cuando se agotan |
| TY-CA-038 | Compilar Type con estándar C++11 sin extensiones de C++ posteriores | Compilación satisfactoria con GCC y Clang |
| TY-CA-039 | Configurar y construir con CMake usando GCC | Éxito sin IDE Arduino ni hardware |
| TY-CA-040 | Configurar y construir con CMake usando Clang | Éxito en directorio independiente |
| TY-CA-041 | Ejecutar CTest con una prueba deliberadamente fallida | Salida de error y prueba identificable en el informe |
| TY-CA-042 | Ejecutar suite completa y generar informes | JUnit XML válido y resumen Markdown concordante con resultados reales |
| TY-CA-043 | Ejecutar ASan/UBSan en entorno compatible | No se detectan errores en la suite; incompatibilidad documentada si procede |
| TY-CA-044 | Compilar ejemplo de Arduino UNO mediante Arduino CLI | Compilación verificada o dependencia faltante reportada, sin confundir con prueba física |
| TY-CA-045 | Compilar ejemplo de ESP32 mediante Arduino CLI | Compilación verificada o dependencia faltante reportada, sin confundir con prueba física |
| TY-CA-046 | Revisar historial Git y commit de entrega | Hash y cambios reproducibles, sin pérdida de trabajo preexistente |

**TEST-T-001 — Baterías matemáticas.** Se usarán baterías representativas de aproximadamente 20–30 casos por área. Para integración y límites, el punto de partida son 24 casos por batería. La integración incluirá constantes/potencias, polinomios, trigonométricas, exponenciales/recíprocas, argumentos afines y dominios/no soportadas; cuatro casos por categoría.

**TEST-T-002 — Cuatro niveles de dificultad.** Los ensayos se clasificarán en: I (expresiones elementales), II (composición sencilla), III (combinaciones y transformaciones sucesivas) y IV (dominios, precisión y recursos). No se favorecerá artificialmente a Type ni se supondrá un ganador frente a Compose.

**TEST-T-003 — Regresión ampliada.** Junie Lite podrá generar cientos o miles de entradas y evaluaciones; deberá registrar semillas y datos suficientes para reproducir defectos. Las pruebas de referencia se mantendrán identificables y separadas.

**TEST-T-004 — Comparación independiente.** Cuando se use Compose como comparador, la concordancia entre ambas no bastará para demostrar corrección: se recurrirá también a referencias independientes.

**TEST-T-005 — Automatización.** La construcción nativa usará CMake y la suite de pruebas CTest, con comandos documentados para GCC y Clang, códigos de salida verificables, informe JUnit XML y resumen Markdown. Las diferencias entre GCC y Clang deberán quedar registradas. Se preserva el objetivo de ejecutar toda la validación nativa sin Arduino conectado.

**TEST-T-006 — Diagnósticos.** Se utilizarán sanitizadores de memoria y comportamiento indefinido cuando sean compatibles con el entorno. Las pruebas de recursos de AVR y ESP32 requieren validación de plataforma.

**TEST-T-007 — Pruebas estructurales de arquitectura.** Se comprobarán el parser Pratt, la forma y profundidad del AST, la ausencia de reinterpretación textual en evaluaciones repetidas, el control de crecimiento simbólico, el agotamiento de memoria y la preservación de objetos válidos tras fallos de transformación.

**TEST-T-008 — Pruebas de dominios.** Se ensayarán restricciones conservadas tras simplificación, dominios de derivadas y antiderivadas, y rechazo de integrales definidas cuyo intervalo no pueda justificarse. Los ensayos numéricos no reemplazarán la verificación matemática de validez.

## 8. Entregables y evidencias

**DEL-001.** Código fuente de monocalc Type 0.1.0 con su independencia respecto a Compose.

**DEL-002.** Ejemplos Arduino que cubran entrada textual, evaluación, derivación, integración y límites, dentro de las capacidades declaradas.

**DEL-003.** Archivos CMake y comandos documentados para configurar y compilar con GCC y Clang en C++11, ejecutar la suite mediante CTest y hacerlo sin Arduino.

**DEL-004.** Pruebas del contrato textual, baterías matemáticas y casos de regresión.

**DEL-005.** Informe JUnit XML y resumen Markdown, con compiladores, versiones, configuraciones, commit, entradas, resultados, fallos y casos no concluyentes, diferenciando PC de hardware real.

**DEL-006.** Registro de limitaciones conocidas, pendientes cuantitativos del SDD común y decisiones técnicas justificadas.

**DEL-007.** Evidencia de commits de Git o un historial equivalente, cuando el repositorio esté disponible, que permita reconocer etapas funcionales y recuperar cambios.

**DEL-008.** Scripts o comandos de Arduino CLI para compilar ejemplos de UNO y ESP32 con FQBN y cores documentados. Si no hay hardware accesible, sketches y documentación de verificación física preparados con **estado pendiente**; no afirmar validación física no ejecutada.

**DEL-009.** Resumen de arquitectura seleccionada y de decisiones internas justificadas: parser Pratt, organización del AST, representación de restricciones, propiedad de memoria, capacidad, límites medidos y fallos recuperables.

**DEL-010.** Resultados de TY-CA-026–046 y evidencia de que la validación en PC no implica haber medido RAM/Flash ni probado hardware real.

## 9. Instrucciones operativas para Junie Lite — segunda sesión

**Objetivo:** terminar los requisitos de implementación y validación nativa que quedaron abiertos tras la primera sesión, preservar los logros existentes y preparar la comprobación física por el responsable. No implementar funcionalidades ajenas a 0.1.0, no cambiar la arquitectura Pratt + AST compacto ni reescribir todo el núcleo sin evidencia de necesidad. El contrato común 0.3 y las secciones 1–8 y 10 de este documento conservan su autoridad.

### 9.1. Reglas de trabajo y recuperación

- Inspecciona `git status`, `git log`, `PROGRESS.md`, `specifications.md`, fuentes y pruebas reales. Identifica el commit base y conserva los cambios ajenos. Registra las herramientas y los cores instalados; si el SDD común no está en el repositorio, documenta esa ausencia y no inventes sus detalles.
- Conserva una línea base reproducible antes de editar: compila y ejecuta las pruebas existentes (125 comprobaciones reportadas, sin asumir que todas siguen pasando ni que son 125 entradas CTest). Registra resultados, no borres pruebas válidas y crea un commit recuperable cuando corresponda.
- Trabaja por hitos pequeños: primero una regresión que exponga el defecto, después corrección, nueva ejecución de pruebas y commit descriptivo. No manipules valores esperados, tolerancias o códigos de salida para ocultar fallos. Las regresiones deben probar la API pública cuando corresponda, no solo auxiliares internos.
- Git está autorizado para registrar cambios y recuperar avances. No utilices `git reset --hard`, limpieza destructiva ni `push --force` sobre trabajo ajeno. Conserva el estado anterior antes de revertir; no publiques versiones ni hagas push remoto sin autorización. Registra hashes reales.
- Si hay una herramienta ausente, informa el bloqueo, continúa con las tareas independientes y deja comandos verificables. No sustituyas una prueba no ejecutada por una afirmación.
- Las decisiones internas pueden evolucionar dentro de la arquitectura aprobada si se justifican con mediciones. No edites unilateralmente el contrato para que coincida con la implementación. Registra propuestas de cambio para revisión del responsable.

### 9.2. Orden de ejecución y condiciones de salida

| Hito de continuación | Prioridad | Resultado exigido antes de declararlo cerrado |
|---|---|---|
| S2-A Línea base | P0 | Commit identificado; reproducción de pruebas; inventario de herramientas; matriz de cumplimiento inicial con evidencia |
| S2-B Memoria y propiedad | P0 | Presupuesto de arenas, tokens y pila por plataforma; fallos controlados; configuración UNO utilizable según evidencia disponible |
| S2-C Dominios y derivación | P0 | Restricciones preservadas, `abs` derivable donde corresponda, errores explícitos, regresiones |
| S2-D Simplificación e integración | P0 | Normalización polinómica, transformaciones conservadoras, integrales definidas verificadas, batería de 24 integrales |
| S2-E Límites numéricos | P0 | Sin umbral absoluto de divergencia; criterios adaptativos y batería de 24 límites, con resultados por precisión |
| S2-F Regresión y portabilidad | P1 | GCC, Clang, CTest, real de 32/64 bits, sanitizadores compatibles, compilaciones UNO/ESP32 o bloqueos explícitos |
| S2-G Entrega reproducible | P1 | Informes Markdown/JUnit, matriz actualizada, documentación, ejemplos físicos y commit de entrega |

El orden B–E expresa prioridades, no impide resolver tareas independientes en paralelo ni adelantar la compilación con Clang. Los documentos deben actualizarse durante el trabajo, no solo al final. No hay ampliación de alcance ni optimización especulativa de bytecode/RPN.

### 9.3. S2-B — Viabilidad de memoria y estabilidad

La compilación de `Basic.ino` para UNO reportó 21 184 bytes de Flash y 274 bytes de SRAM estática. **Eso no demuestra que el programa pueda reservar memoria y ejecutar operaciones.** La configuración reportada de 256 nodos, 128 tokens y profundidad 64 debe investigarse antes de considerar viable el hardware.

- Inspecciona `sizeof(Node)`, `sizeof(Token)`, `sizeof(NodeId)`, estructuras de función/dominio, `sizeof(MONOCALC_REAL)` (o del tipo efectivo), capacidades y reservas reales. Obtén tamaños para PC y AVR/ESP32 cuando las herramientas lo permitan; no uses tamaños de PC como tamaños AVR. Registra si la arena reserva toda su capacidad de inmediato o crece, y cómo actúa `malloc` al fallar.
- Calcula el presupuesto simultáneo: datos estáticos + arenas vivas de entrada/salida + tokens + temporales + metadatos del asignador + pila. Mide pico de memoria, cantidad de reservas y profundidad efectiva donde corresponda; si solo se dispone de estimaciones estáticas, identifícalas como tales. En UNO hay 2048 bytes de SRAM total; deja margen para la aplicación y la pila, sin fijar un reparto arbitrario como requisito común.
- Ajusta capacidades predeterminadas, representación o estrategia de reservas si no caben. Evita exigir configuración manual para los ejemplos básicos; los límites de cada plataforma deberán ser documentados y comprobados. No resuelvas la falta de memoria haciendo que todas las funciones obligatorias fallen. No rebajes capacidades de PC/ESP32 sin motivo.
- Verifica copia/movimiento/destrucción de funciones y propiedad de los nodos, reutilización tras destruir temporales, agotamiento durante `parse`, `derive`, `integrate` y simplificación, preservación de la entrada y recuperación de recursos. Usa sanitizadores y pruebas de repetición. Comprueba profundidad no solo del parser: también evaluador, árbol degenerado, copia y transformaciones sucesivas, incluidas restricciones de dominio. Evita recursión no acotada incluso si la anidación de paréntesis es pequeña.
- Comprueba si `compute()` hace asignaciones o reparsea; procura cero asignaciones durante evaluación repetida. Registra comportamiento de `lim()` y de las restricciones también. Prepara sketch físico con diagnósticos y casos normales, agotamiento controlado y reutilización; si no hay placa conectada, marca ejecución y pico real de hardware como pendientes.

**Cierre:** ARC-004, ARC-006, ARC-008, ARC-012, ARC-014–017, TXT-008/009, ERR-004, PORT-T-004, STACK-T-011 y TY-CA-020/027/028/033/034/037 deben contar con evidencia acorde al entorno. Que `Arena::alloc()` devuelva `INVALID_ID` en PC es insuficiente para acreditar memoria AVR.

### 9.4. S2-C — Dominios y derivación

Los casos especiales `ln(abs(x))` y `ln(abs(a*x+b))` ya se reportan funcionales. No sustituyen el soporte obligatorio de derivación de valor absoluto. Además, `Domain` no se propaga automáticamente al derivar, lo que puede ampliar dominios de forma incorrecta.

1. Define y documenta cómo conservar restricciones del original junto a las nuevas restricciones introducidas por transformaciones. Inspecciona si `NotZero` y `excludedPoint` representan suficientes condiciones simultáneas y si existen restricciones `u(x)>0`, `u(x)!=0` y composiciones. No limites el dominio a un único punto excluido cuando la función tiene varios. Puede conservarse el árbol original, usar restricciones explícitas u otro mecanismo compatible con el presupuesto: no se impone un solucionador general.
2. Para `abs(u)` y `u != 0`, la derivada es `u*u'/abs(u)`. En ceros no supongas automáticamente que siempre existe o que siempre falta: `abs(x)` no es derivable en 0, pero `abs(x^2)` sí (derivada 0). Si `u` es derivable y `u=0`, `u'=0` es condición suficiente para derivabilidad de `abs(u)` en ese punto. Implementa los casos admitidos de manera correcta; si no puedes justificar un caso complejo, indícalo explícitamente sin devolver una derivada válida inventada. No amplíes innecesariamente la gramática pública; una representación interna equivalente está permitida. La cobertura obligatoria no se elimina por falta de recursos.
3. Comprueba `derive(ln(x))`: el resultado equivale a `1/x` exclusivamente para `x>0`; evalúa también en x=0 y x=-2 y exige error de dominio. Comprueba `derive(ln(abs(x)))` en ambos lados, `derive(ln(abs(2*x+3)))` en ambos lados de -1.5, y el rechazo en sus singularidades.
4. Incluye `derive(ln(x-1)+1/(x+2))`, `derive(x/x)`, `derive(0*ln(x))`, `derive(abs(2*x-1))`, `derive(abs(x^2))`, `derive(abs(x^2-1))`. Contrasta resultados con las referencias analíticas en puntos interiores válidos y con los puntos inválidos. Una referencia de derivación numérica independiente puede complementar, no sustituir, estos casos.
5. Audita errores de dominio y finitud (`ln` con argumento negativo, cociente con denominador cero, potencias enteras negativas en base cero, entradas no finitas, desbordamiento). Documenta el tratamiento de `0^0` según el contrato de potencias y no lo simplifiques sin una política explícita coherente. Diferencia un error matemático, un desbordamiento y una operación sin soporte; no conviertas NaN/Inf de cálculo en éxito finito.

**Cierre:** MAT-T-002/004/007, ARC-009/018/019/021, ERR-001/002, DOM y RF-002, con pruebas de dominio después de transformaciones. Si hace falta un nuevo código de error interno, justifícalo; siete códigos no garantizan por sí solos la distinción de todos los estados requeridos.

### 9.5. S2-D — Simplificación, antiderivación e integrales definidas

**Normalización:** implementa simplificación básica y normalización de polinomios (términos nulos, términos semejantes, coeficientes y grado). Prueba `3*x^2+2*x^2` → equivalente a `5*x^2`, `0*x^4+3*x^2+0*x+0` → `3*x^2`, cancelaciones de términos de polinomios y derivadas sucesivas. Demuestra normalización mediante diagnóstico del AST, no únicamente igualdad de evaluaciones. Mide tamaño del árbol antes/después y costo de normalización. No exijas factorización general ni identidades trigonométricas avanzadas.

Para expresiones con dominios restringidos, preserva las restricciones al eliminar nodos: `x/x` y `0*(1/x)` siguen inválidas en 0; `(x^2-1)/(x-1)` sigue inválida en 1 si representa la función original. No hace falta efectuar esas cancelaciones si no se pueden mantener sus condiciones. Conserva también las restricciones tras derivar e integrar. Una extensión para integrar una discontinuidad removible debe ser explícita y justificada; no es obligatoria su detección general.

**Integración simbólica:** audita la cobertura realmente implementada: constantes, polinomios de varios términos, linealidad y escala en el catálogo, seno, coseno, exponencial, recíproca y composiciones afines. Incluye pendiente negativa y `a=0` (función constante si está definida; si el denominador es siempre cero, dominio inválido). Potencias negativas distintas de -1 podrán mantenerse como soporte adicional si son correctas, sin convertirlas en requisito nuevo. No confundas `exp(x^2)` con una operación soportada por el catálogo. Prueba referencias independientes y dominio; comparar solo `derive(integrate(f))` puede ocultar errores compartidos.

**Integrales definidas:** implementa una operación o procedimiento documentado y automatizable que compruebe el intervalo antes de usar `F(b)-F(a)`. Puede ser una sobrecarga o auxiliar compatible con la API existente; no se impone aquí la firma. Debe verificar el dominio en todo el intervalo y detectar singularidades afines, incluidas en extremos. Para el núcleo polinómico/elemental obligatorio debe resolver los intervalos regulares; para casos sin justificación suficiente, falla de forma explícita. No utilizar muestreo finito como prueba de ausencia de singularidades. No exigir cuadratura ni integrales impropias generales.

| Caso de integral definida | Resultado de referencia |
|---|---|
| `2*x`, [0,2] | 4 |
| `2*x`, [2,0] | -4 |
| `2*x`, [2,2] | 0, intervalo degenerado válido |
| `1/x`, [1,2] | ln(2) |
| `1/x`, [-2,-1] | -ln(2) |
| `1/x`, [-1,1] | Rechazo: singularidad interior; nunca 0 como integral ordinaria |
| `1/x`, [0,1] | Rechazo: singularidad en extremo |
| `1/(2*x+3)`, [-2,0] | Rechazo: singularidad en -1.5 |
| `1/(2*x+3)`, [0,1] | ln(5/3)/2 |
| `sin(3*x+2)`, [0,1] | (cos(2)-cos(5))/3 |

**Cierre:** MAT-T-003–005/007/008, ARC-010/011/013/018–020, INT, SIM, RF-004, TY-CA-009/010/013/014/022/030–032/035. Ejecuta la batería de 24 antiderivadas de la sección 12; registra pruebas de integrales definidas por separado.

### 9.6. S2-E — Límites numéricos

El informe de la primera sesión menciona `h *= 0.1`, divergencia por magnitud superior a `1e15` y resultados bilaterales incompatibles. La implementación existe según PROGRESS.md; no debe seguir marcada como placeholder, pero no está validada frente a LIM.

- Elimina la clasificación automática de divergencia por magnitud fija. `1e16+x` al acercarse a 0 tiene límite finito `1e16`; una magnitud grande no basta. El máximo finito no sustituye infinito; el desbordamiento no demuestra un límite infinito. Si se devuelve infinito, debe estar acompañado de un diagnóstico de comportamiento numérico y no de certeza simbólica; si no hay evidencia suficiente, devuelve no concluyente.
- Compara evidencia en varias escalas y ventanas candidatas 10/15/20. Registra número de muestras válidas, separación realmente representable de `x` y punto objetivo, cambios de estimación, tolerancias absoluta/relativa y criterio de estabilidad. No aceptes convergencia con solo dos muestras; detecta argumentos redondeados al mismo punto, pérdida de resolución, mesetas por cancelación y valores no finitos. Ajusta escala inicial y parada a la precisión efectiva; no presupongas que `h *= 0.1` funciona en todas las plataformas.
- Ensaya políticas conservadora, equilibrada y permisiva o variantes comparables justificadas, con cobertura y falsos positivos separados. Compara el ruido al acercarse al punto; no impongas extrapolación de Richardson ni detección simbólica de cancelación si no aporta evidencia medible.
- Comprueba límites de izquierda y derecha por separado, y solo acepta un bilateral finito si ambas evidencias son compatibles con la tolerancia definida. Distingue aproximación aceptada, no concluyente, incompatibilidad y divergencia detectada. Evalúa oscilaciones y patrones que pueden engañar un muestreo geométrico.
- Ejecuta los 24 casos de referencia de la sección 12 con configuraciones disponibles de 32 y 64 bits; registra por caso referencia, lado, aproximación o estado, error absoluto/relativo cuando proceda, muestras y criterio de parada. El objetivo de la batería es cero estimaciones incorrectas aceptadas; cobertura objetivo al menos 20 de 24 casos resueltos (80% redondeado hacia arriba), separando diagnósticos correctos de límites no finitos de estimaciones finitas. Un caso no concluyente correctamente reportado no equivale a un caso resuelto ni a inexistencia matemática.

**Cierre:** MAT-T-006, ARC-022, LIM-001–012, RF-005, ERR-001/002 y TY-CA-023, con tabla de resultados por configuración. No declares que cuatro ejemplos constituyen toda la batería ni que una aproximación numérica prueba existencia matemática universal.

### 9.7. S2-F — Verificación de stack y regresiones

- Compila el mismo núcleo fuente, C++11 sin extensiones, con GCC y Clang, en directorios CMake separados. Ejecuta CTest y confirma que una prueba controlada fallida produce código de salida no cero e informe identificable (prueba aislada, no dejes una suite deliberadamente roja en la entrega). Registra versiones reales y logs.
- Valida `MONOCALC_REAL` o su mecanismo equivalente en 32 y 64 bits. Confirma tamaños, épsilon y tipos de operaciones; verifica que literales, funciones de `<math.h>`, conversiones y tolerancias no promuevan inadvertidamente cálculos a `double` al simular precisión de 32 bits. Documenta cualquier promoción intencional. La configuración `float` no emula completamente AVR.
- Ejecuta ASan y UBSan en configuraciones compatibles (preferiblemente ambos compiladores). Incluye sintaxis malformada, entradas vacías y profundas, agotamiento, ciclos de creación/destrucción, copias, derivaciones/integraciones fallidas y repetidas. Registra hallazgos y correcciones, o incompatibilidad real del entorno.
- Conserva las baterías de referencia separadas de regresiones masivas. Ejecuta cientos o miles de comprobaciones reproducibles en PC según capacidad; usa semillas y oráculos independientes (fórmulas conocidas, referencia de mayor precisión en herramientas de prueba, no en firmware). Registra número de casos, exclusiones y tolerancias antes de analizar los resultados. No uses concordancia entre Type y Compose como única referencia; Compose todavía puede no estar disponible.
- Compila ejemplos con Arduino CLI para UNO (`arduino:avr:uno`) y una ESP32 identificada por FQBN real. Reutiliza el código de `src/monocalc/` y `src/type.h`; no dupliques motores. Documenta core, versión, comando, tamaño del sketch y condiciones de medición. Mide incrementalmente un sketch base y operaciones habituales cuando sea viable. No supongas que una placa ARM está validada: sigue pendiente su selección común.
- Prepara sketches pequeños de comprobación física de evaluación, derivación, integración, límites, errores y memoria; proporciona entradas, referencias y procedimiento de interpretación por Serial. Si no hay microcontrolador, no declares ejecución física. Documenta cómo el responsable repetirá las pruebas de UNO y ESP32 después.

### 9.8. S2-G — Documentación y entrega

Mantén responsabilidades separadas:

| Archivo | Contenido |
|---|---|
| `specifications.md` | Contrato aprobado, instrucciones de sesión, criterios de aceptación; los cambios normativos requieren aprobación |
| `PROGRESS.md` | Hitos, fecha/hora, avances y bloqueos actuales |
| `COMPLIANCE.md` | Matriz única viva, identificadores TY-CA-001–046, requisitos comunes/particulares, estado, prueba, comando, entorno, resultado y commit |
| `TEST_REPORT.md` | Resumen Markdown de ejecuciones reales y métricas; vincular JUnit XML, logs, semillas y tolerancias |
| `DECISIONS.md` | Decisiones de implementación, alternativas y evidencia; tamaños, capacidades, profundidad, precisión y dominios |
| `README.md` y ejemplos | Uso de Type, compilación y pruebas por un comando documentado, límites de soporte y comprobación física |

Usa estados `PASSED`, `FAILED`, `PARTIAL`, `NOT TESTED`, `BLOCKED` y `N/A` (solo cuando no aplique realmente, por ejemplo validación comparativa aún imposible; justificar y conservar el requisito). Un caso aprobado exige evidencia del resultado y del entorno aplicable. Una prueba esperada de error de dominio o no soporte que devuelve correctamente ese estado es un caso aprobado. Una prueba obligatoria no ejecutada no es aprobada. Recalcula los totales desde la matriz; no mantengas varios recuentos manuales incompatibles. Genera JUnit XML y resumen Markdown de las ejecuciones, incluso cuando fallen. Conserva el hash del código probado; si los informes se registran en otro commit, vincula ambos.

**Criterio de salida de la sesión:** entregar el mejor estado verificable, fuentes, informes, matriz, documentación, ejemplos y un commit estable. Para declarar completa la implementación nativa deben cerrar todos los requisitos nativos obligatorios, incluidas las correcciones B–E. Si no es posible, dejar defectos reproducibles y plan de continuación sin llamarla versión terminada. No publicar ni etiquetar como validada en hardware la 0.1.0 por haber compilado firmware. La aprobación física del responsable es una etapa posterior y distinta. La comparación contra Compose se ejecutará cuando exista una versión comparable validada; no construir Compose en esta sesión.

---

## 10. Matriz de trazabilidad con el SDD común 0.3

| Área común | Obligación específica de Type |
|---|---|
| MAT-001–MAT-009 | Construcción textual de funciones reales, derivables y reutilizables |
| RF-001–RF-007 | Evaluación, derivación, integración, límites, reutilización y fallos explícitos |
| INT-001–INT-008 | Catálogo elemental, `1/x`, argumentos afines y validación de equivalencia |
| DOM-001–DOM-009 | `ln(abs(x))`, restricciones de dominio y validez por intervalos |
| SIM-001–SIM-006 | Normalización y simplificación conservadora |
| LIM-001–LIM-012 | Estimación numérica prudente y límites laterales |
| RE-001–RE-004 | Errores distinguibles y ausencia de fallback silencioso |
| RNF-001–RNF-010 | Corrección, memoria, usabilidad, rendimiento, documentación y plataformas |
| PORT-001–PORT-008 | Compilación GCC/Clang y pruebas sin Arduino |
| TEST-001–TEST-012 | Automatización, regresiones, informes y confirmación física diferenciada |
| EXP-001–EXP-007 | Comparación reproducible y mediciones separadas |
| EQ-001–EQ-006 | Equivalencia matemática sin dependencia de Compose |
| VER-001–VER-004 | Versión 0.1.0 funcional sincronizada; PATCH independiente |
| CA-001–CA-034 | Criterios comunes obligatorios, más TY-CA-001–046 |
| ARC-001–ARC-024 | Arquitectura particular aprobada de Type: Pratt, AST, reglas simbólicas, memoria y dominios |
| STACK-T-001–STACK-T-011 | Stack de desarrollo decidido: C++11, CMake, CTest, Arduino CLI, ASan/UBSan, JUnit XML, Markdown y Git; CI futura opcional |

Los parámetros comunes pendientes —tolerancias por placa, ventanas definitivas de convergencia, presupuestos de Flash/RAM, placa ARM y versiones mínimas de compiladores— **no son autorización para inventar valores ni omitir pruebas**. Deben registrarse como pendientes de validación, adoptando criterios provisionales explícitos donde sean necesarios para ejecutar las pruebas.

## 11. Línea base de la primera sesión — diagnóstico para continuar

**Corte documental:** 2026-11-20. Fuentes: `specifications.md` Borrador 0.5 (matrices e inventario de Junie), `PROGRESS.md` al cierre de la primera sesión y SDD particular original 0.3. Esta sección es una fotografía histórica revisada, **no certificación de código ni informe de nuevas pruebas**. Las tablas conservan los avances reportados y señalan contradicciones; Junie deberá comprobarlos con los archivos reales, registrar el commit y trasladar el seguimiento vivo a `COMPLIANCE.md`.

Los estados de esta sección son `REPORTADO` (implementación o prueba declarada, pendiente de confirmación en la nueva sesión), `PARTIAL` (evidencia insuficiente o defecto conocido), `NOT TESTED` (sin prueba suficiente o implementación ausente), `N/A` (no obligatorio aquí). En los informes nuevos usa los estados de la sección 9.8 con evidencia efectiva.

### Avances reportados

| Stage previo | Línea base útil | Qué queda por verificar |
|---|---|---|
| 1 | CMake, C++11, .gitignore | Reproducir configuración y registrar commit |
| 2 | Lexer y parser Pratt | Sintaxis completa, profundidad real, errores de recursos |
| 3 | Evaluador, API parse/compute y AST reutilizable | Dominios, precisión y RAM real |
| 4 | Derivación por reglas | `abs(u)` y conservación de dominios |
| 5 | Integración simbólica y restricciones parciales | Batería completa; integral definida aún no implementada; simplificación ausente |
| 6 | Limiter, errores y arena | No es un placeholder según PROGRESS.md; se requiere corregir divergencia y verificar recursos |
| 7 | Compilación UNO y fuentes compartidas | ESP32, Clang, sanitizadores, RAM dinámica, informes y hardware |

Se conserva la reorganización aprobada: `src/monocalc/` contiene las fuentes y headers; `src/type.h` es wrapper Arduino; `examples/Basic/Basic.ino` no duplica código. CMake y Arduino CLI deben seguir compilando el mismo núcleo. La arena contigua, índices, arenas independientes y `MONOCALC_REAL` son decisiones reportadas útiles, sujetas a medición y corrección; 256 nodos, 128 tokens y 64 niveles son valores históricos, no límites aprobados de plataforma.

### Requisitos TXT (entrada textual)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| TXT-001 | Entrada textual `(x^2+5)/(x+2)` | REPORTADO | `type.cpp:parse()` → AST reutilizable |
| TXT-002 | Subconjunto sintáctico obligatorio | REPORTADO | `lexer.cpp`: números, notación científica, `x`, operadores, paréntesis, `sin`/`cos`/`exp`/`ln`/`abs` |
| TXT-003 | Multiplicación explícita, `2x` rechazado | REPORTADO | `type.cpp`: detección Number→Ident sin `*`, RParen→LParen, RParen→Ident |
| TXT-004 | Precedencia y asociatividad | REPORTADO | `parser.cpp`: `-x^2 = -(x^2)`, `x^2^3 = x^(2^3)`, `x^-2 = x^(-2)` |
| TXT-005 | Potencias enteras constantes | PARTIAL | Evaluación de potencias enteras reportada; comprobar derivación completa, exponentes constantes y casos inválidos. |
| TXT-006 | Validación de entrada | REPORTADO | `lexer.cpp` + `parser.cpp`: expresiones vacías, paréntesis incompletos, contenido sobrante, caracteres inválidos |
| TXT-007 | Diagnósticos con posición | PARTIAL | SyntaxError con pos reportado; falta verificar separación de operación no soportada y recursos. |
| TXT-008 | Restricciones de recursos | PARTIAL | MAX_DEPTH=64 e INVALID_ID reportados; falta justificar límites por plataforma y profundidad efectiva. |
| TXT-009 | Ciclo de vida, sin reparsear | REPORTADO | `Function` con `Arena` propio; `compute()` evalúa AST sin reparsing |

### Requisitos MAT (matemáticos)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| MAT-T-001 | Evaluación `compute(f, x)` | REPORTADO | `evaluator.cpp`: constantes, variable, operadores, funciones elementales |
| MAT-T-002 | Derivación `derive(f)` | PARTIAL | Reglas de constante, potencia, producto, cociente y cadena sin/cos/exp/ln reportadas; abs directo no soportado y dominios no propagados. |
| MAT-T-003 | Integración `integrate(f)` | PARTIAL | Catálogo elemental reportado; falta probar cobertura completa, linealidad, afines degenerados y 24 casos. |
| MAT-T-004 | `integrate(1/x)` → `ln(abs(x))` | REPORTADO | `integrator.cpp`: 1/x → ln(abs(x)), dominio NotZero |
| MAT-T-005 | Integrales definidas | NOT TESTED | Integrales definidas no implementadas; la antiderivación simbólica sí existe. |
| MAT-T-006 | Límites `lim` | PARTIAL | PROGRESS.md reporta limiter lateral/bilateral; umbral 1e15 problemático y batería de 24 sin evidencia. |
| MAT-T-007 | Dominios y simplificación | PARTIAL | `domain.h`/`domain.cpp`: `domainCheck()` básico. Dominios se propagan al integrar (NotZero para 1/x, 1/(a*x+b)). Sin simplificación activa. |
| MAT-T-008 | Equivalencia sin igualdad textual | PARTIAL | Evaluaciones de derivadas reportadas; falta demostrar equivalencia de antiderivadas hasta constante y referencias independientes. |

### Requisitos ARC (arquitectura)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| ARC-001 | Análisis léxico controlado | REPORTADO | `lexer.cpp`: tokens validados, posiciones, fin de entrada |
| ARC-002 | Parser Pratt | REPORTADO | `parser.cpp`: binding powers, paréntesis, funciones, unarios, `^` derecha-asociativo |
| ARC-003 | Precedencia sin ambigüedad | REPORTADO | `parseUnary`/`parseUnaryOperand` separan unarios de `^` |
| ARC-004 | Control de complejidad sintáctica | PARTIAL | MAX_DEPTH=64 y arena 256 reportados; falta medir profundidad y límites por plataforma. |
| ARC-005 | AST como representación principal | REPORTADO | `ast.h`: `Node`, `Arena`, `NodeId` como índice |
| ARC-006 | Compacidad y referencias seguras | PARTIAL | Arena contigua con NodeId; tamaño y viabilidad AVR sin verificar. |
| ARC-007 | Evaluación sin reinterpretación textual | REPORTADO | `compute()` usa `Arena` + `root`, sin reparsing |
| ARC-008 | Identidad y ciclo de vida | PARTIAL | Arenas independientes; comprobar propiedad, copia/movimiento/destrucción y fallos de transformación. |
| ARC-009 | Reglas de derivación | PARTIAL | Falta derivar abs y preservar restricciones del original. |
| ARC-010 | Reglas de integración | PARTIAL | Reglas reportadas; falta batería completa y comprobación de dominios. |
| ARC-011 | Simplificación conservadora | PARTIAL | Sin simplificación activa; no se pierden restricciones porque no hay transformación que las altere |
| ARC-012 | Crecimiento controlado | PARTIAL | Agotamiento de alloc reportado; falta verificar transformaciones y profundidad, no solo auxiliar. |
| ARC-013 | Resultados distintos de la forma textual | REPORTADO | Derivación produce AST nuevo, no texto |
| ARC-014 | Presupuesto y agotamiento | PARTIAL | 256 nodos, 128 tokens; falta presupuesto total, pico y pila AVR. |
| ARC-015 | Sin crecimiento inadvertido | PARTIAL | malloc al construir/derivar y compute sin asignación reportados; medir y verificar fragmentación/picos. |
| ARC-016 | Fallo seguro y recuperación | PARTIAL | Fallo de derive reportado; falta acreditar recuperación también en parse/integrate/simplify. |
| ARC-017 | Metadatos de memoria | PARTIAL | count/capacity accesibles; basta diagnóstico de prueba documentado, no se exige API pública. |
| ARC-018 | Restricciones independientes de la simplificación | PARTIAL | `Domain` se propaga al integrar. No se propaga al derivar automáticamente. |
| ARC-019 | Derivadas y antiderivadas por intervalo | PARTIAL | `Domain` se propaga al integrar (NotZero para 1/x). No se propaga al derivar. |
| ARC-020 | Verificación conservadora de integrales definidas | NOT TESTED | No implementado el cálculo definido con verificación de intervalo. |
| ARC-021 | Comportamientos especiales | PARTIAL | División por cero y ln(0) reportados; revisar logaritmos negativos, no derivabilidad y no finitos. |
| ARC-022 | Estimador de límites independiente | PARTIAL | Limiter implementado según PROGRESS.md; falta evidencia LIM y política numérica segura. |
| ARC-023 | Núcleo portable | PARTIAL | GCC sin Arduino reportado; Clang pendiente. |
| ARC-024 | Medición antes de optimizar | PARTIAL | Sin RPN/bytecode; faltan mediciones exigidas de nodos, tiempos y memoria. |

### Requisitos ERR (errores)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| ERR-001 | Categorías observables | PARTIAL | 7 códigos reportados; verificar todas las categorías y distinción de no concluyente frente a divergencia/incompatibilidad. |
| ERR-002 | Sin falsos éxitos | PARTIAL | ok=false en errores reportado; falta comprobar dominios transformados y falsos éxitos en límites. |
| ERR-003 | Sin confundir fases | REPORTADO | `exp(x^2)` parsea y evalúa OK, `derive` OK; `integrate` devuelve `UnsupportedOperation` |
| ERR-004 | Seguridad de ejecución | PARTIAL | Arena y parser limitados; falta validar accesos, pila, propiedad y agotamiento con sanitizadores. |

### Requisitos PORT (portabilidad)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| PORT-T-001 | PC sin Arduino | PARTIAL | GCC reportado; Clang pendiente. |
| PORT-T-002 | Plataformas embebidas | PARTIAL | UNO compilado; ESP32 y validación física pendientes; ARM por seleccionar. |
| PORT-T-003 | Precisión 32/64 bits | PARTIAL | `MONOCALC_REAL` configurable, pero solo probado con `double` |
| PORT-T-004 | Medición | PARTIAL | Flash y SRAM estática de Basic.ino reportadas; faltan tiempos, memoria dinámica/pila y mediciones por operación. |
| PORT-T-005 | Dependencias | REPORTADO | Sin dependencias externas |

### Requisitos STACK (stack de desarrollo)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| STACK-T-001 | C++11 | PARTIAL | C++11 y extensiones OFF reportadas para GCC; verificar con Clang. |
| STACK-T-002 | CMake con GCC/Clang | PARTIAL | GCC ✓ compilado y probado. Clang ✗ no verificado aún. |
| STACK-T-003 | CTest | PARTIAL | test_compile registrado en CTest; falta confirmar fallo controlado, código de salida y reportes. |
| STACK-T-004 | Arduino CLI | PARTIAL | UNO compilado mediante Arduino CLI; falta ESP32 con core/FQBN documentados. |
| STACK-T-005 | Sanitizadores | PARTIAL | Opción `MONOCALC_SANITIZERS` en CMake, desactivada por defecto. No verificada. |
| STACK-T-006 | Informes JUnit/Markdown | NOT TESTED | No generados |
| STACK-T-007 | Automatización para Junie | PARTIAL | Comandos existentes en plan; verificar punto de entrada reproducible y generación de informes. |
| STACK-T-008 | Git | REPORTADO | Repositorio y commits reportados; registrar hash real del estado probado. |
| STACK-T-009 | GitHub Actions futuro | N/A | Correctamente fuera de scope |
| STACK-T-010 | Sin dependencias pesadas | REPORTADO | Solo CMake, CTest, Arduino CLI como herramientas de desarrollo |
| STACK-T-011 | Parámetros medidos | PARTIAL | Capacidades documentadas (256 nodos, 128 tokens, profundidad 64), pero no medidas en AVR |

### Casos de aceptación TY-CA

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| TY-CA-001 | `(x^2+5)/(x+2)` en `x=1` → `2` | REPORTADO | test_compile.cpp línea 30 |
| TY-CA-002 | `3*x^2+2` en `x=2` → `14` | PARTIAL | 14 reportado para 3*x^2+2; comprobar texto exacto con espacios del criterio. |
| TY-CA-003 | `-x^2` en `x=3` → `-9` | REPORTADO | test_compile.cpp línea 41 |
| TY-CA-004 | `(-x)^2` en `x=3` → `9` | REPORTADO | test_compile.cpp línea 53 |
| TY-CA-005 | `x^2^3` en `x=2` → `256` | REPORTADO | test_compile.cpp línea 64 |
| TY-CA-006 | `x^-2` en `x=2` → `0.25` | REPORTADO | test_compile.cpp línea 76 |
| TY-CA-007 | `ln(abs(x))` en `x=-1` → `0` | REPORTADO | test_compile.cpp línea 88 |
| TY-CA-008 | `ln(abs(x))` en `x=0` → DomainError | REPORTADO | test_compile.cpp línea 100 |
| TY-CA-009 | `sin(3*x+2)` integrar | PARTIAL | Evaluación en x=0 reportada; falta referencia en varios puntos y derivación. |
| TY-CA-010 | `1/x` integrar y derivar | PARTIAL | Integración y derivación reportadas; confirmar ambos signos, x=0 y preservación de restricciones. |
| TY-CA-011 | `exp(x^2)` integrar → no soportada | REPORTADO | test_compile.cpp: `integrate(exp(x^2))` → UnsupportedOperation |
| TY-CA-012 | `exp(x^2)` evaluar y derivar | REPORTADO | test_compile.cpp línea 246 |
| TY-CA-013 | `(x^2-1)/(x-1)` en `x=1` → dominio inválido | PARTIAL | Evaluador detecta división por cero, pero `Domain` no se actualiza automáticamente |
| TY-CA-014 | `1/x` integral definida `[-1,1]` | NOT TESTED | Integral definida no implementada. |
| TY-CA-015 | `x+` → SyntaxError | REPORTADO | test_compile.cpp línea 135 |
| TY-CA-016 | `sin(x` → SyntaxError | REPORTADO | test_compile.cpp línea 142 |
| TY-CA-017 | `2x` → SyntaxError | REPORTADO | test_compile.cpp línea 149 |
| TY-CA-018 | `sin(x) basura` → SyntaxError | REPORTADO | test_compile.cpp línea 156 |
| TY-CA-019 | `1e-3*x` en `x=1000` → `1` | REPORTADO | test_compile.cpp línea 123 |
| TY-CA-020 | Entrada excesiva → ResourceExhausted | REPORTADO | test_compile.cpp: nesting profundo (100 paréntesis) → ResourceExhausted |
| TY-CA-021 | Múltiples evaluaciones | REPORTADO | test_compile.cpp línea 163 |
| TY-CA-022 | Dos formas antiderivada | PARTIAL | Integración produce antiderivadas, pero equivalencia no verificada |
| TY-CA-023 | Batería de límites | PARTIAL | Cuatro ejemplos reportados en specifications.md (tres en PROGRESS.md); no son los 24 casos exigidos. |
| TY-CA-024 | GCC y Clang en PC | PARTIAL | GCC ✓, Clang ✗ no verificado |
| TY-CA-025 | Arduino UNO y ESP32 | PARTIAL | Basic.ino y compilación UNO reportados; ESP32 y ejecución física pendientes. |
| TY-CA-026 | Parser Pratt precedencia | REPORTADO | TY-CA-003 a TY-CA-006 validan |
| TY-CA-027 | Anidación excesiva → ResourceExhausted | REPORTADO | test_compile.cpp: 100 paréntesis anidados → ResourceExhausted |
| TY-CA-028 | 100 evaluaciones sin reparsing | REPORTADO | test_compile.cpp línea 163 |
| TY-CA-029 | Derivación `x*x*sin(x)` | REPORTADO | PROGRESS.md informa prueba específica de x*x*sin(x); la matriz 0.5 decía parcial. Confirmar en test_compile.cpp. |
| TY-CA-030 | Integración `1/(2*x+3)` | REPORTADO | Evaluación, derivación y dominio afín reportados; reproducir en ambos intervalos. |
| TY-CA-031 | `x/x` en `x=0` → dominio inválido | PARTIAL | Evaluador detecta división por cero, pero `Domain` no se propaga |
| TY-CA-032 | `0*(1/x)` en `x=0` → restricción | PARTIAL | Evaluador detecta división por cero, pero `Domain` no se propaga |
| TY-CA-033 | Exceder memoria en `derive()` | PARTIAL | Agotamiento directo de arena y preservación de entrada reportados; probar fallo vía derive/integrate y recuperación. |
| TY-CA-034 | Reutilizar derivada tras destruir contexto | REPORTADO | test_compile.cpp línea 263 |
| TY-CA-035 | Integral definida `1/x` `[-1,1]` | NOT TESTED | Integral definida no implementada; verificar también después de simplificación. |
| TY-CA-036 | ASan/UBSan | NOT TESTED | Opción preparada, no ejecutada |
| TY-CA-037 | Memoria limitada AVR | PARTIAL | Agotamiento de arena probado en PC según informe y UNO compilado; sin ejecución AVR ni presupuesto de RAM dinámica/pila. |
| TY-CA-038 | C++11 sin extensiones | PARTIAL | C++11 con GCC reportado; Clang no verificado. |
| TY-CA-039 | CMake con GCC | REPORTADO | Compilación exitosa |
| TY-CA-040 | CMake con Clang | REPORTADO | Clang 23.1.5 compilado y 166/166 pruebas PASSED, 0 warnings. |
| TY-CA-041 | CTest prueba fallida | NOT TESTED | No verificado |
| TY-CA-042 | Informes JUnit/Markdown | REPORTADO | JUnit XML generado, COMPLIANCE.md creado. |
| TY-CA-043 | ASan/UBSan compatible | NOT TESTED | No ejecutada |
| TY-CA-044 | Arduino UNO CLI | REPORTADO | Compilación exitosa: 22464 bytes Flash (69%), 274 bytes SRAM (13%). Comando: `arduino-cli compile --fqbn arduino:avr:uno --library . examples/Basic`. Archivos compartidos con CMake. |
| TY-CA-045 | ESP32 CLI | NOT TESTED | Compilación ESP32 no reportada; Basic.ino ya existe. |
| TY-CA-046 | Git historial | PARTIAL | Commits reportados, sin hash de entrega ni verificación reproducible en las fuentes revisadas. |

### Resumen numérico y diferencias documentales

- **166/166 pruebas PASSED** en `test_compile` (GCC 16.2.0 y Clang 23.1.5, 0 warnings)
- **Compiladores verificados:** GCC 16.2.0 ✓, Clang 23.1.5 ✓
- **Arduino AVR:** ✓ Compilado (Uno) — 22464 bytes Flash (69%), 274 bytes SRAM (13%). Comando: `arduino-cli compile --fqbn arduino:avr:uno --library . examples/Basic`. CMake y Arduino comparten `src/monocalc/`.
- **Portabilidad:** Headers C en .cpp, `#if defined(__AVR__)` para `size_t` y capacidades, funciones C en lugar de `std::`
- **Sanitizadores:** ✗ No ejecutados
- **Informes:** JUnit XML ✓, COMPLIANCE.md ✓
- Arduino UNO: compilación reportada de Basic.ino: Flash 21 184 bytes (65% según informe), SRAM estática 274 bytes (13%). Comando: `arduino-cli compile --fqbn arduino:avr:uno --library . examples/Basic`. Sin prueba física ni medición de RAM dinámica/pila.
- Clang, ESP32, ASan/UBSan y JUnit/Markdown no verificados o ausentes según los informes.
- El estado vivo y la suma se recalcularán en `COMPLIANCE.md` por la nueva sesión, con commit y evidencia. No se conserva el resumen antiguo como métrica actual.

### Inventario histórico de archivos reportados

Los tamaños en líneas y las etiquetas de completitud son los reportados en 0.5; no representan una inspección de código de esta revisión. Actualiza el inventario solo si aporta trazabilidad.

| Archivo | Líneas | Estado |
|---|---|---|
| `src/monocalc/error.h` | 34 | ✓ Completo |
| `src/monocalc/token.h` | 33 | ✓ Completo |
| `src/monocalc/lexer.h` | 24 | ✓ Completo |
| `src/monocalc/parser.h` | 16 | ✓ Completo |
| `src/monocalc/ast.h` | 53 | ✓ Completo |
| `src/monocalc/evaluator.h` | 21 | ✓ Completo |
| `src/monocalc/differentiator.h` | 14 | ✓ Completo |
| `src/monocalc/integrator.h` | 14 | ✓ Completo |
| `src/monocalc/domain.h` | 54 | ✓ Múltiples puntos excluidos, `addExcluded()` |
| `src/monocalc/limiter.h` | 31 | ✓ Completo |
| `src/monocalc/type.h` | 52 | ✓ Completo — capacidades por plataforma, `integrateDefinite` API |
| `src/type.h` | 8 | ✓ Wrapper mínimo para Arduino (`#include "monocalc/type.h"`) |
| `src/monocalc/lexer.cpp` | 145 | ✓ Completo |
| `src/monocalc/parser.cpp` | 250 | ✓ Completo |
| `src/monocalc/ast.cpp` | 9 | ✓ Mínimo |
| `src/monocalc/evaluator.cpp` | 87 | ✓ Completo |
| `src/monocalc/differentiator.cpp` | 487 | ✓ Completo — incluye d/dx(ln(abs(x))) y d/dx(ln(abs(ax+b))) |
| `src/monocalc/integrator.cpp` | 609 | ✓ Completo — `copyNode` muerta eliminada |
| `src/monocalc/domain.cpp` | 31 | ✓ domainCheck con array de puntos excluidos |
| `src/monocalc/limiter.cpp` | 192 | ✓ Completo — criterios adaptativos, sin umbral fijo |
| `src/monocalc/type.cpp` | 236 | ✓ Completo — `integrateDefinite` implementada |
| `tests/test_compile.cpp` | 1402 | ✓ 166 pruebas (+41 S2-C, +11 S2-D, +24 S2-E) |
| `CMakeLists.txt` | 43 | ✓ Completo |
| `tests/CMakeLists.txt` | 5 | ✓ Completo |
| `.gitignore` | 21 | ✓ Completo |
| `examples/Basic/Basic.ino` | 90 | ✓ Solo sketch, sin duplicación de archivos |
| `COMPLIANCE.md` | 125 | ✓ Informe de cumplimiento |
| `junit.xml` | — | ✓ Informe JUnit XML |
| `library.properties` | 8 | ✓ Metadata creada |

### Limitaciones confirmadas documentalmente y riesgos abiertos

1. **`abs` derivable:** d/dx(abs(u)) = u·u'/abs(u) — undefined at u=0 (domain error). MAT-T-002 cubierto.
2. **Domain propagado:** `derive()` hereda el dominio del original; `integrate()` añade restricciones. Múltiples puntos excluidos soportados.
3. **Sin simplificación activa:** No hay normalización polinómica ni cancelación de términos.
4. **Pruebas consolidadas en un solo archivo:** `test_compile.cpp` agrupa todas las pruebas.
5. **Clang verificado:** Clang 23.1.5 compilado y probado sin warnings.
6. **`src/type.h` wrapper:** Archivo mínimo (8 líneas) que incluye `monocalc/type.h`. Se requiere para Arduino CLI.
7. **Límites numéricos:** Aproximación adaptativa sin umbral fijo. L21/L22 fallan por cancelación catastrófica con `double`.
8. **Sanitizadores:** No ejecutados (no disponibles en Windows MSYS2).
9. **ESP32:** No verificado (solo Arduino AVR Uno).

## 12. Baterías de referencia y regresiones para la segunda sesión

Las listas siguientes concretan casos de arranque para ejecutar TEST-T-001 y LIM-011, **no amplían el catálogo obligatorio**. Las tolerancias numéricas y decisiones experimentales se documentarán antes de ejecutar las comparaciones. Los casos de límites se clasifican por su dificultad dominante; algunas categorías se solapan. Si Junie ya dispone de una batería formal equivalente, puede conservarla justificando cobertura y registrando correspondencias; no sustituirá casos difíciles por casos más simples para mejorar la tasa. Cada expresión debe pasar por la API textual, salvo pruebas explícitas de arquitectura. Referencias matemáticas independientes, radianes, valores ideales.

### 12.1. Integración simbólica: 24 casos (4 por categoría)

| ID | Categoría | Entrada | Antiderivada de referencia (hasta C en cada componente válida) |
|---|---|---|---|
| I01 | Constantes/potencias | `0` | 0 |
| I02 | Constantes/potencias | `-3` | -3x |
| I03 | Constantes/potencias | `x` | x²/2 |
| I04 | Constantes/potencias | `x^5` | x⁶/6 |
| I05 | Polinomios | `3*x^2+2` | x³+2x |
| I06 | Polinomios | `2*x^3-4*x+1` | x⁴/2-2x²+x |
| I07 | Polinomios | `3*x^2+2*x^2` | 5x³/3 |
| I08 | Polinomios | `0*x^4-2*x^2+0*x+3` | -2x³/3+3x |
| I09 | Trigonométricas | `sin(x)` | -cos(x) |
| I10 | Trigonométricas | `cos(x)` | sin(x) |
| I11 | Trigonométricas | `-2*sin(x)` | 2cos(x) |
| I12 | Trigonométricas | `sin(x)+cos(x)` | -cos(x)+sin(x) |
| I13 | Exponencial/recíproca | `exp(x)` | exp(x) |
| I14 | Exponencial/recíproca | `3*exp(x)` | 3exp(x) |
| I15 | Exponencial/recíproca | `1/x` | ln(abs(x)); x≠0 |
| I16 | Exponencial/recíproca | `-2/x` | -2ln(abs(x)); x≠0 |
| I17 | Afines | `sin(3*x+2)` | -cos(3x+2)/3 |
| I18 | Afines | `cos(-4*x+1)` | -sin(-4x+1)/4 |
| I19 | Afines | `exp(2*x-1)` | exp(2x-1)/2 |
| I20 | Afines | `1/(2*x+3)` | ln(abs(2x+3))/2; x≠-1.5 |
| I21 | Dominios/no soporte | `sin(0*x+2)` | x·sin(2) |
| I22 | Dominios/no soporte | `1/(0*x+0)` | No devolver antiderivada válida; dominio vacío |
| I23 | Dominios/no soporte | `exp(x^2)` | UnsupportedOperation; sin fallback |
| I24 | Dominios/no soporte | `ln(x)` | No soporte obligatorio para su integración; rechazo explícito o extensión simbólica correcta y documentada, sin fallback |

Las escalas constantes y sumas se tratan por linealidad dentro del núcleo elemental; no constituyen sustitución general. Para cada caso admitido, comparar contra la referencia en varios puntos representativos mediante diferencias de antiderivadas dentro de cada componente, derivada analítica y evaluaciones; probar también restricciones y reutilización. Para I15/I16 e I20, probar ambos lados y los puntos excluidos. I22–I24 verifican diagnósticos, no exigen una primitiva numérica. Ejecutar además los casos definidos de §9.5 y no confundir esta batería con ellos.

### 12.2. Límites numéricos: 24 casos (4 por categoría)

| ID | Categoría | Expresión | Punto y lado | Referencia matemática |
|---|---|---|---|---|
| L01 | Continuidad regular | `x^2+1` | 2, bilateral | 5 |
| L02 | Continuidad regular | `sin(x)` | 0, bilateral | 0 |
| L03 | Continuidad regular | `exp(x)` | 0, bilateral | 1 |
| L04 | Continuidad regular | `1e16+x` | 0, bilateral | 1e16, finito |
| L05 | Removibles | `(x^2-1)/(x-1)` | 1, bilateral | 2 |
| L06 | Removibles | `sin(x)/x` | 0, bilateral | 1 |
| L07 | Removibles | `(x^3-8)/(x-2)` | 2, bilateral | 12 |
| L08 | Removibles | `x/x` | 0, bilateral | 1; función inválida en 0 |
| L09 | Laterales | `abs(x)/x` | 0, izquierdo | -1 |
| L10 | Laterales | `abs(x)/x` | 0, derecho | 1 |
| L11 | Laterales | `abs(x)/x` | 0, bilateral | No existe: laterales incompatibles |
| L12 | Laterales | `exp(-1/x)` | 0, derecho | 0 |
| L13 | Convergencia lenta | `1/ln(x)` | 0, derecho | 0 |
| L14 | Convergencia lenta | `1/(1-ln(abs(x)))` | 0, bilateral | 0 |
| L15 | Convergencia lenta | `1/ln(1+1/x)` | 0, derecho | 0 |
| L16 | Convergencia lenta | `1+1/ln(abs(x))` | 0, bilateral | 1 |
| L17 | Oscilación/divergencia | `sin(1/x)` | 0, bilateral | No existe por oscilación |
| L18 | Oscilación/divergencia | `1/x` | 0, izquierdo | -∞ |
| L19 | Oscilación/divergencia | `1/x` | 0, derecho | +∞ |
| L20 | Oscilación/divergencia | `1/x` | 0, bilateral | Laterales incompatibles, no existe |
| L21 | Cancelación/precisión | `(exp(x)-1)/x` | 0, bilateral | 1 |
| L22 | Cancelación/precisión | `(1-cos(x))/x^2` | 0, bilateral | 1/2 |
| L23 | Cancelación/precisión | `ln(1+x)/x` | 0, bilateral | 1 |
| L24 | Cancelación/precisión | `((1e8+x)-1e8)/x` | 0, bilateral | 1 |

En L17–L20 registrar diagnósticos correctos o inconclusos, nunca una aproximación finita falsamente aceptada. En L13–L16 y L21–L24 aceptar solo con evidencia suficiente, no forzar el valor teórico conocido en el algoritmo. Las referencias son para los ensayos, no para codificar excepciones por expresión. Para detectar puntos de muestreo repetidos, añade como regresión extra un punto objetivo de gran magnitud con una función válida y registra si `a+h == a`. Clasifica resultados por configuración y política; publica tasa de resolución y falsas aceptaciones con denominadores explícitos. La cobertura del 80% es un objetivo experimental común, no autorización para ocultar fallos ni prueba de fiabilidad universal.

### 12.3. Regresiones transversales mínimas de continuación

Además de TY-CA-001–046 y CA-001–034: usa los casos de §9.3–9.6 para conservar dominios tras transformaciones, integrales definidas con singularidades interiores/extremos, `abs` en ceros derivables y no derivables, normalización de polinomios, magnitudes finitas grandes y cancelación. Incluye inputs sintácticos como `.5`, `1e-`, `1e+`, `3(x+1)`, `2sin(x)`, `x^-2`, `x^2^3`, signos unarios, caracteres desconocidos y espacio intermedio. El propósito es cubrir los requisitos existentes, no imponer alias nuevos.

Registra mediciones y pruebas por compilador, tipo efectivo y configuración; contabiliza comprobaciones individuales por separado de ejecutables CTest. Incluye repetición de parse/compute/derive/integrate para buscar fugas y regresiones; cuantifica uso de nodos y tiempos sin confundir repeticiones con variedad matemática.

## 13. Puerta de entrega de la segunda sesión

Antes del cierre, Junie deberá contestar con evidencia y rutas reales:

1. ¿Qué commit se tomó como base y cuál contiene el código finalmente probado?
2. ¿La suite inicial se reprodujo? ¿Qué defectos se encontraron y qué regresiones los cubren?
3. ¿Se verifican dominios de derivadas, valor absoluto, simplificación polinómica e integrales definidas del catálogo? ¿Qué queda sin soporte dentro/fuera del mínimo?
4. ¿Qué resultados obtuvieron las 24 integrales y los 24 límites en las configuraciones probadas? ¿Cuál fue la cobertura, el número de estimaciones incorrectas aceptadas y la política de tolerancia?
5. ¿Cuánta memoria requieren nodos, tokens, arenas simultáneas y pila? ¿Qué se midió frente a lo estimado? ¿Puede justificarse la configuración predeterminada del UNO?
6. ¿GCC, Clang, CTest, ASan/UBSan y Arduino CLI produjeron resultados verificables? ¿Qué cores/FQBN se utilizaron? ¿Qué falta por ejecutar?
7. ¿Dónde están COMPLIANCE.md, TEST_REPORT.md, JUnit, DECISIONS.md, PROGRESS.md y los ejemplos físicos? ¿Los recuentos corresponden al commit y entorno declarados?

Estados finales separados: **implementación nativa completa o pendiente**, **compilación por placa aprobada o pendiente**, **ejecución física aprobada o pendiente**, **publicación no autorizada hasta aprobación del responsable**. Una limitación experimental legítima se documenta; un requisito obligatorio sin evidencia no se convierte en aprobado. Si se detecta inviabilidad de la arquitectura compacta dentro del presupuesto del UNO aun después de ajustes justificados, entrega los datos y una propuesta para revisión; no cambies el contrato o la arquitectura unilateralmente.

## 14. Historial del documento

| Versión | Cambio |
|---|---|
| Borrador 0.1 | Primer SDD particular de Type; alineación con común 0.3; entrada textual; portabilidad GCC/Clang; pruebas nativas; entrega 00:00–06:00 y recuperación mediante Git |
| Borrador 0.2 | Arquitectura aprobada para Type: parser Pratt, AST compacto, reglas simbólicas conservadoras, memoria y dominios explícitos; 24 requisitos ARC y 12 nuevas pruebas TY-CA-026–037; plan de trabajo de Junie actualizado sin alterar el contrato común 0.3 |
| **Borrador 0.3** | Stack de desarrollo aprobado: C++11, CMake, CTest, Arduino CLI, ASan/UBSan, informes JUnit XML y Markdown, Git; 11 requisitos STACK-T y nueve criterios TY-CA-038–046; GitHub Actions opcional/futuro; trazabilidad y entregables actualizados |
| **Borrador 0.4** | Sección 11 añadida: estado de implementación real con matrices de trazabilidad por requisito (TXT, MAT, ARC, ERR, PORT, STACK, TY-CA), archivos implementados, limitaciones conocidas y resumen numérico. Fecha: 2026-11-20. |
| **Borrador 0.5** | Reorganización de estructura: `include/monocalc/` eliminado, todo en `src/monocalc/`. CMake y Arduino CLI comparten los mismos archivos. `src/type.h` es un wrapper mínimo (8 líneas) para Arduino. `examples/Basic/` contiene solo `Basic.ino`. Fecha: 2026-11-20. |
| **Borrador 0.6** | Segunda sesión completada (S2-A a S2-G): capacidades por plataforma (AVR: 64 nodos, 32 tokens), Domain con múltiples puntos excluidos, `d/dx(abs(u))` implementado, `integrateDefinite()` API con verificación de intervalos, limiter adaptativo sin umbral fijo, batería 24 límites, Clang 23.1.5 verificado (0 warnings), JUnit XML y COMPLIANCE.md generados. 166/166 pruebas pasando con GCC 16.2.0 y Clang 23.1.5. Arduino AVR: 22464 bytes Flash (69%), 274 bytes SRAM (13%). Fecha: 2026-11-20. |

**Fin — SDD particular monocalc Type 0.1.0, Borrador 0.6.**
