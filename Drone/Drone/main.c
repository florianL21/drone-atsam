/*
 * Drone.c
 *
 * Created: 15.10.2017 14:14:19
 * Author : flola
 */ 


/*

// Enable IO
PIOB->PIO_PER = PIO_PB27;
// Set to output
PIOB->PIO_OER = PIO_PB27;
// Disable pull-up
PIOB->PIO_PUDR = PIO_PB27;
PIOB->PIO_CODR = PIO_PB27;

*/


#include "sam.h"
#include "uart0.h"
#include "USART0.h"
#include "BNO055.h"
#include "HelperFunctions.h"
#include "ESCControl.h"
#include "RCReader.h"
#include "PID.h"

BNO055_eulerData SensorValues;
//Init variables for the drone programm
RemoteControlValues RemoteValues;
uint16_t Motor_speeds[4] = {0};
float ValueMapFactor = 0.3;
uint8_t maximumControlDegree = 10;

//PID Config:
float PID_PitchInput = 0,	PID_PitchOutput = 0,	PID_PitchSetPoint = 0;
float PID_RoleInput = 0,	PID_RoleOutput = 0,		PID_RoleSetPoint = 0;
float PitchKp = 0.5,	PitchKi = 0.3,		PitchKd = 0.008;
float RoleKp = 0.5,		RoleKi = 0.3,		RoleKd = 0.008;
pidData PitchPid;
pidData RolePid;

void configure_wdt(void)
{
	WDT->WDT_MR = 0x00000000; // disable WDT
}

void BNO_Error(BNO_STATUS_BYTES Error, StatusCode Transmit_error_code)
{
	if(Error == BNO_STATUS_BUS_OVER_RUN_ERROR && Transmit_error_code == BNO055_ERROR)
		BNO055_start_euler_measurement(true,true);
	else 
	{
		_Delay(840000);
		UART0_puts("ERROR ");
		UART0_put_int(Error);
		UART0_puts("\n\r");
	}
}

void DataReady()
{
	bool needComputePitch = PID_need_compute(&PitchPid);
	bool needComputeRole = PID_need_compute(&RolePid);
	if(needComputeRole == true || needComputePitch == true)
	{
		//read Values from sensor and remote control:
		SensorValues = BNO055_get_euler_measurement_data();
		RemoteValues = rc_read_values();
		
		if(RemoteValues.error != true)
		{
			//UART0_puts("Values OK\n\r");
			//int16_t MappedYaw   = map(RemoteValues.Yaw, 0, RC_CONTROL_CENTER__PITCH * 2, -(RemoteValues.Throttle * ValueMapFactor), (RemoteValues.Throttle * ValueMapFactor));
			
			//pitch --> role: 180;-180
			//role --> pitch: 90;-90
			
			PID_PitchInput		= SensorValues.role;
			PID_RoleInput		= SensorValues.pitch;
			PID_PitchSetPoint	= map(RemoteValues.Pitch, 0, RC_CONTROL_CENTER__PITCH * 2, -maximumControlDegree, maximumControlDegree);
			PID_RoleSetPoint	= map(RemoteValues.Role, 0, RC_CONTROL_CENTER__PITCH * 2, -maximumControlDegree, maximumControlDegree);
			
			// Compute new PID output value
			if (needComputePitch)
				PID_Compute(&PitchPid);
			if (needComputeRole)
				PID_Compute(&RolePid);

			int16_t MappedPitch = map(PID_PitchOutput, -180, 180, -(RemoteValues.Throttle * ValueMapFactor), (RemoteValues.Throttle * ValueMapFactor));
			int16_t MappedRole  = map(PID_RoleOutput, -90, 90, -(RemoteValues.Throttle * ValueMapFactor), (RemoteValues.Throttle * ValueMapFactor));
			//int16_t MappedYaw   = map(PID_YawOutput, -1000, 1000, -(RemoteValues.Throttle * ValueMapFactor), (RemoteValues.Throttle * ValueMapFactor));
			Motor_speeds[0] = RemoteValues.Throttle - MappedPitch - MappedRole;// - MappedYaw;
			Motor_speeds[1] = RemoteValues.Throttle - MappedPitch + MappedRole;// + MappedYaw;
			Motor_speeds[2] = RemoteValues.Throttle + MappedPitch - MappedRole;// - MappedYaw;
			Motor_speeds[3] = RemoteValues.Throttle + MappedPitch + MappedRole;// + MappedYaw;
		
			esc_set(1, Motor_speeds[0]);
			esc_set(2, Motor_speeds[1]);
			esc_set(3, Motor_speeds[2]);
			esc_set(4, Motor_speeds[3]);
		
		
			/*if(UART0_is_idle())
			{
				char buffer[100] = "";
				sprintf(buffer, "In: %3.3f\tOut: %3.3f\tSet: %3.3f\n\r", PID_RoleInput, PID_RoleOutput, PID_RoleSetPoint);
				UART0_puts(buffer);
			}*/
			/*if(UART0_is_idle())
			{
				char buffer[100] = "";
				sprintf(buffer, "role: %3.3f\tpitch: %3.3f\theading: %3.3f\n\r", SensorValues.role, SensorValues.pitch, SensorValues.heading);
				UART0_puts(buffer);
			}*/
		}
	}
}

