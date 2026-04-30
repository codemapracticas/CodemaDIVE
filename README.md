# CodemaDIVE

Desarrollo de una estación medidora de la calidad del agua y una aplicación para SmartPhones que permite su control a distancia y análisis de las mediciones. Este desarrollo se realizó para la IV edición de las Teleco Games (Olimpiada de Ingeniería de Telecomunicaciones).

Esta competición se desarrolló en la Escuela Politécnica de Ingeniería de Gijón (EPI) en abril de 2026.

https://olimpiadasteleco.com/

Esta solución de ciencia ciudadana consta de dos partes. La estación propiamente dicha que posee un microcontrolador Aruino ESP-32 y cuatro sensores responsables de medir las siguientes características físicas del agua:
  * Temperatura
  * Ph
  * TDS (conductividad eléctrica)
  * Turbidez

La otra parte es una App para smartphones (Android) que facilita la realización de las mediciones, la recogida y almacenamiento de los datos en la nube y posteriormente, permite su estudio mediante la realización de consultas, gráficas o visualización de las medidas geolocalizadas en un mapa.

Es de destacar, el desarrollo de la parte de ciberseguridad para que la aplicación y los datos no estén accesibles a todo el mundo. Se requiere el registro de los usuarios que vayan ausar la aplicación, facilitando la autenticación de 2 pasos a los usuarios. La encriptación y seguridad de las contraseñas usadas, ...
