/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/
/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <string.h>
#include "app.h"        // also beings in app_tone_lookup_table.h
#include "../src/ble/non_gui/ble.h"
#include "click_routines/weather/weather.h"
#include "click_routines/10dof/10dof.h"
extern BLE_DATA bleData;
extern volatile bool cmd_sent;
// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;

void read_PTH_values(void);
// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/// *****************************************************************************
 /*
  Function:
        static void App_TimerCallback
        (
            uintptr_t context
        )

  Summary:
    Implements the handler for timer callback function.

  Description:
    Called every 1 ms by timer services.  It then decrements WM8904Delay if
    non-zero.

  Parameters:
    context      - Contains index into driver's instance object array

  Remarks:
    None
*/
static void App_TimerCallback( uintptr_t context)
{    
    if (appData.waitingToConnect)
    {
        appData.waitingToConnectTimer++;
        if ((appData.waitingToConnectTimer & 0xFF) == 0)    // about every 1/4 second
        {
            LED1_Toggle();
        }
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary local functions.
*/

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */
void APP_Initialize ( void )
{
   
     
    DRV_BM71_Initialize();
    /* TODO: Initialize your application's state machine and other
     * parameters.
     */   

	LED1_On();       // initially on, until connected
    appData.state = APP_STATE_INIT;   

    appData.queryDelay = 0;         // used for status queries

    appData.waitingToConnect = false;
    appData.waitingToConnectTimer = 0;

    bleInitialize(true);
}

/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */
DRV_HANDLE tmrHandle;

void APP_Tasks ( void )
{    
    DRV_BM71_Tasks();
    bleTasks(); 
    
    /* Check the application's current state. */
    switch ( appData.state )
    {       
        /* Application's initial state. */
        case APP_STATE_INIT:
        {
            /* Open the timer Driver */
            tmrHandle = SYS_TIME_CallbackRegisterMS(App_TimerCallback, 
                    (uintptr_t)0, 1/*ms*/, SYS_TIME_PERIODIC);

            if ( SYS_TIME_HANDLE_INVALID != tmrHandle )
            {
               appData.state = APP_STATE_WAIT_INIT;
            }            
        }
        break;
        
        case APP_STATE_WAIT_INIT:
        {
            if (DRV_BT_STATUS_READY == DRV_BT_GetPowerStatus())
            {           
                tenDof_init();
                appData.state=APP_STATE_IDLE;
            }
        }
        break;
        
        // Initialized 
        case APP_STATE_IDLE:
        {
             
            APP_Sensor_Tasks();
            
            if (appData.queryDelay == 0)
            {
                DRV_BT_BLE_QueryStatus(bleData.bt.handle);
                appData.queryDelay = QUERY_DELAY;
            }
		}
		break;
         
        default:
        {
            /* TODO: Handle error in application's state machine. */
        }
        break;             
    }
}
volatile bool gyro_request = 0 ,environment_request = 0,cmd_successfull = 0,rotation_vector_request = 0;
volatile bool accel_request = 0;
void APP_Sensor_Tasks()
{
struct bno055_gyro_t gyro_data;
struct bno055_accel_t accel_data;    
struct bno055_quaternion_t quaternion_data;

 if((cmd_successfull == 1) || (cmd_sent == 0))
 {
    if(cmd_successfull == 1){
        cmd_successfull = 0;
    }   
    if(gyro_request){   
        tenDof_get_bno55_gyro_xyz(&gyro_data);
        printgyro(&gyro_data);
        SYSTICK_DelayMs(200);
    }else if(accel_request){       
        tenDof_get_bno55_accel_xyz(&accel_data);
        printaccel(&accel_data);
        SYSTICK_DelayMs(200);
    }else if(environment_request){
        read_PTH_values();
        SYSTICK_DelayMs(20);  
    }else if(rotation_vector_request){
        tenDof_get_bno55_quaternion_wxyz(&quaternion_data);
        printquaternion(&quaternion_data);
        SYSTICK_DelayMs(500);
        
    }
 }
}

typedef struct
{
    int16_t temperature;
    uint16_t pressure;
    float humidity;
    
}BME_SENSOR_DATA;

BME_SENSOR_DATA              meas_Data;

void read_PTH_values(void)
{
    uint8_t tempBuffer[21] = {0};
    
    Weather_readSensors();

    meas_Data.temperature = Weather_getTemperatureDegC()*100;
    meas_Data.humidity = Weather_getPressureKPa();
    meas_Data.pressure = Weather_getHumidityRH();
    
    tempBuffer[0] = ((uint8_t*)&meas_Data.temperature)[0];
    tempBuffer[1] = ((uint8_t*)&meas_Data.temperature)[1];
    tempBuffer[2] = ((uint8_t*)&meas_Data.pressure)[0];
    tempBuffer[3] = ((uint8_t*)&meas_Data.pressure)[1];
    tempBuffer[4] = 0;      /* Light Sensor Not supported */ 
    tempBuffer[5] = 0;      /* Light Sensor Not supported */
    tempBuffer[6] = 0;      /* Light Sensor Not supported */
    tempBuffer[7] = 0;      /* Light Sensor Not supported */
    tempBuffer[8] = (uint8_t)meas_Data.humidity;
    tempBuffer[9] = '\r';
    tempBuffer[10] = '\n';  
    Sendenv(tempBuffer);

}
/*******************************************************************************
 End of File
 */
