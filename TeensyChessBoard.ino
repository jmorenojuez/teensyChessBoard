/** 
 *  USB chess board that uses Teensy 2.0.
 */
#define DEBUG           1           // Debug flag
#define NUM_ROWS  8
#define NUM_COLS  8
// Pines para los leds
#define PLAYER_LED_PIN    9
#define OPONENT_LED_PIN   10
// Pines para los botones
#define NEWGAME_PIN       20
#define REINSTATE_PIN     21
// Macro para calcular el número de elementos de un array
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

// Rutinas de depuración: Usa la macro F() para alojar las cadenas en meomoria de 
// program y no en la RAM.
#if DEBUG == 1
#define dprint(expression) Serial.print(F("# ")); Serial.print( #expression ); Serial.print( F(": ") ); Serial.println( expression )
#define dshow(expression) Serial.println( F(expression) )
#else
#define dprint(expression)
#define dshow(expression)
#endif



///////////// VARIABLES GLOBALES ///////////////
bool bComputerOponent = false;
bool bSendEnter = true;         // Enviamos ENTER después de cada movimiento?
bool bPlayAsWhite = true;       // El jugador humano juega con las piezas blancas
bool bPlayerHasTurn = true      // Indica si el juegador humano tiene el turno.
bool bWhiteCanCastle = true;    // Indica si el jugador blanco puede enrocar
bool bBlackCanCastle = true;    // Indica si el jugador negro puede enrocar
bool bPromoteQeenAlways = true; // Indica si las proocioens de peones son reinas autm.

// Pines para cada una de las filas del tablero
const byte rowPins[NUM_ROWS] = {5,6,7,8,22,23,11,12}; 
// Pines para cada una de las columnas del tablero
const byte colPins[NUM_COLS] = {0,1,2,3,13,14,15,4}; 

/* Matriz de booleanos que representa el tablero de ajedrez. true si la casilla contiene pieza. false en otro caso.
Notese que el tablero se representa de la siguiente manera:

-----------------------------------------------------------------
|       |       |       |       |       |       |       | (7,7) |
|       |       |       |       |       |       |       |  H8   |
-----------------------------------------------------------------
|       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |
-----------------------------------------------------------------
|       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |
-----------------------------------------------------------------
|       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |
-----------------------------------------------------------------
|       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |
-----------------------------------------------------------------
|       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |
-----------------------------------------------------------------
|       |       |       |       |       |       |       |       |
|       |       |       |       |       |       |       |       |
-----------------------------------------------------------------
| (0,0) |       |       |       |       |       |       |       |
| A1    |       |       |       |       |       |       |       |
-----------------------------------------------------------------
*/
byte chessBoard[NUM_COLS];

void InitChessBoard(){
    for (int i=0;i<NUM_COLS;i++) chessBoard[i] = 0;
}

// Tipo de datos que define una casilla del tablero 
struct chess_square {
    unsigned int row;
    unsigned int col;
};

void InitChessSquare(chess_square* square) {
    square->row = 8;
    square->col = 8;
}

// Tipo de datos que define un movimento de una pieza.
struct chess_movement{
    chess_square from;
    chess_square to;
};

void InitChessMovement(chess_movement* movement){
    InitChessSquare(&movement->from);
    InitChessSquare(&movement->to);
}

// Variable que almacena el movimiento actual
chess_movement current_movement;

void initglobalVars() {
    bComputerOponent = true;
    bSendEnter = true;
    bPlayAsWhite = true;
    bPlayerHasTurn = true;
    InitChessBoard(); // Inicializamos el tablero
    // Inicializamos el movimiento actual
    InitChessMovement(&current_movement);∫
    sampleBoard();
}

////////////////////////////////////////////////

/* Metodo que se ejecuta una única vez e inicializa las variables y pines
de la tarjeta */
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
    
    initglobalVars();

}
/* Metodo que devuelve la diferencias entre dos tableros en forma de matriz de bits
siendo retval[fila][columna] = 0 si no hay diferencia y retval[fila][columna] = 1 si hay diferencia */
chess_square getBoardDifference(byte[] board1,byte[] board2){
    chess_square retval;
    retval.row = 8;
    retval.col = 8;

    for (int i=0;i<NUM_ROWS;i++)
        {
            // 
            byte b1 = board1[i]; // Fila i del primer tablero
            byte b2 = board2[i]; // Fila i del segundo tablero
            byte diff = b1 ^ b2; // Operación XOR que nos devuelve la diferencia
            if (diff != 0) {
                // Hallamos el bit más significativo a uno
                // http://stackoverflow.com/questions/7587027/how-to-find-most-significant-bit-msb
                retval.col = (diff & 0xff) >> 7;
                retval.row = i;
                return retval;
            }
        }   
    return retval;
}

