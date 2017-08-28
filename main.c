/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * main.c
 * 
 * Autor:   Alfredo Orozco 
 * Fecha:   Mayo 1 del 2017
 * Versión: 1.0
 * 
 * Descripción:
 * 
 *     Temporizador de encendido para Bomba de Agua.
 *     
 *     Este programa controla el tiempo de encendido de
 *     una bomba de agua usada para bombear el liquido al
 *     tinaco de un edificio.
 *     
 *     El circuito utiliza un relevador de estado sólido para
 *     el control del la bomba de agua, 2 displays de 7 
 *     segmentos para visualizar el tiempo de trabajo y 3
 *     botones para cambiar el tiempo de trabajo y para
 *     iniciar o detener el temporizador.
 * 
 * 	   Para más detalles de la conexión, ver el esquemático.
 * 	   
 *     Copyright© Alfredo Orozco <alfredoopa@gmail.com>
 *     
 *++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


#include "stm8happy.h"

// Hay que habilitar los módulos a usar en el archivo stm8happy_cfg.h
#include "tim2.h"		// Bilbioteca para el manej del Timer 2.
#include "gpio.h"		// Bilbioteca para el manejo de los GPIO.
#include "itc.h"        // Biblioteca para el manejo de interrupciones.


#define MAX_TIME_RUNNING 			55	// Tiempo máximo que puede funcionar el sistema.
#define ANIMATION_DURATION_SHORT    8	// Duración corta para animación.
#define ANIMATION_DURATION_NORMAL   16	// Duración media para animación.
#define ANIMATION_DURATION_LONG     26  // Duración larga para animación.

#define LED_BOARD_GPIO  GPIOB			// Puerto GPIO para el led de la tarjeta.
#define LED_BOARD       GPIO_PIN_5		// Pin del led de la tarjeta.

#define BOMBA_GPIO  GPIOA				// Puerto GPIO para el relay de la bomba
#define BOMBA_PIN   GPIO_PIN_3			// Pin del relay de la bomba.

#define BOTONES_GPIO    GPIOD			// Puerto GPIO para los botones
#define BOTON_MENOS     GPIO_PIN_4		// Pin del botón -
#define BOTON_MAS       GPIO_PIN_5		// Pin del botón +
#define BOTON_START     GPIO_PIN_6		// Pin del boton de INICIO
// Máscara de bits de los pines de los botones.
#define BOTONES_MASK    ( BOTON_MAS | BOTON_MENOS | BOTON_START )

#define DISPLAY_1_GPIO GPIOD	// Primer puerto GPIO para los displays
#define DISPLAY_2_GPIO GPIOC	// Segundo puerto GPIO para los displays

#define DISPLAY_A GPIO_PIN_3	// Pin del segmento A del display.
#define DISPLAY_B GPIO_PIN_2	// Pin del segmento B del display.
#define DISPLAY_C GPIO_PIN_1	// Pin del segmento C del display.
#define DISPLAY_D GPIO_PIN_7	// Pin del segmento D del display.
#define DISPLAY_E GPIO_PIN_6	// Pin del segmento E del display.
#define DISPLAY_F GPIO_PIN_5	// Pin del segmento F del display.
#define DISPLAY_G GPIO_PIN_4	// Pin del segmento G del display.
// Máscara de bits 1 de los pines del display
#define DISPLAY_1_MASK ( DISPLAY_A | DISPLAY_B | DISPLAY_C )

// Máscara de bits 2 de los pines del display
#define DISPLAY_2_MASK ( DISPLAY_D | DISPLAY_E | DISPLAY_F | DISPLAY_G )

#define DISPLAY_1_EN_GPIO   GPIOC		// Puerto GPIO para la habilitación del display 1
#define DISPLAY_1_EN        GPIO_PIN_3	// Pin de habilitación del display 1

#define DISPLAY_2_EN_GPIO   GPIOB		// Puerto GPIO para la habilitación del display 2
#define DISPLAY_2_EN        GPIO_PIN_4	// Pin de habilitación del display 2


/**
 * @brief      Funcion de retardo
 *
 * @param[in]  count  Intervalo a demorar.
 */
void delay(uint32_t count);

/**
 * @brief      Inicializa el sistema
 * 
 * Hace las llamadas a las funciones de configuración
 * de los módulos del sistema a utilizar.
 */
void System_Init();
/**
 * @brief      Inicializa el Timer 2 (TIM2)
 * 
 * Configura el TIM2 para que genere una interrupción
 * cada 1 segundo, se utiliza para contar tiempo.
 */
void TIM2_Init();
/**
 * @brief      Inicializa el LED de la tarjeta
 */
void LED_Init();
/**
 * @brief      Inicializa el pin para el relevador.
 * 
 * Configura el pin conectado al relevador para la 
 * bomba como salida.
 */