int main(void)
{
	SystemInit();
	configure_wdt();
	UART0_init(115200,1);
	UART0_puts("Go!\n\r");
	//BNO init:
	BNO055_init(true);
	UART0_puts("Calib OK\n\r");
	BNO055_register_error_callback(BNO_Error);
	
	BNO055_register_data_ready_callback(DataReady);
	//rc control and esc init:
	rc_init();
	esc_init();
	
	PID_Init();
	PID_Initialize(&PitchPid, &PID_PitchInput, &PID_PitchOutput, &PID_PitchSetPoint, PitchKp, PitchKi, PitchKd,-180,180,10);
	PID_Initialize(&RolePid, &PID_RoleInput, &PID_RoleOutput, &PID_RoleSetPoint, RoleKp, RoleKi, RoleKd,-90,90,10);
	UART0_puts("ALL INITS DONE!\n\r");
	BNO055_start_euler_measurement(true,true);
	while(1)
	{
		
	}
}


/* PID Test Program:*/
/*

float Input=0;
float Output=0;
float SetPoint=0;
float Kp=0.5;
float Ki=0.3;
float Kd=0.008;

pidData myPid;

int main(void)
{
	SystemInit();
	configure_wdt();
	uart0_init(115200);
	rc_init();
	PID_Init();
	
	PID_Initialize(&myPid, &Input, &Output, &SetPoint, Kp, Ki, Kd,-1000,1000,10);
	
	RemoteControlValues Values;
	while(1)
	{
		Values = rc_read_values();
		SetPoint = Values.Throttle;
		if (PID_need_compute(&myPid))
		{
			// Compute new PID output value
			PID_Compute(&myPid);
			//Change actuator value
			if(uart0_has_space())
			{
				uint8_t numBuf[20] = "";
				uint8_t buffer[50] = "";
				strcat(buffer,"In: ");
				itoa(Input,numBuf,10);
				strcat(buffer,numBuf);
				strcat(buffer,"\tOut: ");
				itoa(Output,numBuf,10);
				strcat(buffer,numBuf);
				strcat(buffer,"\tSet: ");
				itoa(SetPoint,numBuf,10);
				strcat(buffer,numBuf);
				strcat(buffer,"\n\r");
				uart0_puts(buffer);
			}
			Input += Output;
		}
		
	}
}
*/


/*

/ *Drone Test Program:* /

int main(void)
{
	/ * Initialize the SAM system * /
	SystemInit();
	configure_wdt();
	rc_init();
	esc_init();
	RemoteControlValues RemoteValues;
	int16_t MappedPitch=0;
	int16_t MappedRole=0;
	int16_t MappedYaw=0;
	uint16_t Motor_speeds[4] = {0};
	float ValueMapFactor = 0.3;
	while (1)
	{
		RemoteValues = rc_read_values();
		if(RemoteValues.error != true)
		{
			MappedPitch = map(RemoteValues.Pitch,0,RC_CONTROL_CENTER__PITCH*2,-(RemoteValues.Throttle*ValueMapFactor),(RemoteValues.Throttle*ValueMapFactor));
			MappedRole  = map(RemoteValues.Role,0,RC_CONTROL_CENTER__PITCH*2,-(RemoteValues.Throttle*ValueMapFactor),(RemoteValues.Throttle*ValueMapFactor));
			MappedYaw   = map(RemoteValues.Yaw,0,RC_CONTROL_CENTER__PITCH*2,-(RemoteValues.Throttle*ValueMapFactor),(RemoteValues.Throttle*ValueMapFactor));
			
			
			Motor_speeds[0] = RemoteValues.Throttle-MappedPitch-MappedRole-MappedYaw;
			Motor_speeds[1] = RemoteValues.Throttle-MappedPitch+MappedRole+MappedYaw;
			Motor_speeds[2] = RemoteValues.Throttle+MappedPitch-MappedRole-MappedYaw;
			Motor_speeds[3] = RemoteValues.Throttle+MappedPitch+MappedRole+MappedYaw;
			
			esc_set(1,Motor_speeds[0]);
			esc_set(2,Motor_speeds[1]);
			esc_set(3,Motor_speeds[2]);
			esc_set(4,Motor_speeds[3]);
			
			if(uart0_has_space())
			{
				uint8_t numBuf[20] = "";
				uint8_t buffer[50] = "";
				strcat(buffer,"1: ");
				itoa(Motor_speeds[0],numBuf,10);
				strcat(buffer,numBuf);
				itoa(Motor_speeds[1],numBuf,10);
				strcat(buffer,"\t2: ");
				strcat(buffer,numBuf);
				itoa(Motor_speeds[2],numBuf,10);
				strcat(buffer,"\t3: ");
				strcat(buffer,numBuf);
				itoa(Motor_speeds[3],numBuf,10);
				strcat(buffer,"\t4: ");
				strcat(buffer,numBuf);
				strcat(buffer,"\n\r");
				uart0_puts(buffer);
			}
		}
		
	}
}
*/






/*
void Test(uint8_t* Data, uint16_t Length)
{
	uint8_t test[3]="\r\n";
	USART0_put_data(Data,Length);
	USART0_put_data(test,2);
	USART0_set_receiver_length(1);
}

int main(void)
{
	
	SystemInit();
	configure_wdt();
	PIOB->PIO_PER = PIO_PB27;
	// Set to output
	PIOB->PIO_OER = PIO_PB27;
	// Disable pull-up
	PIOB->PIO_PUDR = PIO_PB27;
	PIOB->PIO_CODR = PIO_PB27;
	USART0_init(115200,3);
	USART0_register_received_callback(Test);
	
	while(1)
	{
	}
}*/