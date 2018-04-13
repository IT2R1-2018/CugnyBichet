// Utilisation Event UART en emission-reception

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions

#include "Driver_USART.h"               // ::CMSIS Driver:USART
#include "Board_GLCD.h"                 // ::Board Support:Graphic LCD
#include "GLCD_Config.h"                // Keil.MCB1700::Board Support:Graphic LCD
#include "LPC17xx.h"                    // Device header
#include "cmsis_os.h"                   // CMSIS RTOS header file
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "rl_net.h"
#include "Board_ADC.h" 
#include "Board_LED.h"                  // ::Board Support:LED
#include "Board_Buttons.h"              // ::Board Support:Buttons

extern GLCD_FONT GLCD_Font_6x8;
extern GLCD_FONT GLCD_Font_16x24;
int32_t udp_sock;
char Cconversion;
int i,conversion;
short pot;
uint8_t etat_batterie[4];
void Thread_T (void const *argument);                             // thread function Transmit
osThreadId tid_Thread_T;                                          // thread id
osThreadDef (Thread_T, osPriorityNormal, 1, 0);                   // thread object

void envoi (void const *argument);
osThreadId tid_envoi;
osThreadDef (envoi, osPriorityNormal, 1,0);
void Thread_R (void const *argument);                             // thread function Receive
osThreadId tid_Thread_R;                                          // thread id
osThreadDef (Thread_R, osPriorityNormal, 1, 0);                   // thread object

// prototypes fonctions
void Init_UART(void);
void sendCommand(char * command, int tempo_ms);
void Init_WiFi(void);

extern ARM_DRIVER_USART Driver_USART1;

//fonction de CB lancee si Event T ou R
void event_UART(uint32_t event)
{
	switch (event) {
		
		case ARM_USART_EVENT_RECEIVE_COMPLETE : 	osSignalSet(tid_Thread_R, 0x01);
																							break;
		
		case ARM_USART_EVENT_SEND_COMPLETE  : 	osSignalSet(tid_Thread_T, 0x02);
																							break;
		
		default : break;
	}
}
void send_udp_data (void) {
	NET_ADDR addr = { NET_ADDR_IP4, 2000 , 192, 168, 0, 2 };
}
void send_udp_data_jul (void) {
 
  if (udp_sock > 0) {
     
    // IPv4 address: 192.168.0.2
    NET_ADDR addr = { NET_ADDR_IP4, 2000 , 192, 168, 0, 2 };

    uint8_t *sendbuf;
		ADC_StartConversion();
		//while(ADC_ConversionDone!=0);
		conversion=ADC_GetValue();
		
		etat_batterie[0]=conversion>>8;
		etat_batterie[1]=conversion;
		//printf(etat_batterie, conversion);
		
		
    sendbuf = netUDP_GetBuffer (3);
    sendbuf[0] = 0xAA;
    sendbuf[1] = etat_batterie[0];
		sendbuf[2] = etat_batterie[1];

 
    netUDP_Send (udp_sock, &addr, sendbuf, 3);
    
  }
}
uint32_t udp_cb_func (int32_t socket, const  NET_ADDR *addr, const uint8_t *buf, uint32_t len) {
 char c[5];
	char d[4];
	 if (buf[0] == 0xAA) {
    LED_On (0);
		send_udp_data_jul();
  }
  switch(buf[0]) {
		case 0xBA :
									
									if((buf[1] & 0x01) ==0x00){
									GLCD_DrawString(100,1,"0");
								
									}
									
									if ((buf[1] & 0x01)==0x01){
									GLCD_DrawString(100,1,"1");}
									
									if((buf[1] & 0x10) ==0x00){
									 GLCD_DrawString(100,20,"0");}
									 
									if ((buf[1] & 0x10)==0x10){
									 GLCD_DrawString(100,20,"1");}
									break;
									
		case 0xCC :
		
		sprintf(d,"C%d",buf[1]);
			sendCommand("AT+CIPSEND=5\r\n",2000);
			sendCommand(d,2000);
								
									break;
		}

  return (0);
}
void send_udp_data_places_parking () {
 char data;

  if (udp_sock > 0) {
		
    NET_ADDR addr = { NET_ADDR_IP4, 2000, 192, 168, 0, 7 };
 
    uint8_t *sendbuf;
 
    sendbuf = netUDP_GetBuffer (2);
		data = 0xBA;
    *sendbuf = data;
 
    netUDP_Send (udp_sock, &addr, sendbuf, 2);
    
  }
}
int main (void){

	osKernelInitialize ();                    // initialize CMSIS-RTOS
	
	// initialize peripherals here
	Init_UART();
	GLCD_Initialize();
	GLCD_ClearScreen();
	GLCD_SetFont(&GLCD_Font_6x8);
	NVIC_SetPriority(UART1_IRQn,2);
	netInitialize();
	LED_Initialize();
	ADC_Initialize();
	//Creation des 2 taches
	tid_Thread_T = osThreadCreate(osThread(Thread_T),NULL);
	tid_Thread_R = osThreadCreate(osThread(Thread_R),NULL);
	tid_envoi = osThreadCreate(osThread(envoi),NULL);
		udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) {
    netUDP_Open (udp_sock, 2000);}
	
	osKernelStart ();                         // start thread execution 
	
	osDelay(osWaitForever);
	
	return 0;
}

