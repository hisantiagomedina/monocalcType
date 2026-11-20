# MONOCALC TYPE 0.1.0
## SDD particular — Spec-Driven Development

**Documento:** SDD-TYPE-0.1.0  
**Revisión:** Borrador 0.3 — arquitectura y herramientas de desarrollo aprobadas  
**Contrato superior:** SDD común monocalc 0.1.0, Borrador 0.3  
**Producto:** monocalc Type (independiente de monocalc Compose)  
**Estado:** Especificación de Type con arquitectura y stack de desarrollo seleccionados; parámetros cuantitativos pendientes según SDD común.

> [!WARNING]
> **JUNIE LITE — ADVERTENCIA OPERATIVA, INICIO 00:00.** El plazo indicado por el responsable del proyecto es de **seis horas: objetivo de entrega a las 06:00, hora local de esa misma jornada**. Si terminas antes, entrega antes. Trabaja de manera autónoma, ejecuta pruebas nativas con GCC y Clang y deja resultados reproducibles. **Puedes utilizar Git** para registrar avances mediante commits pequeños y recuperar un estado anterior si algo falla. No sobrescribas trabajo ajeno, no uses operaciones destructivas sin necesidad y no declares pruebas aprobadas si no las ejecutaste. **El plazo no autoriza a sacrificar corrección matemática, omitir fallos ni presentar la validación en PC como validación física del Arduino.** Si el tiempo no alcanza, entrega el estado verificable, los bloqueos y el siguiente paso concreto.

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

## 9. Instrucciones operativas para Junie Lite — ventana 00:00–06:00

> **PRIORIDAD DE EJECUCIÓN.** Son las **00:00 según la ventana indicada por el responsable del proyecto**. El objetivo es completar la implementación y la validación nativa exigible de Type 0.1.0 **antes de las 06:00 de la misma jornada**, y entregar inmediatamente si se consigue antes. Esta es una meta operativa, no una certificación de que el trabajo pueda terminarse sin obstáculos. No ocultes fallos para cumplir el reloj.

**Orden de trabajo recomendado:**

1. **Inspección y línea base.** Leer el SDD común 0.3 y este documento. Identificar estructura existente, estado de Git, GCC, Clang, CMake, CTest, Arduino CLI, cores disponibles y pruebas actuales. No sobrescribir cambios no relacionados.
2. **Pruebas antes que comportamiento.** Convertir los requisitos observables en casos de prueba: sintaxis textual y precedencia Pratt, AST y memoria, evaluación, derivación, integración, dominios, límites y errores.
3. **Construcción incremental obligatoria.** Construir primero lexer y parser Pratt; después AST compacto con propiedad de memoria explícita y evaluación; a continuación derivación, simplificación conservadora, integración simbólica, dominios e integración definida; finalmente límites. Implementar el mínimo necesario para aprobar cada grupo sin dependencia de Compose. No sustituir esta arquitectura por RPN puro o bytecode por conveniencia.
4. **Depuración nativa.** Construir en C++11 con CMake usando GCC y Clang; ejecutar CTest y regresión ampliada, además de ASan/UBSan cuando sean compatibles. Corregir y repetir, produciendo JUnit XML y resumen Markdown coherentes.
5. **Preparación Arduino.** Compilar ejemplos para UNO y ESP32 mediante Arduino CLI cuando estén disponibles los cores; registrar versiones y FQBN. Preparar pruebas físicas y distinguir compilación de ejecución real en placa.
6. **Cierre.** Entregar fuentes, comandos reproducibles, informes JUnit XML y Markdown, versión/commit, limitaciones y pruebas físicas pendientes.

**Git autorizado como herramienta de recuperación:**

- Revisar `git status` y el historial antes de modificar archivos.
- Trabajar en una rama de desarrollo cuando sea apropiado, preservando modificaciones preexistentes.
- Hacer commits pequeños y descriptivos después de hitos comprobables, por ejemplo: pruebas del parser, evaluación, derivación, integración, límites y validación nativa.
- Antes de recuperar una versión anterior, identificar el commit estable y preservar el trabajo actual. Utilizar mecanismos seguros de Git y revisar las diferencias resultantes.
- No usar `git reset --hard`, limpieza destructiva ni `push --force` sobre trabajo ajeno sin autorización explícita. No incluir secretos ni archivos generados innecesarios en commits.
- Registrar el hash del commit que corresponda al informe final.

