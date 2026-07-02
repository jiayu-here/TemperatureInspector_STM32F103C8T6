# Keil Project

Generate the real Keil project from CubeMX:

1. Open `../TemperatureInspector_STM32F103C8T6.ioc`.
2. Set `Toolchain / IDE` to `MDK-ARM`.
3. Click `GENERATE CODE`.
4. CubeMX will create `TemperatureInspector.uvprojx` here.
5. Open it with Keil.

The application code is in:

- `../Core/Inc/app_*.h`
- `../Core/Src/app_*.c`

