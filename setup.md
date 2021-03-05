# 



### Include files

Copy the "Nextion_HMI" folder in to your project, then include to your project the "Inc" folder. 

Steps are: 

Right click on to your project Name, then choose Properties

Click on to "C/C++ Build  -  Settings"

Click on Add under ToolSettings/MCU GCC Compiler/IncludePaths

Click on Workspace and choose the "Nextion_HMI/Inc" folder

Click on Apply

<img title="" src="./images/Include_Header.jpg" alt="Algorithm schema" data-align="center" width="720">

### Configure printf

Click on MCU Settings, Use float with printf

Click on Apply and Close

<img title="" src="./images/Project_Setting_sprintf.jpg" alt="Algorithm schema" data-align="center">

### Clock Source Config

This is my chosen clock settings, configure according your needs.

![Algorithm schema](./images/ClockTree72MHz_HSE.jpg)

### File generating config

![Algorithm schema](./images/Project_Config.jpg)

### UART Config

Choose the UART you want to use.

Mode: Asynchronous.

Parameters: 9600, 8 N 1

![Algorithm schema](./images/USART_Parameter.jpg)

### NVIC Config

Enable Global Interrupt for chosen UART

![Algorithm schema](./images/NVIC_Config.jpg)

Tick mark Generate IRQ Handler for chosen UART

![Algorithm schema](./images/NVIC_Config2.jpg)

### SysTick Source

Choose an a unused timer for Timebase Source. (FreeRTOS will use SysTick timer)

![Algorithm schema](./images/SYS_TimeBase.jpg)

### FreeRTOS config

Enable FreeRTOS, choose CMSIS_V2 and create tasks

![Algorithm schema](./images/FREERTOS_CMSISv2.jpg)