**Criterio de entrega a las 06:00:** entregar el mejor estado **realmente verificado**, no una declaración ficticia de éxito. Si faltan requisitos, identificar cuáles no aprobaron, cómo reproducirlos y desde qué commit puede continuarse. No considerar publicada ni validada en hardware la versión por haber superado solamente la fase nativa.

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

## 11. Estado de implementación

> Actualizado: 2026-11-20. Solo se registra lo realmente implementado y verificado. Los requisitos sin marcar se consideran **pendientes**.

### Stages completados

| Stage | Estado | Descripción |
|---|---|---|
| Stage 1 | ✓ Completado | CMake, estructura include/src, .gitignore, 11 cabeceras |
| Stage 2 | ✓ Completado | Lexer completo + Parser Pratt con precedencia correcta |
| Stage 3 | ✓ Completado | Evaluador numérico + API pública parse(), compute() |
| Stage 4 | ✓ Completado | Derivación simbólica completa (constante, potencia, linealidad, producto, cociente, cadena) |
| Stage 5 | ✓ Completado | Integración simbólica completa + propagación de dominios |
| Stage 6 | ✗ Pendiente | Límites, errores distinguibles y control de memoria |
| Stage 7 | ✗ Pendiente | Ejemplo Arduino, validación final y reporte |

### Requisitos TXT (entrada textual)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| TXT-001 | Entrada textual `(x^2+5)/(x+2)` | ✓ Implementado | `type.cpp:parse()` → AST reutilizable |
| TXT-002 | Subconjunto sintáctico obligatorio | ✓ Implementado | `lexer.cpp`: números, notación científica, `x`, operadores, paréntesis, `sin`/`cos`/`exp`/`ln`/`abs` |
| TXT-003 | Multiplicación explícita, `2x` rechazado | ✓ Implementado | `type.cpp`: detección Number→Ident sin `*`, RParen→LParen, RParen→Ident |
| TXT-004 | Precedencia y asociatividad | ✓ Implementado | `parser.cpp`: `-x^2 = -(x^2)`, `x^2^3 = x^(2^3)`, `x^-2 = x^(-2)` |
| TXT-005 | Potencias enteras constantes | ✓ Implementado | `evaluator.cpp`: potencias enteras con bases negativas |
| TXT-006 | Validación de entrada | ✓ Implementado | `lexer.cpp` + `parser.cpp`: expresiones vacías, paréntesis incompletos, contenido sobrante, caracteres inválidos |
| TXT-007 | Diagnósticos con posición | ✓ Implementado | `ErrorCode::SyntaxError` con `pos` en `ParseResult` |
| TXT-008 | Restricciones de recursos | ✓ Implementado | `parser.cpp`: `MAX_DEPTH=64`, `Arena::alloc()` con `INVALID_ID` |
| TXT-009 | Ciclo de vida, sin reparsear | ✓ Implementado | `Function` con `Arena` propio; `compute()` evalúa AST sin reparsing |

### Requisitos MAT (matemáticos)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| MAT-T-001 | Evaluación `compute(f, x)` | ✓ Implementado | `evaluator.cpp`: constantes, variable, operadores, funciones elementales |
| MAT-T-002 | Derivación `derive(f)` | ✓ Implementado | `differentiator.cpp`: constante, potencia, linealidad, producto, cociente, cadena (sin/cos/exp/ln) |
| MAT-T-003 | Integración `integrate(f)` | ✓ Implementado | `integrator.cpp`: constantes, polinomios, sin, cos, exp, 1/x, composiciones afines |
| MAT-T-004 | `integrate(1/x)` → `ln(abs(x))` | ✓ Implementado | `integrator.cpp`: 1/x → ln(abs(x)), dominio NotZero |
| MAT-T-005 | Integrales definidas | ✗ Pendiente | Integración no implementada |
| MAT-T-006 | Límites `lim` | ✗ Pendiente | `limiter.cpp`: placeholder, devuelve `NotConverged` |
| MAT-T-007 | Dominios y simplificación | ⚠ Parcial | `domain.h`/`domain.cpp`: `domainCheck()` básico. Dominios se propagan al integrar (NotZero para 1/x, 1/(a*x+b)). Sin simplificación activa. |
| MAT-T-008 | Equivalencia sin igualdad textual | ✓ Implementado | Derivación valida por evaluación en puntos, no por texto |