void envoi(void const *argument)
{
	int status;
		while(1)
	{
		 status=Buttons_GetState ();
	
	if(status == 1)
	{send_udp_data_places_parking();
	LED_On (0x02);}
	}
}
void Thread_T (void const *argument) {
	
	char Cmd[30];
	char ReqHTTP[90];
	Init_WiFi();
	while(1)
	{
		//sendCommand("AT+CIPSEND=4\r\n",2000);
		//sendCommand("test",2000);
	}
	
	
}

// Tache de réception et d'affichage

void Thread_R (void const *argument) {

	char RxChar;
	int ligne;
	int i=0;	// i pour position colonne caractère
	char RxBuf[200];
	
  while (1) {
		Driver_USART1.Receive(&RxChar,1);		// A mettre ds boucle pour recevoir 
		osSignalWait(0x01, osWaitForever);	// sommeil attente reception
		
		RxBuf[i]=RxChar;
		i++;
		//Suivant le caractère récupéré
		switch(RxChar)
		{
			case 0x0D: 		//Un retour chariot? On ne le conserve pas...
				i--;
				break;
			case 0x0A:										//Un saut de ligne?
				RxBuf[i-1]=0;											//=> Fin de ligne, donc, on "cloture" la chaine de caractères
				GLCD_DrawString(1,ligne,RxBuf);	//On l'affiche (peut etre trop long, donc perte des caractères suivants??)
				ligne+=10;										//On "saute" une ligne de l'afficheur LCD
			  if(ligne>220)
				{
					ligne=1;
					GLCD_ClearScreen();
					osDelay(1000);
				}
				i=0;													//On se remet au début du buffer de réception pour la prochaine ligne à recevoir
				break;
		}
  }
}

void Init_UART(void){
	Driver_USART1.Initialize(event_UART);
	Driver_USART1.PowerControl(ARM_POWER_FULL);
	Driver_USART1.Control(	ARM_USART_MODE_ASYNCHRONOUS |
							ARM_USART_FLOW_CONTROL_NONE   |
							ARM_USART_DATA_BITS_8		|
							ARM_USART_STOP_BITS_1		|
							ARM_USART_PARITY_NONE		,							
							115200);
	Driver_USART1.Control(ARM_USART_CONTROL_TX,1);
	Driver_USART1.Control(ARM_USART_CONTROL_RX,1);
}

void sendCommand(char * command, int tempo_ms)
{
	int len;
	len = strlen (command);
	Driver_USART1.Send(command,len); // send the read character to the esp8266
	osSignalWait(0x02, osWaitForever);		// sommeil fin emission
	osDelay(tempo_ms);		// attente traitement retour
}


void Init_WiFi(void)
{
	// reset module
	sendCommand("AT+RST\r\n",7000); 
	
	// disconnect from any Access Point
	sendCommand("AT+CWQAP\r\n",2000);
	
	sendCommand("AT+CWMODE=3\r\n",2000);
	
  // configure as Station 
	sendCommand("AT+CWJAP=\"IT2R1\",\"It2r2018\"\r\n",7000);
	
	
	//Connect to YOUR Access Point
		sendCommand("AT+CIFSR\r\n",7000);
	
	sendCommand("AT+CIPSTART=\"TCP\",\"192.168.0.100\",333\r\n",10000);
	//sendCommand("AT+CIPMUX=1\r\n",2000);
	
	
	//sendCommand("AT+CIPSERVER=1\r\n",7000);
	
	

	
	
	
}



