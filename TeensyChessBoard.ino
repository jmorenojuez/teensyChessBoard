/** 
 *  USB chess board that uses Teensy 2.0.
 */

#define DEBUG           1           // Debug flag
#define SAMPLE_DELAY    1000        // Constante que define el retardo en el muestreo del tablero en milisegundos
#define NUM_ROWS  8
#define NUM_COLS  8
// Pines para los leds
#define PLAYER_LED_PIN    9
#define OPONENT_LED_PIN   10
// Pines para los botones
#define BUT_NEWGAME_PIN       20
#define BUT_REINSTATE_PIN     21
#define BUT_FLIP_BOARD        40 // FIXME:
#define BUT_CHANGE_OPONENT    41 // FIXME:

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
bool bComputerOponent = true;
bool bSendEnter = true;         // Enviamos ENTER después de cada movimiento?
bool bPlayAsWhite = true;       // El jugador humano juega con las piezas blancas
bool bPlayerHasTurn = true;     // Indica si el juegador humano tiene el turno.
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
struct CHESS_SQUARE {
    unsigned int row;
    unsigned int col;
    
    // Operacion de comparación.
    bool operator==(const CHESS_SQUARE& a) const
    {
        return (row == a.row && col == a.col);
    }
} ;

// Tipo de datos que defino los distinto movimentos
typedef enum MOVEMENT_TYPE{
  NOT_SET,  // Movimiento no establecido todavia
  SET_ORIGIN, // Sólo sabemos la casilla de origen
  SIMPLE, // Movimiento de pieza simple, sin captura
  CAPTURE_BEGIN, // Movimiento que captura pieza
  CAPTURE_END, // Movimiento que captura pieza
  EN_PASSANT, // Captura al paso
  CASTLE_SHORT, // Enroque corto
  CASTLE_LONG, // Enroque largo
  PROMOTION // Peon promocionado
};
// Tipo de datos que define un movimento de una pieza.
struct  CHESS_MOVEMENT{
    CHESS_SQUARE from;
    CHESS_SQUARE to;
    MOVEMENT_TYPE type;
};

// Variable que almacena el movimiento actual
CHESS_MOVEMENT current_movement;

// Declaración de funciones
void InitChessSquare(CHESS_SQUARE* square);
void InitChessMovement(CHESS_MOVEMENT* movement);
CHESS_SQUARE getBoardDifference(byte board1[],byte board2[]);
void sampleBoard(bool bInitBoard = false);
void initglobalVars();
unsigned int getDifferenceNumber(byte differenceBoard[]);
void updateChessBoard(byte newChessBoard[]);
bool isSetMovement(CHESS_MOVEMENT movement);
bool isValidSquare(CHESS_SQUARE square);
bool hasPiece(CHESS_SQUARE square, byte chessBoard[]);
String translatMovement(const CHESS_MOVEMENT movement);
unsigned int count_bit(byte x);
void changeTurn();
//////////////////////////////////

void initglobalVars() {
    bComputerOponent = true;
    bSendEnter = true;
    bPlayAsWhite = true;
    bPlayerHasTurn = true;
    bWhiteCanCastle = true;
    bBlackCanCastle = true;
    InitChessBoard(); // Inicializamos el tablero
    // Inicializamos el movimiento actual
    InitChessMovement(&current_movement);
    sampleBoard(true);
}

// Método que inicializa una casilla de ajedrez
void InitChessSquare(CHESS_SQUARE* square) {
    square->row = 8;
    square->col = 8;
    return;
}

// Método que inicializa un movimiento de ajedrez
void InitChessMovement(CHESS_MOVEMENT* movement){
    InitChessSquare(&movement->from);
    InitChessSquare(&movement->to);
    movement->type = NOT_SET;
}

////////////////////////////////////////////////