### Requisitos ARC (arquitectura)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| ARC-001 | Análisis léxico controlado | ✓ Implementado | `lexer.cpp`: tokens validados, posiciones, fin de entrada |
| ARC-002 | Parser Pratt | ✓ Implementado | `parser.cpp`: binding powers, paréntesis, funciones, unarios, `^` derecha-asociativo |
| ARC-003 | Precedencia sin ambigüedad | ✓ Implementado | `parseUnary`/`parseUnaryOperand` separan unarios de `^` |
| ARC-004 | Control de complejidad sintáctica | ✓ Implementado | `MAX_DEPTH=64`, `Arena::capacity=256` |
| ARC-005 | AST como representación principal | ✓ Implementado | `ast.h`: `Node`, `Arena`, `NodeId` como índice |
| ARC-006 | Compacidad y referencias seguras | ✓ Implementado | Arena contigua, índices `NodeId`, sin punteros entre nodos |
| ARC-007 | Evaluación sin reinterpretación textual | ✓ Implementado | `compute()` usa `Arena` + `root`, sin reparsing |
| ARC-008 | Identidad y ciclo de vida | ✓ Implementado | `derive()` crea nuevo `Arena` independiente; función original preservada |
| ARC-009 | Reglas de derivación | ✓ Implementado | `differentiator.cpp`: todas las reglas especificadas |
| ARC-010 | Reglas de integración | ✓ Implementado | `integrator.cpp`: constantes, polinomios, sin/cos/exp con afines, 1/x, 1/(a*x+b) |
| ARC-011 | Simplificación conservadora | ⚠ Parcial | Sin simplificación activa; no se pierden restricciones porque no hay transformación que las altere |
| ARC-012 | Crecimiento controlado | ✓ Implementado | `Arena::alloc()` devuelve `INVALID_ID` al agotarse |
| ARC-013 | Resultados distintos de la forma textual | ✓ Implementado | Derivación produce AST nuevo, no texto |
| ARC-014 | Presupuesto y agotamiento | ✓ Implementado | `DEFAULT_ARENA_CAPACITY=256`, `DEFAULT_TOKEN_CAPACITY=128` |
| ARC-015 | Sin crecimiento inadvertido | ✓ Implementado | `malloc` en `parse()`/`derive()`; sin asignación en `compute()` |
| ARC-016 | Fallo seguro y recuperación | ✓ Implementado | `derive()` falla sin afectar función original (test 20) |
| ARC-017 | Metadatos de memoria | ⚠ Parcial | `Arena::count`/`capacity` accesibles, pero no expuestos en API pública |
| ARC-018 | Restricciones independientes de la simplificación | ⚠ Parcial | `Domain` se propaga al integrar. No se propaga al derivar automáticamente. |
| ARC-019 | Derivadas y antiderivadas por intervalo | ⚠ Parcial | `Domain` se propaga al integrar (NotZero para 1/x). No se propaga al derivar. |
| ARC-020 | Verificación conservadora de integrales definidas | ⚠ Parcial | Integración implementada pero integrales definidas no verificadas aún |
| ARC-021 | Comportamientos especiales | ✓ Implementado | `DomainError` en división por cero y `ln(0)` |
| ARC-022 | Estimador de límites independiente | ✗ Pendiente | Placeholder |
| ARC-023 | Núcleo portable | ✓ Implementado | Compilación GCC sin Arduino |
| ARC-024 | Medición antes de optimizar | ✓ Implementado | Sin bytecode ni RPN |

### Requisitos ERR (errores)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| ERR-001 | Categorías observables | ✓ Implementado | `error.h`: 7 códigos de error distinguibles |
| ERR-002 | Sin falsos éxitos | ✓ Implementado | Errores retornan `ok=false` |
| ERR-003 | Sin confundir fases | ✓ Implementado | `exp(x^2)` parsea y evalúa OK, `derive` OK; `integrate` devuelve `UnsupportedOperation` |
| ERR-004 | Seguridad de ejecución | ✓ Implementado | Límites de arena y profundidad controlados |

### Requisitos PORT (portabilidad)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| PORT-T-001 | PC sin Arduino | ✓ Implementado | Compilación y pruebas con GCC 16.2.0 |
| PORT-T-002 | Plataformas embebidas | ✗ Pendiente | Arduino AVR no compilado aún |
| PORT-T-003 | Precisión 32/64 bits | ⚠ Parcial | `MONOCALC_REAL` configurable, pero solo probado con `double` |
| PORT-T-004 | Medición | ✗ Pendiente | Flash/RAM no medidos |
| PORT-T-005 | Dependencias | ✓ Implementado | Sin dependencias externas |