void BombaRelay_Init();
/**
 * @brief      Inicializa los displays de 7 segmentos 
 * 			   configurando sus pines.
 */	
void Displays_Init();
/**
 * @brief      Inicializa los botones.
 * 
 * Inicializa los botones para el control del sistema.
 * Habilita la interrupción para atender cada pulsación
 * de los botones.
 */
void Botones_Init();

/**
 * @brief      Envía un dato al display.
 * 
 * Envía el dato separando sus bits para
 * cada pin del display.
 *
 * @param[in]  data  Dato codificado a mostrar en el display.
 */
void WriteDisplayData(uint8_t data);
/**
 * @brief      Muestra un número en los displays.
 * 
 * Muestra un número en formato decimal en los displays, 
 * separando las unidades y las decenas, para posteriormente
 * ser enviados a cada display por separado, haciendo el
 * efecto de que ambos displays están encendidos al mismo
 * tiempo (POV)
 *
 * @param[in]  numero  Número a mostrar en los displays
 */
void MuestraNumeroDisplays(unsigned char numero);
/**
 * @brief      Enciende la bomba de agua.
 * 
 * Activa el relevador de la bomba de agua.
 */
void Enciende_Bomba();
/**
 * @brief      Apaga la bomba de agua.
 * 
 * Desactiva el relevador de la bomba de agua.
 */
void Apaga_Bomba();

/**
 * @brief      Inicia el sistema de temporización.
 * 
 * Esta función ejecuta las acciones para inciar la
 * temporización del sistem: enciende la bomba
 * y activa el timer para contar el tiempo transcurrido.
 */
void System_Start();

/**
 * @brief      Detiene el sistema de temporización.
 * 
 * Esta función detiene la temporización. Reinicia los 
 * contadores de tiempo, apaga la bomba y detiene el
 * timer de conteo.
 */
void System_Stop();
/**
 * @brief      Muestra una animación en los displays.
 *
 * Muestra una animación de parpadeo en los displays.
 * La animacíón se muestra en ambos displays, y la
 * duración es el numero de parpadeos que hace la animación.
 * 
 * @param[in]  show      Dato a enviar por los displays.
 * @param[in]  duration  Número de repeticiones de la animación.
 */
void System_EndAnimation(uint8_t show, uint8_t duration);


volatile uint8_t seconds;		// Guarda los segundos transcurridos de la temporización
volatile uint8_t minutes;		// Guarda los minutos transcurridos de la temporización
volatile uint8_t is_running;	// Guarda el estado del sistema, si esta corriendo o no.
volatile uint8_t time_left;		// Guarda el tiempo restante de trabajo del sistema.

uint8_t user_minutes;			// Guarda los minutos de trabajo ingresados por el usuario


uint8_t display_codes[10] = {	// Arreglo con los digitos codificados para el display.
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
 };

 /**
  * @brief      Funcion de servicio de interrupción para manejar 
  * 			el desbordamiento del TIMER2 cada segundo.
  */
void TIM2_OVF_IRQ() {
    
    
    seconds++;

    if(seconds == 60){
        seconds=0;
        minutes++;

        if(minutes >= MAX_TIME_RUNNING)
            System_Stop();
        
        time_left = user_minutes - minutes;
    }

    // Si terminó el tiempo, apaga el sistema
    if(time_left == 0)
    {
        System_Stop();
        System_EndAnimation(0x40, ANIMATION_DURATION_NORMAL);
    }

    ClearBit(TIM2_SR1, TIM2_SR1_UIF);

}

/**
 * @brief      Función para manejar la interrupción externa en el pin  3  
 * 			   del puerto  GPIOD configurado como interrupción en flanco
 * 			   de bajada.
 */
