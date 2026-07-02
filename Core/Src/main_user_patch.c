/*
 * This file shows the required CubeMX main.c user patch.
 * Do not compile this file together with generated main.c.
 *
 * In CubeMX-generated Core/Src/main.c:
 *
 * USER CODE BEGIN Includes
 * #include "app.h"
 * USER CODE END Includes
 *
 * USER CODE BEGIN 2
 * App_Init();
 * USER CODE END 2
 *
 * while (1)
 * {
 *   USER CODE BEGIN WHILE
 *   App_Loop();
 *   USER CODE END WHILE
 * }
 */