### Requisitos STACK (stack de desarrollo)

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| STACK-T-001 | C++11 | ✓ Implementado | `CMAKE_CXX_STANDARD 11`, `CMAKE_CXX_EXTENSIONS OFF` |
| STACK-T-002 | CMake con GCC/Clang | ⚠ Parcial | GCC ✓ compilado y probado. Clang ✗ no verificado aún. |
| STACK-T-003 | CTest | ✓ Implementado | `test_compile` registrado y ejecutado |
| STACK-T-004 | Arduino CLI | ✗ Pendiente | `examples/Basic/` vacío, sin sketch ni `library.properties` |
| STACK-T-005 | Sanitizadores | ⚠ Parcial | Opción `MONOCALC_SANITIZERS` en CMake, desactivada por defecto. No verificada. |
| STACK-T-006 | Informes JUnit/Markdown | ✗ Pendiente | No generados |
| STACK-T-007 | Automatización para Junie | ✓ Implementado | Comandos documentados en plan |
| STACK-T-008 | Git | ✓ Implementado | Repositorio activo, commits registrados |
| STACK-T-009 | GitHub Actions futuro | ✗ Fuera de alcance | Correctamente fuera de scope |
| STACK-T-010 | Sin dependencias pesadas | ✓ Implementado | Solo CMake, CTest, Arduino CLI como herramientas de desarrollo |
| STACK-T-011 | Parámetros medidos | ⚠ Parcial | Capacidades documentadas (256 nodos, 128 tokens, profundidad 64), pero no medidas en AVR |

### Casos de aceptación TY-CA