void EXTI_PORTD_IRQ() {
    
    uint8_t butons_state;

    // Guarda el estado de los botones.
    butons_state = GPIORead(BOTONES_GPIO);

    // Desactiva interrupción externa en los botones
    GPIODisableExtInt(BOTONES_GPIO, BOTONES_MASK);

    // Si se presiona el boton de inicio...
	MuestraNumeroDisplays(user_minutes);
    if((butons_state & BOTON_START) == LOW) {

        // Espera a que se suelte el botón
        while((GPIORead(BOTONES_GPIO) & BOTON_START) == LOW)
			MuestraNumeroDisplays(user_minutes);
				
        // Si la temporización está iniciada, la detiene y muestra animación.
        if(is_running == TRUE) {
			System_Stop();
			System_EndAnimation(0x40, ANIMATION_DURATION_NORMAL);
        }
		else // Si la temporización está detenida, la inicia.
		{
			System_Start();
			delay(0xffff);
		}
		
	}
	else // Si se presionó el boton - ...
	if((butons_state & BOTON_MENOS) == LOW) {
		
		// Espera a que se suelte el botón
		while((GPIORead(BOTONES_GPIO) & BOTON_MENOS)== LOW)
			MuestraNumeroDisplays(user_minutes);
			
		// Si no está corriendo la temporización, decrementa el tiempo de temporización.
		if(is_running == FALSE) {
			user_minutes--;
			if(user_minutes==0)
				user_minutes = MAX_TIME_RUNNING;
		}
		else { // Si el sistema está corriendo, muestra animación de advertencia
			System_EndAnimation(0x79, ANIMATION_DURATION_SHORT);
		}
	}
	else // Si se presionó el boton + ...
	if((butons_state & BOTON_MAS) == LOW) {
		// Espera a que se suelte el botón
		while((GPIORead(BOTONES_GPIO) & BOTON_MAS) == LOW)
			MuestraNumeroDisplays(user_minutes);
		
        // Si no está corriendo la temporizacipon, incrementa el tiempo de temporización.
        if(is_running == FALSE) {
            user_minutes++;
            if(user_minutes> MAX_TIME_RUNNING)
                user_minutes = 1;
        }
        else {// Si el sistema está corriendo, muestra animación de advertencia
            System_EndAnimation(0x79, ANIMATION_DURATION_SHORT);
        }
    }
  
    // Activa de nuevo las interrupción externas en los botones
    GPIOEnableExtInt(BOTONES_GPIO, BOTONES_MASK);
}

/**
 * Función main.
 */
void main() {

	// Inicializa el sistema
    System_Init();	

    // Ciclo infinito y más allá.
    while(TRUE){
        
        // Si la temporización esta corriendo ..
        if(is_running){
            if( !(seconds%2)) {
                // Muestra el tiempo de temporización restante
                MuestraNumeroDisplays(time_left);
            }
            else{
                // Apaga los displays
                GPIOWriteBit(DISPLAY_1_EN_GPIO, DISPLAY_1_EN, LOW);
                GPIOWriteBit(DISPLAY_2_EN_GPIO, DISPLAY_2_EN, LOW);
            }
        }
        else { // Si la temporización no está corriendo, muestra el tiempo
               // de temporización ingresado por el usuario.
            MuestraNumeroDisplays(user_minutes);
        }
    }
        
}

void System_Init() {

    /* Deshabilita todas las interrupciones. */
    DisableInterrupts();
   
    CLK_CKDIVR = 0; // Frecuencia del CPU a 16 MHz

    BombaRelay_Init();	
    Displays_Init();
    Botones_Init();
    LED_Init();     
    TIM2_Init();    
    System_Stop();

    user_minutes = 1;

    /* Habilita todas las interrupciones. */
    EnableInterrupts();
}

void System_Start() {
	
	time_left = user_minutes;
	// Enciende el LED
	GPIOWriteBit(LED_BOARD_GPIO, LED_BOARD, LOW);
	// Enciende la bomba
	Enciende_Bomba();
	// Inicia el timer.
	// Reinicia contadores
	TIM2_CNTRH = 0;
	TIM2_CNTRL = 0;
	seconds = 0;
	minutes = 0;

	TIM2Start();
	is_running = TRUE;
	}

void System_Stop() {

	// Apaga la bomba
    Apaga_Bomba();
    // Detiene el timer
    TIM2Stop();
    // Apaga el led
    GPIOWriteBit(LED_BOARD_GPIO, LED_BOARD, HIGH);
    is_running = FALSE;
}

void System_EndAnimation(uint8_t show, uint8_t duration) {
    unsigned char i;

    // Envía el dato a los displays.
    WriteDisplayData(show);
    for (i = 0; i < duration; ++i)
    {
        if(!(i%2)){
        	// Apaga ls displays
            GPIOWriteBit(DISPLAY_1_EN_GPIO, DISPLAY_1_EN, HIGH);
            GPIOWriteBit(DISPLAY_2_EN_GPIO, DISPLAY_2_EN, HIGH);
        }
        else{
        	// Enciende ls displays
            GPIOWriteBit(DISPLAY_1_EN_GPIO, DISPLAY_1_EN, LOW);
            GPIOWriteBit(DISPLAY_2_EN_GPIO, DISPLAY_2_EN, LOW);

        }

        // Retardo de la animación
        delay(0xFFFF);
        
    }
}

void Enciende_Bomba() {
	// Habilita el relevador de la bomba
    GPIOWriteBit(BOMBA_GPIO, BOMBA_PIN, HIGH);
}

void Apaga_Bomba() {
	// Deshabilita el relevador de la bomba
    GPIOWriteBit(BOMBA_GPIO, BOMBA_PIN, LOW);
}


