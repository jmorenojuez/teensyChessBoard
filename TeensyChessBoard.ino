/** 
 *  USB chess board that uses Teensy 2.0.
 */
#include <Keypad.h>

#define NUM_ROWS  8
#define NUM_COLS  8

// Pines para los leds
#define PLAYER_LED_PIN    9
#define OPONENT_LED_PIN   10
// Pines para los botones
#define NEWGAME_PIN       20
#define REINSTATE_PIN     21

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
// Pins para cada una de las filas del tablero
const byte rowPins[NUM_ROWS] = {5,6,7,8,22,23,11,12}; 
// Pins para cada una de las columnas del tablero
const byte colPins[NUM_COLS] = {0,1,2,3,13,14,15,4}; 

// Matriz de booleanos que representa el tablero de ajedrez. true si la casilla contiene pieza. false en otro caso.
byte chessBoard[NUM_COLS];

void setup() {
    // Se ejecuta una única vez
    pinMode(PLAYER_LED_PIN, OUTPUT);
    pinMode(OPONENT_LED_PIN, OUTPUT);

    // Establecemos los pines para las filas como salida
    for (int i=0;i<NUM_ROWS;i++){
        pinMode(rowPins[i], OUTPUT);
        digitalWrite(rowPins[i], HIGH);
    }
    // Establecemos los pines de las columnas como entrada
    for (int i=0;i<NUM_COLS;i++)
        pinMode(colPins[i], INPUT);

    // Inicializamos la matriz del tablero para que no contenga ninguna pieza
    for (int i=0;i<NUM_COLS;i++)
            chessBoard[i] = 0b00000000;


/* Metodo que devuelve la diferencias entre dos tableros en forma de matriz de bits
siendo retval[fila][columna] = 0 si no hay diferencia y retval[fila][columna] = 1 si hay diferencia */
byte[] getBoardDifferences(byte[] board1,byte[] board2){
    byte retval[NUM_COLS];
    for (int i=0;i<NUM_COLS;i++) {
        // Usamos el operador XOR para calcular la diferencia
        retval[i] = board1[i] ^ board2[i];
    }
    return retval;
}

// http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
unsigned int count_bit(byte x)
{
  x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
  x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
  x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
  x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
  x = (x & 0x0000FFFF) + ((x >> 16)& 0x0000FFFF);
  return x;
}

unsigned int getDifferenceNumber(byte[] differenceBoard) {
    unsigned int retval = 0;
    for (int i=0;i<NELEMS(differenceBoard);i++)
        retval += count_bit(differenceBoard[i]);
    return retval;
}

void sampleBoard(){

    byte currentChessBoard[NUM_COLS]; // Variable que almacena el estado actual del tablero
    for (int i=0;i<NUM_COLS;i++)
            currentChessBoard[i] = 0b00000000;
    
    for (int row=0;row<NUM_ROWS;row++)
    {
        digitalWrite(rowPins[row], LOW); // Ponemos a cero una fila entera
        for (int column=0;column<NUM_COLS;column++)
        {
            if (digitalRead(colPin[column] == LOW)) {
                // Hay una pieza en la casilla [row,column]
                // http://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit-in-c-c
                currentChessBoard[column] |= 1 << row; 
            } else {
                // No hay pieza en la casilla [row,column]
                currentChessBoard[column] &= ~(1 << row);
            }
        }
        digitalWrite(rowPins[row], HIGH); // Dejamos como estaba la fila 
    }
    // Ahora tenemos en currentChessBoard el estado actual del tablero
    // Vamos a compararlo con el estado anterior
    // 1. Si son iguales, hemos acabado
    if (memcmp(chessBoard,currentChessBoard,sizeof(chessBoard) == 0)) {
        return;
    }

    byte differences[NUM_COLS] = getBoardDifferences(chessBoard,currentChessBoard);
    // Ahora en el array differences tenemos las casillas que han cambiado
    // Caso 1. Solo hay una diferencia --> En principio la pieza se está moviendo, por lo que tenemos un movimiento parcial
    // Caso 2. Tenemos dos diferencias --> 


}

void loop() {
    sampleBoard();
    delay(1000);
}