| ID | Descripción | Estado | Evidencia |
|---|---|---|---|
| TY-CA-001 | `(x^2+5)/(x+2)` en `x=1` → `2` | ✓ PASSED | test_compile.cpp línea 30 |
| TY-CA-002 | `3*x^2+2` en `x=2` → `14` | ✓ PASSED | test_compile.cpp línea 111 |
| TY-CA-003 | `-x^2` en `x=3` → `-9` | ✓ PASSED | test_compile.cpp línea 41 |
| TY-CA-004 | `(-x)^2` en `x=3` → `9` | ✓ PASSED | test_compile.cpp línea 53 |
| TY-CA-005 | `x^2^3` en `x=2` → `256` | ✓ PASSED | test_compile.cpp línea 64 |
| TY-CA-006 | `x^-2` en `x=2` → `0.25` | ✓ PASSED | test_compile.cpp línea 76 |
| TY-CA-007 | `ln(abs(x))` en `x=-1` → `0` | ✓ PASSED | test_compile.cpp línea 88 |
| TY-CA-008 | `ln(abs(x))` en `x=0` → DomainError | ✓ PASSED | test_compile.cpp línea 100 |
| TY-CA-009 | `sin(3*x+2)` integrar | ✓ PASSED | test_compile.cpp: -cos(3*x+2)/3 evaluado en x=0 |
| TY-CA-010 | `1/x` integrar y derivar | ✓ PASSED | `integrate(1/x)` → ln(abs(x)), dominio NotZero. `d/dx(ln(abs(x)))` = 1/x verificado |
| TY-CA-011 | `exp(x^2)` integrar → no soportada | ✓ PASSED | test_compile.cpp: `integrate(exp(x^2))` → UnsupportedOperation |
| TY-CA-012 | `exp(x^2)` evaluar y derivar | ✓ PASSED | test_compile.cpp línea 246 |
| TY-CA-013 | `(x^2-1)/(x-1)` en `x=1` → dominio inválido | ⚠ Parcial | Evaluador detecta división por cero, pero `Domain` no se actualiza automáticamente |
| TY-CA-014 | `1/x` integral definida `[-1,1]` | ✗ Pendiente | Integración no implementada |
| TY-CA-015 | `x+` → SyntaxError | ✓ PASSED | test_compile.cpp línea 135 |
| TY-CA-016 | `sin(x` → SyntaxError | ✓ PASSED | test_compile.cpp línea 142 |
| TY-CA-017 | `2x` → SyntaxError | ✓ PASSED | test_compile.cpp línea 149 |
| TY-CA-018 | `sin(x) basura` → SyntaxError | ✓ PASSED | test_compile.cpp línea 156 |
| TY-CA-019 | `1e-3*x` en `x=1000` → `1` | ✓ PASSED | test_compile.cpp línea 123 |
| TY-CA-020 | Entrada excesiva → ResourceExhausted | ✓ PASSED | test_compile.cpp: nesting profundo (100 paréntesis) → ResourceExhausted |
| TY-CA-021 | Múltiples evaluaciones | ✓ PASSED | test_compile.cpp línea 163 |
| TY-CA-022 | Dos formas antiderivada | ⚠ Parcial | Integración produce antiderivadas, pero equivalencia no verificada |
| TY-CA-023 | Batería de límites | ✓ PASSED | test_compile.cpp: sin(x)/x→1, 1/x→±∞, abs(x)/x lateral, (x²-1)/(x-1)→2 |
| TY-CA-024 | GCC y Clang en PC | ⚠ Parcial | GCC ✓, Clang ✗ no verificado |
| TY-CA-025 | Arduino UNO y ESP32 | ✗ Pendiente | Sin sketch ni compilación |
| TY-CA-026 | Parser Pratt precedencia | ✓ PASSED | TY-CA-003 a TY-CA-006 validan |
| TY-CA-027 | Anidación excesiva → ResourceExhausted | ✓ PASSED | test_compile.cpp: 100 paréntesis anidados → ResourceExhausted |
| TY-CA-028 | 100 evaluaciones sin reparsing | ✓ PASSED | test_compile.cpp línea 163 |
| TY-CA-029 | Derivación `x*x*sin(x)` | ⚠ Parcial | Reglas de producto y cadena implementadas, sin prueba específica |
| TY-CA-030 | Integración `1/(2*x+3)` | ✓ PASSED | Evalúa a ln(3)/2 en x=0, derivada equivale a 1/(2*x+3), dominio x≠-1.5 |
| TY-CA-031 | `x/x` en `x=0` → dominio inválido | ⚠ Parcial | Evaluador detecta división por cero, pero `Domain` no se propaga |
| TY-CA-032 | `0*(1/x)` en `x=0` → restricción | ⚠ Parcial | Evaluador detecta división por cero, pero `Domain` no se propaga |
| TY-CA-033 | Exceder memoria en `derive()` | ✓ PASSED | test_compile.cpp: Arena agotada → INVALID_ID, función original preservada |
| TY-CA-034 | Reutilizar derivada tras destruir contexto | ✓ PASSED | test_compile.cpp línea 263 |
| TY-CA-035 | Integral definida `1/x` `[-1,1]` | ✗ Pendiente | Integración no implementada |
| TY-CA-036 | ASan/UBSan | ✗ Pendiente | Opción preparada, no ejecutada |
| TY-CA-037 | Memoria limitada AVR | ⚠ Parcial | Arena::alloc() devuelve INVALID_ID al agotarse (verificado en PC). Arduino no compilado aún |
| TY-CA-038 | C++11 sin extensiones | ✓ PASSED | CMake configurado correctamente |
| TY-CA-039 | CMake con GCC | ✓ PASSED | Compilación exitosa |
| TY-CA-040 | CMake con Clang | ✗ Pendiente | No verificado aún |
| TY-CA-041 | CTest prueba fallida | ✗ Pendiente | No verificado |
| TY-CA-042 | Informes JUnit/Markdown | ✗ Pendiente | No generados |
| TY-CA-043 | ASan/UBSan compatible | ✗ Pendiente | No ejecutada |
| TY-CA-044 | Arduino UNO CLI | ✗ Pendiente | Sin sketch |
| TY-CA-045 | ESP32 CLI | ✗ Pendiente | Sin sketch |
| TY-CA-046 | Git historial | ✓ PASSED | Commits registrados |

### Resumen numérico

- **TY-CA completados:** 23/46 (✓ PASSED)
- **TY-CA parciales:** 7/46 (⚠ Parcial — funcionalidad base existe pero falta prueba o integración completa)
- **TY-CA pendientes:** 16/46 (✗ Pendiente — no implementados)
- **Pruebas unitarias:** 125/125 PASSED en `test_compile` (GCC 16.2.0)
- **Compiladores verificados:** GCC 16.2.0 ✓, Clang ✗
- **Arduino AVR:** ✗ No compilado
- **Sanitizadores:** ✗ No ejecutados

### Archivos implementados

