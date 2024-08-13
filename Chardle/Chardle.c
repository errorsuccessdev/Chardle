/*
 * HOMEWORK:
 * - Try to find a worlde clone online that mimics wordle's
 *      duplicate behavior
 * - What is this hashmap thing chat won't shut up about ;)
 *
 * NOW:
 *
 * NEXT STREAM:
 * Replace calls to printCharactersInColor with directly using
 *  the string to print something in color
 *
 * FUTURE:
 * Fix duplicate letter not turning green in alphabet
 * Other UI improvements
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

#define NOT         !
#define TRUE        1
#define FALSE       0
#define ESCAPE_KEY  27
#define NUM_GUESSES 6

typedef enum enumColor
{
    DEFAULT, GREEN, YELLOW, GRAY, ORANGE
} Color;
const char* colorStrings[] =
{
   "\x1b[0m",                // Defualt
   "\x1b[32m",               // Green
   "\x1b[33m",               // Yellow
   "\x1b[38;2;103;103;103m", // Gray
   "\x1b[38;2;252;136;3m"    // Orange
};

typedef enum enumAction
{
    MOVE_UP, MOVE_FROM_TOP,
    ERASE, CLEAR_SCREEN, CLEAR_LINE,
    SAVE_POS, RESTORE_POS
} Action;

// TODO: To use or not to use?
typedef struct structLetter
{
    char character;
    Color color;
} Letter;

int isGuessInDictionary(char* guess);
void printAlphabet(char letter, int color, int print);
int endGame(int won, char* answer, int numAnswers);
int binarySearch(const char* guess, int start, int end);
char* getRandomAnswer(int numAnswers);
int getInput(char* buffer);
void doCursorAction(Action action, int numTimes);
int checkAgainstAnswer(char* guess, const char* answer,
                       int* numGuesses);

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
            int padding = 6;
            for (int newline = 0;
                 newline < (NUM_GUESSES + padding);
                 newline++)
            {
                printf("\n");
            }

            answer = getRandomAnswer(numAnswers);
            printf(
                "Guess a %d-letter word, "
                "or press ESC to quit: ",
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
        int shouldEnd = getInput(buffer);

        // If the user has pressed escape, quit
        if (shouldEnd)
        {
            break;
        }

        // Erase user's current guess
        doCursorAction(
            ERASE,
            WORD_LENGTH
        );

        numCorrectLetters =
            checkAgainstAnswer(
                buffer,
                answer,
                &numGuesses
            );

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
                doCursorAction(
                    ERASE,
                    1
                );
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
            return TRUE;
        }
        else if (input == '\r' &&
                 numEnteredChars == WORD_LENGTH)
        {
            buffer[WORD_LENGTH] = 0;
            int inDict = isGuessInDictionary(buffer);
            if (inDict)
            {
                return FALSE;
            }
            doCursorAction(ERASE, WORD_LENGTH);
            numEnteredChars = 0;
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
                0 // Top row
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
            printf("\x1b[2K\x1b[0G");
            break;
        }
        case CLEAR_SCREEN:
        {
            printf("\x1b[2J\x1b[3J\x1b[0;0H");
            break;
        }
        case SAVE_POS:
        {
            printf(
                "\x1b%d",
                7 // Save cursor position
            );
            break;
        }
        case RESTORE_POS:
        {
            printf(
                "\x1b%d",
                8 // Restore cursor position
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
    // The word isn't in the array
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
    // Clear input message
    doCursorAction(
        CLEAR_LINE,
        0
    );
    doCursorAction(
        MOVE_UP,
        1
    );
    if (won)
    {
        printf(
            "%sCongratulations, you've won!\n%s",
            colorStrings[GREEN], 
            colorStrings[DEFAULT]
        );
    }
    else
    {
        printf(
            "%sSorry, the word was %s%s\n",
            colorStrings[ORANGE],
            answer,
            colorStrings[DEFAULT]
        );
    }

    // If they have exhausted the entire answer dictionary,
    //      quit out
    if (numAnswers == 1)
    {
        printf(
            "%s\n"
            "You have guessed all of the words in the game!\n"
            "Thanks for playing!\n%s",
            colorStrings[GREEN],
            colorStrings[DEFAULT]
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
            DEFAULT,
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
    doCursorAction(
        CLEAR_SCREEN,
        0
    );
    return FALSE;
}

// If they pass 0 as the letter parameter, skip updates
void printAlphabet(char letter, int color, int print)
{
    static Color colors[26] = { 0 };

    // Update letter color if desired
    if (letter >= 'a' &&
        letter <= 'z')
    {
        colors[letter - 'a'] = color;
    }

    if (print)
    {
        doCursorAction(
            SAVE_POS,
            0
        );
        doCursorAction(
            MOVE_UP,
            5
        );

        char* order = "qwertyuiop\n asdfghjkl\n  zxcvbnm";

        for (int index = 0;
             index < 31;
             index++)
        {
            if (order[index] == ' ' ||
                order[index] == '\n')
            {
                printf("%c", order[index]);
            }
            else
            {
                // Calculate where the next letter on the
                // keyboard is in the color array
                int pos = order[index] - 'a';

                // Print the letter and color of that "key"
                printf(
                    "%s%c",
                    colorStrings[colors[pos]],
                    order[index]
                );
                printf(" ");
            }
        }
        doCursorAction(
            RESTORE_POS,
            0
        );
    }
}

// TODO: This could certainly be refactored into something nicer
int checkAgainstAnswer(
    char* guess,
    const char* answer,
    int* numGuesses)
{
    int numCorrectLetters = 0;
    Color colors[WORD_LENGTH] = { 0 };

    // Check for exactly correct letters
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        char letter = guess[index];

        // Is letter in the right spot?
        if (letter == answer[index])
        {
            printAlphabet(
                letter,
                GREEN,
                FALSE
            );
            colors[index] = GREEN;
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
        char letter = guess[index];
        if (colors[index] != GREEN)
        {
            // Is letter in the answer anywhere?
            int letterInAnswer = FALSE;
            for (int index = 0;
                 tempAnswer[index] != '\0';
                 index++)
            {
                if (colors[index] != GREEN)
                {
                    if (letter == tempAnswer[index])
                    {
                        tempAnswer[index] = '_';
                        letterInAnswer = TRUE;
                    }
                }
            }
            if (letterInAnswer)
            {
                printAlphabet(
                    letter,
                    YELLOW,
                    FALSE
                );
                colors[index] = YELLOW;
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
        char letter = guess[index];

        if (color != GREEN &&
            color != YELLOW)
        {
            colors[index] = GRAY;
            printAlphabet(
                letter,
                GRAY,
                FALSE
            );
        }
    }

    *numGuesses += 1;
    doCursorAction(
        SAVE_POS,
        0
    );
    doCursorAction(
        MOVE_FROM_TOP,
        *numGuesses
    );
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        printf(
            "%s%c",
            colorStrings[colors[index]],
            guess[index]
        );
    }
    doCursorAction(
        RESTORE_POS,
        0
    );
    return numCorrectLetters;
}

int isGuessInDictionary(char* guess)
{
    int inDict = binarySearch(
        guess,
        0,
        (NUM_VALID_WORDS - 1)
    );

    doCursorAction(
        SAVE_POS,
        0
    );
    doCursorAction(
        MOVE_UP,
        1
    );

    if (inDict == -1)
    {
        printf(
            "%s not in dictionary!",
            guess
        );
        inDict = FALSE;
    }
    else
    {
        doCursorAction(
            CLEAR_LINE,
            0
        );
        inDict = TRUE;
    }

    doCursorAction(
        RESTORE_POS,
        0
    );

    return inDict;
}