// Funcion que calcula cuantos 1s hay en un byte
// http://stackoverflow.com/questions/9949935/calculate-number-of-bits-set-in-byte
unsigned int count_bit(byte x)
{
  static const byte NIBBLE_LOOKUP [16] =
  {
    0, 1, 1, 2, 1, 2, 2, 3, 
    1, 2, 2, 3, 2, 3, 3, 4
  };

  return NIBBLE_LOOKUP[byte & 0x0F] + NIBBLE_LOOKUP[byte >> 4];
}

// Metodo que devuelve el número de diferencias entre dos tableros de ajedrez
unsigned int getDifferenceNumber(byte[] differenceBoard) 
{
    unsigned int retval = 0;
    for (int i=0;i<NELEMS(differenceBoard);i++)
        retval += count_bit(differenceBoard[i]);
    return retval;
}

// Metodo que muestrea el tablero de ajedrez magnético 
void sampleBoard()
{
    dshow("sampleBoard --> Inicio");
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
    unsigned int numDiferences = getDifferenceNumber(currentChessBoard);
    if (numDiferences == 0) {
        dshow("sampleBoard --> Los tableros son iguales...");
        return;
    }
    if (numDiferences > 1) {
        dshow("sampleBoard --> No debería haber más de un movimento de piezas a la vez");
        return;
    }
    chess_square diffSquare = getBoardDifference(chessBoard,currentChessBoard);
    if (current_movement.from.row == 8) {
        // Caso 1. No estaba inicilizada la casilla de origen del movimiento,
        // Por lo que se corresponde con el inicio del movimiento
        dshow("Inicializamos el origen del movimiento");
        current_movement.from = diffSquare;
    } else if (hasPiece(diffSquare,chessBoard)) {
        // Caso 2. La casilla de origen ya está establecida y había una pieza en la casilla. Debe correspenderse a una captura de pieza.
    } else {
        dshow("Inicializamos el destino del movimiento");
        current_movement.to = diffSquare;
    }

    if (isSetMovement(current_movement)) {
        char[4] strMovement = translatMovement(current_movement);
        dprint(strMovement);
        if (bSendEnter) // Si hay que enviar ENTER al final del movime¡iento
            Keyboard.println(strMovement);
        else 
            Keyboard.print(strMovement);
        // Limpiamos el movimiento enviado
        InitChessMovement(&current_movement);
    }
    // Actualizamos el tablero actual
    updateChessBoard(currentChessBoard);
    dshow("sampleBoard --> FIn del procedimiento");
}
// Funcion que convierte una estructura de tipe chess_movement a una cadena del tipo
// "e2e4"
char[4] translatMovement(const chess_movement movement){
    char[4] retval;
    if (bPlayAsWhite) {
        retval[0] = 'a' + movement.from.col;
        retval[1] = '1' + movement.from.row;
        retval[2] = 'a' + movement.to.col;
        retval[3] = '1' + movement.to.row;
    } else {
        retval[0] = 'a' + movement.from.col;
        retval[1] = '8' - movement.from.row;
        retval[2] = 'a' + movement.to.col;
        retval[3] = '8' - movement.to.row;
    }
    return retval;
}

// Procedimiento que actualiza el tablero actual de ajedrez
void updateChessBoard(byte[] newChessBoard){
   memcpy(chessBoard,newChessBoard,sizeof(newChessBoard)); 
}

// Funcion que comprueba si un movimiento de ajedrez ha sido completamente inicializado
bool isSetMovement(chess_movement movement) {
    return movement.from.row != 8 && movement.from.col != 8 &&
           movement.to.row != 8 && movement.to.col != 8;
}

// Funcion que calcula si hay pieza en una casilla
bool hasPiece(chess_square square, byte[] chessBoard){
    byte b = chessBoard[square.col];
    return (b & ( 1 << square.row)); // http://stackoverflow.com/questions/8920840/a-function-to-check-if-the-nth-bit-is-set-in-a-byte
}

void loop() {
    sampleBoard();
    delay(1000);
}