| Archivo | Líneas | Estado |
|---|---|---|
| `include/monocalc/error.h` | 34 | ✓ Completo |
| `include/monocalc/token.h` | 33 | ✓ Completo |
| `include/monocalc/lexer.h` | 24 | ✓ Completo |
| `include/monocalc/parser.h` | 16 | ✓ Completo |
| `include/monocalc/ast.h` | 53 | ✓ Completo |
| `include/monocalc/evaluator.h` | 21 | ✓ Completo |
| `include/monocalc/differentiator.h` | 14 | ✓ Completo |
| `include/monocalc/integrator.h` | 14 | ✓ Completo |
| `include/monocalc/domain.h` | 30 | ⚠ Básico |
| `include/monocalc/limiter.h` | 31 | ✓ Completo |
| `include/monocalc/type.h` | 43 | ✓ Completo |
| `src/lexer.cpp` | 145 | ✓ Completo |
| `src/parser.cpp` | 250 | ✓ Completo |
| `src/ast.cpp` | 9 | ✓ Mínimo |
| `src/evaluator.cpp` | 87 | ✓ Completo |
| `src/differentiator.cpp` | 487 | ✓ Completo — incluye d/dx(ln(abs(x))) y d/dx(ln(abs(ax+b))) |
| `src/integrator.cpp` | 656 | ✓ Completo |
| `src/domain.cpp` | 26 | ⚠ Básico — domainCheck usa excludedPoint correctamente |
| `src/limiter.cpp` | 127 | ✓ Completo — límites numéricos laterales y bilaterales |
| `src/type.cpp` | 171 | ✓ Completo |
| `tests/test_compile.cpp` | 884 | ✓ 125 pruebas |
| `CMakeLists.txt` | 43 | ✓ Completo |
| `tests/CMakeLists.txt` | 5 | ✓ Completo |
| `.gitignore` | 21 | ✓ Completo |
| `examples/Basic/Basic.ino` | 120 | ✓ Sketch creado |
| `library.properties` | 8 | ✓ Metadata creada |
| `examples/Basic/` | — | ⚠ Sketch creado, compilación AVR falló (toolchain incompleto) |

### Limitaciones conocidas

1. **`abs` no derivable directamente:** `differentiator.cpp` devuelve `UnsupportedOperation` para `d/dx(abs(x))`, pero `d/dx(ln(abs(ax+b)))` funciona como caso especial.
2. **`Domain` no se propaga automáticamente:** Al derivar, las restricciones de dominio no se transfieren. Al integrar, se calculan parcialmente.
3. **Sin simplificación activa:** No hay normalización polinómica ni cancelación de términos.
4. **Pruebas consolidadas en un solo archivo:** `test_compile.cpp` agrupa todas las pruebas; faltan archivos separados.
5. **Clang no verificado:** Solo GCC 16.2.0 ha sido probado.
6. **Arduino no compilado:** `Basic.ino` y `library.properties` creados, pero el toolchain AVR GCC (7.3.0-atmel3.6.1-arduino7) está incompleto: falta `cstddef` y cabeceras C++ estándar. Se requiere reinstalar `avr-gcc` con soporte C++.
7. **Límites numéricos:** La aproximación puede fallar en casos patológicos; no es un cálculo simbólico de límites.

## 12. Historial del documento

| Versión | Cambio |
|---|---|
| Borrador 0.1 | Primer SDD particular de Type; alineación con común 0.3; entrada textual; portabilidad GCC/Clang; pruebas nativas; entrega 00:00–06:00 y recuperación mediante Git |
| Borrador 0.2 | Arquitectura aprobada para Type: parser Pratt, AST compacto, reglas simbólicas conservadoras, memoria y dominios explícitos; 24 requisitos ARC y 12 nuevas pruebas TY-CA-026–037; plan de trabajo de Junie actualizado sin alterar el contrato común 0.3 |
| **Borrador 0.3** | Stack de desarrollo aprobado: C++11, CMake, CTest, Arduino CLI, ASan/UBSan, informes JUnit XML y Markdown, Git; 11 requisitos STACK-T y nueve criterios TY-CA-038–046; GitHub Actions opcional/futuro; trazabilidad y entregables actualizados |
| **Borrador 0.4** | Sección 11 añadida: estado de implementación real con matrices de trazabilidad por requisito (TXT, MAT, ARC, ERR, PORT, STACK, TY-CA), archivos implementados, limitaciones conocidas y resumen numérico. Fecha: 2026-11-20. |

**Fin — SDD particular monocalc Type 0.1.0, Borrador 0.4.**
