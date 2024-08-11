/*
 * HOMEWORK:
 * - Try to find a worlde clone online that mimics wordle's
 *      duplicate behavior
 * - What is this hashmap thing chat won't shut up about ;)
 * - What is an enum
 *
 * NOW:
 * Nicer UI
 * #define's for virtual terminal sequences
 * Consolidate color printing functions (pass number of chars?)
 *
 * NEXT STREAM:
 *
 * FUTURE:
 */

#define WIN32_LEAN_AND_MEAN

#pragma comment( lib, "bcrypt" )

#pragma once
#include "Chardle.h"
#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <bcrypt.h>
#include <conio.h>

#define NOT     !
#define TRUE    1
#define FALSE   0

#define ESCAPE_KEY      27
#define NUM_GUESSES     6

#define COLOR_NORMAL    0
#define COLOR_GREEN     32
#define COLOR_YELLOW    33
#define COLOR_GRAY      1000

int validateInput(char* input);
int checkInputAgainstAnswer(
    char* input,
    const char* answer,
    int* numGuesses
);
void printCharInColor(char text, int color);
void printStringInColor(char* text, int color);
int isLetterInAnswer(
    char letter, 
    char* tempAnswer, 
    int* colors
);
void clearScreen(void);
void printAlphabet(char letter, int color, int print);
int endGame(int won, char* answer, int numAnswers);
int binarySearch(const char* guess, int start, int end);
char* getRandomAnswer(int numAnswers);
int getInput(char* buffer);

typedef enum
{
    MOVE_UP, ERASE, CLEAR_SCREEN, CLEAR_LINE, SAVE, RESTORE,
    MOVE_FROM_TOP
} Action;
void doCursorAction(Action action, int numTimes);

// RESEARCH: Is it bad that these are global?
char dictValid[NUM_VALID_WORDS][WORD_LENGTH + 1];
char dictAnswers[NUM_ANSWERS][WORD_LENGTH + 1];

int main()
{
    // Set output mode to handle virtual terminal sequences
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    assert(hOut != INVALID_HANDLE_VALUE);
    DWORD consoleMode = 0;
    BOOL result = GetConsoleMode(
        hOut,
        &consoleMode
    );
    assert(result);
    DWORD originalConsoleMode = consoleMode;
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    result = SetConsoleMode(
        hOut,
        consoleMode
    );
    assert(result);

    // Main game loop
    int numCorrectLetters = 0;
    int gameStarted = FALSE;
    char* answer = 0;

    int numAnswers = NUM_ANSWERS;
    int numGuesses = 0;

    while (1)
    {
        if (NOT gameStarted)
        {
            int padding = 4;
            for (int newline = 0;
                 newline < (NUM_GUESSES + padding);
                 newline++)
            {
                printf("\n");
            }

            answer = getRandomAnswer(numAnswers);
            printf(
                "Guess a %d-letter word, or press ESC to quit: ",
                WORD_LENGTH
            );
            gameStarted = TRUE;
        }

        printAlphabet(
            0,
            0,
            TRUE
        );

        char buffer[WORD_LENGTH + 1] = { 0 };
        int shouldContinue = getInput(buffer);

        // If the user has pressed escape, 
        //  quit out immediately
        if (NOT shouldContinue)
        {
            break;
        }

        // Erase user's current guess
        doCursorAction(ERASE, WORD_LENGTH);

        int inputIsValid = validateInput(buffer);
        if (inputIsValid)
        {
            numCorrectLetters =
                checkInputAgainstAnswer(
                    buffer,
                    answer,
                    &numGuesses
                );
        }

        // If an end condition has been met
        if (numCorrectLetters == WORD_LENGTH ||
            numGuesses == NUM_GUESSES)
        {
            int won = (numCorrectLetters == WORD_LENGTH);
            int shouldEnd = endGame(
                won,
                answer,
                numAnswers
            );
            if (shouldEnd)
            {
                break;
            }

            // Reset the game if the user wants to play again
            numCorrectLetters = 0;
            numGuesses = 0;
            gameStarted = FALSE;
        }
    }

    // Reset console to prior settings before exiting
    result = SetConsoleMode(
        hOut,
        originalConsoleMode
    );
    assert(result);
}

int getInput(char* buffer)
{
    int numEnteredChars = 0;
    while (1)
    {
        char input = _getch();
        if (input == '\b')
        {
            if (numEnteredChars > 0)
            {
                doCursorAction(ERASE, 1);
                numEnteredChars--;
            }
        }
        else if (input >= 'a' &&
                 input <= 'z' &&
                 numEnteredChars < WORD_LENGTH)
        {
            printf(
                "%c",
                input
            );
            buffer[numEnteredChars] = input;
            numEnteredChars++;
        }
        else if (input == ESCAPE_KEY)
        {
            buffer[WORD_LENGTH] = 0;
            return FALSE;
        }
        else if (input == '\r' &&
                 numEnteredChars == WORD_LENGTH)
        {
            buffer[WORD_LENGTH] = 0;
            return TRUE;
        }
    }
}

