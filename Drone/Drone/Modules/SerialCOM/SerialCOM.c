/*
 * SerialCOM.c
 *
 * Created: 12.02.2018 08:58:53
 *  Author: flola
 */ 

#include "SerialCOM.h"

SerialCOM_RECV_CALLBACK SerialCOM_reciveCallBack = NULL;

void message_received(uint8_t* message, uint16_t Length)
{	
	static uint8_t Type = 0;
	if(Length == 3 && message[0] == 0x02)
	{
		Type = message[1];
		UART0_set_receiver_length(message[2]+1);
	} else if(message[Length-1] == 0x03)
	{
		if(SerialCOM_reciveCallBack != NULL)
		{
			SerialCOM_reciveCallBack(message, Type);
		}
		UART0_set_receiver_length(3);
		SerialCOM_put_Command('A', 0x06);	//ACK
	} else
	{
		UART0_set_receiver_length(3);
		SerialCOM_put_debug("error");
		SerialCOM_put_Command('N', 0x06);	//NACK
	}
}

ErrorCode SerialCOM_register_call_back(SerialCOM_RECV_CALLBACK callback)
{
	if(callback == NULL)
		return MODULE_SERIALCOM | FUNCTION_register_call_back | ERROR_GOT_NULL_POINTER;
	SerialCOM_reciveCallBack = callback;
	return SUCCESS;
}

ErrorCode SerialCOM_init()
{
	DEFAULT_ERROR_HANDLER(UART0_init(115200, 1), MODULE_SERIALCOM, FUNCTION_init);
	DEFAULT_ERROR_HANDLER(UART0_set_receiver_length(3), MODULE_SERIALCOM, FUNCTION_init);
	DEFAULT_ERROR_HANDLER(UART0_register_received_callback(message_received), MODULE_SERIALCOM, FUNCTION_init);
	return SUCCESS;
}

ErrorCode SerialCOM_put_message(uint8_t message[], uint8_t Type, uint8_t Length)
{
	uint8_t *transmissionData = malloc((Length+4)*sizeof(uint8_t));
	if(transmissionData == NULL)
	{
		return MODULE_SERIALCOM | FUNCTION_put_message | ERROR_MALLOC_RETURNED_NULL;
	}
	transmissionData[0] = 0x02;
	transmissionData[1] = Type;
	transmissionData[2] = Length;
	memcpy(&transmissionData[3], message, Length);
	transmissionData[Length+3] = 0x03;
	ErrorCode errorReturn = ErrorHandling_set_top_level(UART0_put_data(transmissionData, Length+4), MODULE_SERIALCOM, FUNCTION_put_message);
	free(transmissionData);
	return errorReturn;
}

ErrorCode SerialCOM_force_put_message(uint8_t message[], uint8_t Type, uint8_t Length)
{
	uint8_t *transmissionData = malloc((Length+4)*sizeof(uint8_t));
	if(transmissionData == NULL)
		return MODULE_SERIALCOM | FUNCTION_force_put_message | ERROR_MALLOC_RETURNED_NULL;
	if(Length == 0)
		return MODULE_SERIALCOM | FUNCTION_force_put_message | ERROR_INVALID_ARGUMENT;
	transmissionData[0] = 0x02;
	transmissionData[1] = Type;
	transmissionData[2] = Length;
	memcpy(&transmissionData[3],message, Length);
	transmissionData[Length+3] = 0x03;
	uart0_put_raw_data(transmissionData, Length+4);
	free(transmissionData);
	return SUCCESS;
}

ErrorCode SerialCOM_put_Command(char CommandChar, uint8_t Type)
{
	return ErrorHandling_set_top_level(SerialCOM_put_message(((uint8_t*)&CommandChar), Type, 1), MODULE_SERIALCOM, FUCNTION_put_Command);
}

ErrorCode SerialCOM_put_debug(char Text[])
{
	return ErrorHandling_set_top_level(SerialCOM_put_message(((uint8_t*)Text), 0x00, strlen(Text)), MODULE_SERIALCOM, FUCNTION_put_debug);
}

ErrorCode SerialCOM_put_debug_n(char Text[], uint8_t Length)
{
	return ErrorHandling_set_top_level(SerialCOM_put_message(((uint8_t*)Text), 0x00, Length), MODULE_SERIALCOM, FUCNTION_put_debug_n);
}

ErrorCode SerialCOM_put_error(char Text[])
{
	return ErrorHandling_set_top_level(SerialCOM_put_message(((uint8_t*)Text), 0x64, strlen(Text)), MODULE_SERIALCOM, FUCNTION_put_error);
}

ErrorCode SerialCOM_force_put_error(char Text[])
{
	return ErrorHandling_set_top_level(SerialCOM_force_put_message(((uint8_t*)Text), 0x64, strlen(Text)), MODULE_SERIALCOM, FUCNTION_force_put_error);
}

uint8_t SerialCOM_get_free_space()
{
	return UART0_get_space();
}

void SerialCOM_serializeFloat(float* Value, uint8_t* startptr)
{
	uint32_t NumValue;
	memcpy(&NumValue, Value, 4);
	startptr[0] = (NumValue & 0xFF000000) >> 24;
	startptr[1] = (NumValue & 0x00FF0000) >> 16;
	startptr[2] = (NumValue & 0x0000FF00) >> 8;
	startptr[3] =  NumValue & 0x000000FF;
}

ErrorCode SerialCOM_print_debug(const char *fmt, ...)
{
	char buffer[SERIALCOM_MAX_PRINT_CHARS];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	ErrorCode rt = ErrorHandling_set_top_level(SerialCOM_put_debug(buffer), MODULE_SERIALCOM, FUNCTION_print_debug);
	return rt;
}

ErrorCode SerialCOM_print_error(const char *fmt, ...)
{
	char buffer[SERIALCOM_MAX_PRINT_CHARS];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	ErrorCode rt = ErrorHandling_set_top_level(SerialCOM_put_error(buffer), MODULE_SERIALCOM, FUNCTION_print_error);
	return rt;
}