void WriteDisplayData(uint8_t data) {

    uint8_t display_data;
    // obtiene los bits para el primer puerto del display
    display_data = ((data & 1) << 3) | ((data & 2) << 1) | ((data & 4) >>1);
    // Envía los bits al primer puerto del display
    GPIOWrite(DISPLAY_1_GPIO, display_data);
    // Obtiene los bits para el segundo puerto del display
    display_data = ((data & 8) << 4) | ((data & 16) << 2) | (data & 32) | ((data & 64) >>2);
    // Envía los bits al segundo puerto del display
    GPIOWrite(DISPLAY_2_GPIO, display_data);
}

void MuestraNumeroDisplays(unsigned char numero) {

    uint8_t dec, uni;

    // Calcula decenas
    dec = numero / 10;
    // Calcula unidades
    uni = numero % 10;

    // Obtiene la decodificación de las decenas para el display
    dec = display_codes[dec];	
    // Obtiene la decodificación de las unidaes para el display
    uni = display_codes[uni];

    // Envía las decenas al primer display
    WriteDisplayData(dec);
    // Enciende el primer display
    GPIOWriteBit(DISPLAY_1_EN_GPIO, DISPLAY_1_EN, HIGH);
    // Espera un tiempo para visualizar el display
    delay(0xfff);
    // Apaga el primer display
    GPIOWriteBit(DISPLAY_1_EN_GPIO, DISPLAY_1_EN, LOW);

    // Envía las unidades al segundo display
    WriteDisplayData(uni);
    // Enciende el segundo display
    GPIOWriteBit(DISPLAY_2_EN_GPIO, DISPLAY_2_EN, HIGH);
    // Espera un tiempo para visualizar el display
    delay(0xfff);
    // Apaga el segundo display
    GPIOWriteBit(DISPLAY_2_EN_GPIO, DISPLAY_2_EN, LOW);
}

void delay(uint32_t count){

    uint32_t i;

    for (i = 0; i < count; ++i)
        NOP();
}

void LED_Init() {
	// Pin del led configurado como salida Open Drain (activo en bajo)
    GPIOInit(LED_BOARD_GPIO, LED_BOARD, GPIO_MODE_OUT_OD);
    GPIOWriteBit(LED_BOARD_GPIO, LED_BOARD, HIGH);
}

void TIM2_Init(){
	// Habilita el reloj de periferico TIM2
    SetBit(CLK_PCKENR1, CLK_PCKENR1_TIM2);

    // TIM2 a 3906.25 HZ, contando de 0 a 4096
    TIM2_CR1 = 0;
    TIM2_PSCR = TIM2_PRESCALER_4096;
    TIM2_ARRH = 0x0F;
    TIM2_ARRL = 0x2F;
	SetBit(TIM2_IER, TIM2_IER_UIE);
}

void BombaRelay_Init() {

	// Configura el pin del relevador de la bomba como
	// salida Push Pull
    GPIOWriteBit(BOMBA_GPIO, BOMBA_PIN, LOW);
    GPIOInit(BOMBA_GPIO, BOMBA_PIN, GPIO_MODE_OUT_PP);

}

void Displays_Init() {

    // Pines de datos del display como salida
    GPIOInit(DISPLAY_1_GPIO, DISPLAY_1_MASK, GPIO_MODE_OUT_PP);
    GPIOInit(DISPLAY_2_GPIO, DISPLAY_2_MASK, GPIO_MODE_OUT_PP);
    
    // Pines de habilitación de los displays como salida
    GPIOInit(DISPLAY_1_EN_GPIO, DISPLAY_1_EN, GPIO_MODE_OUT_PP);
    GPIOInit(DISPLAY_2_EN_GPIO, DISPLAY_2_EN, GPIO_MODE_OUT_PP);
    
    // Limpia los displays
    GPIOWriteBit(DISPLAY_1_GPIO, DISPLAY_1_MASK, LOW);
    GPIOWriteBit(DISPLAY_2_GPIO, DISPLAY_2_MASK, LOW);

    // Apaga los displays
    GPIOWriteBit(DISPLAY_1_EN_GPIO, DISPLAY_1_EN, LOW);
    GPIOWriteBit(DISPLAY_2_EN_GPIO, DISPLAY_2_EN, LOW);

}

void Botones_Init() {

    // Configura puerto de los botones como salida
    GPIOInit(BOTONES_GPIO, BOTONES_MASK, GPIO_MODE_IN_PU);

    // Habilita la interrupción externa en los botones
    GPIOEnableExtInt(BOTONES_GPIO, BOTONES_MASK);

    // Configura interrupción en flanco de bajada.
    ITCSenseGPIOD(ITC_SENSE_FALLING); 

}