void doCursorAction(Action action, int numTimes)
{
    switch (action)
    {
        case MOVE_UP:
        {
            printf(
                "\x1b[%dF",
                numTimes
            );
            break;
        }
        case MOVE_FROM_TOP:
        {
            printf(
                "\x1b[%d;%dH",
                numTimes,
                0
            );
            break;
        }
        case ERASE:
        {
            printf(
                "\x1b[%dD\x1b[%dX",
                numTimes,
                numTimes
            );
            break;
        }
        case CLEAR_LINE:
        {
            printf("\x1b[2K");
            break;
        }
        case CLEAR_SCREEN:
        {
            printf("\x1b[2J\x1b[3J\x1b[0;0H");
            break;
        }
        case SAVE:
        {
            printf(
                "\x1b%d",
                7
            );
            break;
        }
        case RESTORE:
        {
            printf(
                "\x1b%d",
                8
            );
            break;
        }
    }
}

char* getRandomAnswer(int numAnswers)
{
    // Get random word
    unsigned int randomNumber = 0;
    NTSTATUS status = BCryptGenRandom(
        NULL,
        (unsigned char*) &randomNumber,
        sizeof(unsigned int),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    assert(status == 0);
    randomNumber %= numAnswers;
    char* answer = dictAnswers[randomNumber];
    OutputDebugStringA(answer);
    return answer;
}

int binarySearch(const char* guess, int start, int end)
{
    // word isn't in the array
    if (start > end)
    {
        return -1;
    }
    int mid = end + (start - end) / 2;
    char* midWord = dictValid[mid];

    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        if (guess[index] < midWord[index])
        {
            return binarySearch(
                guess,
                start,
                mid - 1
            );
        }
        else if (guess[index] > midWord[index])
        {
            return binarySearch(
                guess,
                mid + 1,
                end
            );
        }
    }
    return mid;
}

int endGame(int won, char* answer, int numAnswers)
{
    // Print game end message
    doCursorAction(MOVE_UP, 1);
    if (won)
    {
        printStringInColor(
            "Congratulations, you've won!\n",
            COLOR_GREEN
        );
    }
    else
    {
        printf(
            "Sorry, the word was %s.\n",
            answer
        );
    }

    // If they have exhausted the entire answer dictionary,
    //      quit out
    if (numAnswers == 1)
    {
        printStringInColor(
            "\nYou have guessed all of the words in the game! "
            "Thanks for playing!\n",
            COLOR_GREEN
        );
        return TRUE;
    }

    // Ask the user if they would like to play again
    printf(
        "Press any key to play again, or ESC to quit..."
    );
    char input = _getch();
    if (input == ESCAPE_KEY)
    {
        return TRUE;
    }

    // If they want to continue, reset the game
    for (char letter = 'a';
         letter <= 'z';
         letter++)
    {
        printAlphabet(
            letter,
            COLOR_NORMAL,
            FALSE
        );
    }

    // Move remaining words up in the array
    int numCharsPerElement = WORD_LENGTH + 1;
    int answerIndex = (int)
        ((answer - dictAnswers[0]) /
         numCharsPerElement);
    int numAnswersLeft = numAnswers - answerIndex;
    int numCharsLeft =
        numAnswersLeft * numCharsPerElement;

    // Remove the used answer from the array of answers
    for (int index = answerIndex + 1;
         index < numAnswers;
         index++)
    {
        for (int letter = 0;
             letter < WORD_LENGTH;
             letter++)
        {
            dictAnswers[index - 1][letter] =
                dictAnswers[index][letter];
        }
    }
    numAnswers--;

    // For debugging purposes, clear the rest of the array
    for (int index = numAnswers;
         index < NUM_ANSWERS;
         index++)
    {
        for (int letter = 0;
             letter < WORD_LENGTH;
             letter++)
        {
            dictAnswers[index][letter] = 0;
        }
    }
    doCursorAction(CLEAR_SCREEN, 0);
    return FALSE;
}

// If they pass 0 as the letter parameter, skip updates
void printAlphabet(char letter, int color, int print)
{
    static char alphabet[26] = { 0 };
    static int colors[26] = { 0 };

    // Initialize alphabet if necessary
    if (alphabet[0] == 0)
    {
        for (char letter = 'a';
             letter <= 'z';
             letter++)
        {
            alphabet[letter - 'a'] = letter;
        }
    }

    // Update letter color if desired
    if (letter >= 'a' &&
        letter <= 'z')
    {
        colors[letter - 'a'] = color;
    }

    if (print)
    {
        // Save cursor position
        doCursorAction(SAVE, 0);

        // Move cursor up n lines
        doCursorAction(MOVE_UP, 3);

        // Print array
        for (int index = 0;
             index < 26;
             index++)
        {
            printCharInColor(
                alphabet[index],
                colors[index]
            );
            printf(" ");
        }

        // Restore cursor position
        doCursorAction(RESTORE, 0);

    }
}