/* Metodo que devuelve la diferencias entre dos tableros en forma de matriz de bits
siendo retval[fila][columna] = 0 si no hay diferencia y retval[fila][columna] = 1 si hay diferencia */
CHESS_SQUARE getBoardDifference(byte board1[],byte board2[]){
    CHESS_SQUARE retval;
    retval.row = 8;
    retval.col = 8;

    for (int i=0;i<NUM_ROWS;i++){
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
unsigned int count_bit(byte x){
  static const byte NIBBLE_LOOKUP [16] =
  {
    0, 1, 1, 2, 1, 2, 2, 3, 
    1, 2, 2, 3, 2, 3, 3, 4
  };

  return NIBBLE_LOOKUP[x & 0x0F] + NIBBLE_LOOKUP[x >> 4];
}

void changeTurn(){
    bPlayerHasTurn = !bPlayerHasTurn;
}

// Metodo que devuelve el número de diferencias entre dos tableros de ajedrez
unsigned int getDifferenceNumber(byte differenceBoard[]){
    unsigned int retval = 0;
    for (int i=0;i<NELEMS(differenceBoard);i++)
        retval += count_bit(differenceBoard[i]);
    return retval;
}

// Metodo que muestrea el tablero de ajedrez magnético 
void sampleBoard(bool bInitBoard) {    
    dshow("sampleBoard --> Inicio");
    byte currentChessBoard[NUM_COLS]; // Variable que almacena el estado actual del tablero
    for (int i=0;i<NUM_COLS;i++)
        currentChessBoard[i] = 0;

    for (int row=0;row<NUM_ROWS;row++)
    {
        digitalWrite(rowPins[row], LOW); // Ponemos a cero una fila entera
        for (int column=0;column<NUM_COLS;column++)
        {
            if (digitalRead(colPins[column] == LOW)) {
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
    if (!bInitBoard) { // Si no es la primera llamada al procedimiento
        unsigned int numDiferences = getDifferenceNumber(currentChessBoard);
        if (numDiferences == 0) { // 1. Si son iguales, hemos acabado 
            dshow("sampleBoard --> Los tableros son iguales...");
            return;
        }
        if (numDiferences > 1) {
            dshow("sampleBoard --> No debería haber más de un movimento de piezas a la vez");
            return;
        }
        CHESS_SQUARE diffSquare = getBoardDifference(chessBoard,currentChessBoard);
        switch(current_movement.type) {
            case NOT_SET:
                // Caso 1. No estaba inicilizada la casilla de origen del movimiento,
                // Por lo que se corresponde con el inicio del movimiento
                dshow("Inicializamos el origen del movimiento");
                current_movement.from = diffSquare;
                current_movement.type = SET_ORIGIN;
                break;
            case SET_ORIGIN:
                current_movement.to = diffSquare;
                // Caso 2. La casilla de origen ya está establecida y había una pieza en la casilla. Debe corresponderse a una captura de pieza.
                if (hasPiece(diffSquare,chessBoard)) {
                    current_movement.type = CAPTURE_BEGIN;
                } else { // NO hay pieza en la casilla de destino
                    current_movement.type = SIMPLE;     
                }
                break;
            case CAPTURE_BEGIN:
                if (diffSquare == current_movement.to) {
                    current_movement.type = CAPTURE_END;
                } else {
                    dshow("Error en la captura de pieza");
                }
                break;
            case SIMPLE:
            case CAPTURE_END:
                break;
        }
        // TODO: Tratar el enroque, captura al paso y coronación de pieza
        if (current_movement.type == SIMPLE || current_movement.type == CAPTURE_END) {
            String strMovement = translatMovement(current_movement);
            dprint(strMovement);
            if (bPlayerHasTurn) 
                (bSendEnter) ? 
                    Keyboard.println(strMovement) : 
                    Keyboard.print(strMovement);
            else if (!bComputerOponent) // Juegan dos humanos? 
                (bSendEnter) ? 
                    Keyboard.println(strMovement) : 
                    Keyboard.print(strMovement);
            // Limpiamos el movimiento enviado
            InitChessMovement(&current_movement);
            // Cambiamos el turno de juego
            changeTurn();
        }
    }
    // Actualizamos el tablero actual
    updateChessBoard(currentChessBoard);
    dshow("sampleBoard --> Fin del procedimiento");
}

// Funcion que convierte una estructura de tipe CHESS_MOVEMENT a una cadena del tipo
// "e2e4"
String translatMovement(const CHESS_MOVEMENT movement){
    String retval = "";
    if (bPlayAsWhite) {
        retval += 'a' + movement.from.col;
        retval += '1' + movement.from.row;
        retval += 'a' + movement.to.col;
        retval += '1' + movement.to.row;
    } else {
        retval += 'a' + movement.from.col;
        retval += '8' - movement.from.row;
        retval += 'a' + movement.to.col;
        retval += '8' - movement.to.row;
    }
    return retval;
}
// Procedimiento que actualiza el tablero actual de ajedrez
void updateChessBoard(byte newChessBoard[]){
   memcpy(chessBoard,newChessBoard,sizeof(newChessBoard)); 
}

// Funcion que comprueba si un movimiento de ajedrez ha sido completamente inicializado
bool isSetMovement(CHESS_MOVEMENT movement) {
    return isValidSquare(movement.from) && 
           isValidSquare(movement.to) && 
           movement.type != NOT_SET;
}

bool isValidSquare(CHESS_SQUARE square) {
    return (square.row >= 0 && square.row < NUM_ROWS) && 
            (square.col >= 0 && square.col < NUM_COLS);
}

// Funcion que calcula si hay pieza en una casilla
bool hasPiece(CHESS_SQUARE square, byte chessBoard[]){
    byte b = chessBoard[square.col];
    return (b & ( 1 << square.row)); // http://stackoverflow.com/questions/8920840/a-function-to-check-if-the-nth-bit-is-set-in-a-byte
}

/* TODO:
RE-INSTATE: sometimes SolusChess program can "be lost": because we have made a mistake moving a piece, or we have accidentally dropped it, or the PC program doesn't understand a move, etc. By acting on this pushbutton the program performs a full scan of the board to check where there are pieces and where are not, and in addition it changes the player turn. The latter is important because I have found that many errors are caused by desynchronization of the player turn when we're playing against the computer. If you don't want to change the turn, you can push a second time and solved, ie, by pressing an even number of times. The change in the turn caused by the use of this control, should not be used to "Flip the Board".

Internally the program makes the following changes:
Removes uncompleted moves
Resets the detection of castling
Rescan the chessboard
Swaps the game turn
*/
void reinstateGame() {

}

// Método que cambia el sentido del tablero de ajedrez
void flipBoard() {
    bPlayAsWhite = !bPlayAsWhite;
}

/* Metodo que se ejecuta una única vez e inicializa las variables y pines
de la tarjeta */
void setup() {
    // Ponemos los pines de los botones como entrada
    pinMode(BUT_CHANGE_OPONENT,INPUT);
    pinMode(BUT_FLIP_BOARD,INPUT);
    pinMode(BUT_CHANGE_OPONENT,INPUT);
    pinMode(BUT_NEWGAME_PIN,INPUT);
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

void loop() {
    // Leemos el estado de los botones
    if (digitalRead(BUT_NEWGAME_PIN) == HIGH){
        dshow("Pulsado el botón de NUEVO JUEGO");
        initglobalVars();
    }
    if (digitalRead(BUT_REINSTATE_PIN) == HIGH) {
        dshow("Pulsado el botón de REINTANCIAR");
        reinstateGame();
    }
    if (digitalRead(BUT_FLIP_BOARD) == HIGH) {
        dshow("Pulsado el botón de DAR LA VUELTA AL TABLERO");
        flipBoard();
    }
    if (digitalRead(BUT_CHANGE_OPONENT) == HIGH) {
        dshow("Pulsado el botón de cambio de oponente");
        bComputerOponent = !bComputerOponent;
    }
    // Si el movimento no ha sido iniciado encedemo sólo el led del jugador que tiene el turno
    if (current_movement.type == NOT_SET) {
        digitalWrite(PLAYER_LED_PIN,(bPlayerHasTurn ? HIGH : LOW));
        digitalWrite(OPONENT_LED_PIN,(bPlayerHasTurn ? LOW : HIGH));
    } else { // En caso contrario hacemos parpadear los leds
        digitalWrite(PLAYER_LED_PIN,(digitalRead(PLAYER_LED_PIN) == LOW ? HIGH : LOW));
        digitalWrite(OPONENT_LED_PIN,(digitalRead(OPONENT_LED_PIN) == HIGH ? LOW : HIGH));
    }
    sampleBoard();
    // Nos echamos una pequeña siesta
    delay(SAMPLE_DELAY);
}