void printStringInColor(char* text, int color)
{
    // We have to use custom RGB to get grey
    if (color == COLOR_GRAY)
    {
        printf("\x1b[38;2;103;103;103m%s",
               text);
    }

    // Otherwise, we can use the system color codes
    else
    {
        printf(
            "\x1b[%dm%s",
            color,
            text
        );
    }

    printf(
        "\x1b[%dm",
        COLOR_NORMAL
    );
}

void printCharInColor(char text, int color)
{
    // We have to use custom RGB to get grey
    if (color == COLOR_GRAY)
    {
        printf("\x1b[38;2;103;103;103m%c",
               text);
    }

    // Otherwise, we can use the system color codes
    else
    {
        printf(
            "\x1b[%dm%c",
            color,
            text
        );
    }

    printf(
        "\x1b[%dm",
        COLOR_NORMAL
    );
}

// TODO: This could certainly be refactored into something nicer
int checkInputAgainstAnswer(
    char* input, 
    const char* answer,
    int* numGuesses)
{
    int numCorrectLetters = 0;
    int colors[WORD_LENGTH] = { 0 };

    // Check for exactly correct letters
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        char letter = input[index];

        // Is letter in the right spot?
        if (letter == answer[index])
        {
            printAlphabet(
                letter,
                COLOR_GREEN,
                FALSE
            );
            colors[index] = COLOR_GREEN;
            numCorrectLetters++;
        }
    }

    // Copy answer into tempAnswer so we can modify it
    char tempAnswer[WORD_LENGTH + 1] = { 0 };
    for (int index = 0;
         index < (WORD_LENGTH + 1);
         index++)
    {
        tempAnswer[index] = answer[index];
    }

    // Check for correct letters in incorrect spots
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        char letter = input[index];
        if (colors[index] != COLOR_GREEN)
        {
            // Is letter in the word at all?
            if (isLetterInAnswer(
                letter,
                tempAnswer,
                colors
            ))
            {
                printAlphabet(
                    letter,
                    COLOR_YELLOW,
                    FALSE
                );
                colors[index] = COLOR_YELLOW;
            }
        }
    }

    // Copy answer into tempAnswer to reset tempAnswer
    for (int index = 0;
         index < (WORD_LENGTH + 1);
         index++)
    {
        tempAnswer[index] = answer[index];
    }

    // Set any letters not in the word to the color grey
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        int color = colors[index];
        char letter = input[index];

        if (color != COLOR_GREEN &&
            color != COLOR_YELLOW)
        {
            colors[index] = COLOR_GRAY;
            printAlphabet(
                letter,
                COLOR_GRAY,
                FALSE
            );
        }
    }

    // Increment the number of guesses
    (*numGuesses)++;

    // Save cursor position
    doCursorAction(SAVE, 0);

    // Move cursor to guess position, print board
    doCursorAction(MOVE_FROM_TOP, *numGuesses);

    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        printCharInColor(
            input[index],
            colors[index]
        );
    }

    // Restore cursor position
    doCursorAction(RESTORE, 0);
    return numCorrectLetters;
}

int isLetterInAnswer(
    char letter,
    char* tempAnswer,
    int* colors
)
{
    for (int index = 0;
         tempAnswer[index] != '\0';
         index++)
    {
        if (colors[index] != COLOR_GREEN)
        {
            if (letter == tempAnswer[index])
            {
                tempAnswer[index] = '_';
                return TRUE;
            }
        }
    }
    return FALSE;
}

// TODO: this can probably be removed if dictionary search
//          is put somewhere else
int validateInput(char* input)
{
    // Make sure it is in our dictionary
    int isInDictionary = binarySearch(
        input,
        0,
        (NUM_VALID_WORDS - 1)
    );
    if (isInDictionary == -1)
    { 
        // Save cursor position
        doCursorAction(SAVE, 0);

        // Move cursor up one line
        doCursorAction(MOVE_UP, 1);

        printf("%s not in dictionary!", input);

        // Restore cursor position
        doCursorAction(RESTORE, 0);

        return FALSE;
    }

    else
    {
        // Save cursor position
        doCursorAction(SAVE, 0);

        // Move cursor up one line
        doCursorAction(MOVE_UP, 1);

        // Delete everything
        doCursorAction(CLEAR_LINE, 0);

        // Restore cursor position
        doCursorAction(RESTORE, 0);

        return TRUE;
    }
}