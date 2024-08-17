/*
* Chardle
*
* Copyright (C) 2024 ERROR_SUCCESS Software
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#pragma once
#pragma comment( lib, "bcrypt" )

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

// Console colors
typedef enum enumColor
{
    DEFAULT, GREEN, YELLOW, GRAY, ORANGE
} Color;
const char* colorStrings[] =
{
   "\x1b[0m",                // Defualt
   "\x1b[32m",               // Green
   "\x1b[38;2;193;156;0m",   // Yellow (PowerShell fix)
   "\x1b[38;2;103;103;103m", // Gray
   "\x1b[38;2;252;136;3m"    // Orange
};

// Actions performed in the console
typedef enum enumAction
{
    MOVE_UP,        // Move cursor up n lines from the bottom
    MOVE_FROM_TOP,  // Move cursor down n lines from the top
    ERASE,          // Erase n characters from current position
    CLEAR_SCREEN,   // Clear entire screen
    CLEAR_LINE,     // Clear entire line
    SAVE_POS,       // Save the current cursor position
    RESTORE_POS,    // Restore the previously saved position
    USE_ALT_BUFFER, // Switch to alternate screen buffer
    USE_MAIN_BUFFER // Switch to main screen buffer
} Action;

int isGuessInDictionary(char* guess);
void updateKeyboard(char letter, int color, int print);
int endGame(int won, char* answer, int numAnswers);
int binarySearch(const char* guess, int start, int end);
char* getRandomAnswer(int numAnswers);
int getInput(char* buffer);
void doCursorAction(Action action, int numTimes);
int checkAgainstAnswer(char* guess, const char* answer,
                       int* numGuesses);

// Forward declaration of dictionaries found in other files
char dictValid[NUM_VALID_WORDS][WORD_LENGTH + 1];
char dictAnswers[NUM_ANSWERS][WORD_LENGTH + 1];

/// @brief  Entry point, sets up console and
///         runs main game loop
/// @return 0 on success
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

    // Save original console mode so we can restore it later
    DWORD originalConsoleMode = consoleMode;
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    result = SetConsoleMode(
        hOut,
        consoleMode
    );
    assert(result);

    // Switch to alternate screen buffer 
    //      (like launching nano in bash)
    doCursorAction(
        USE_ALT_BUFFER,
        0
    );

    // Setup for main game loop
    int numCorrectLetters = 0;
    int gameStarted = FALSE;
    char* answer = 0;
    int numAnswers = NUM_ANSWERS;
    int numGuesses = 0;
    while (1)
    {
        // Start (or restart) the game
        if (NOT gameStarted)
        {
            // Print whitespace in board area
            int padding = 6;
            for (int newline = 0;
                 newline < (NUM_GUESSES + padding);
                 newline++)
            {
                printf("\n");
            }

            // Get answer for this round
            answer = getRandomAnswer(numAnswers);
            if (NOT answer)
            {
                break;
            }

            // Write guess prompt to console (don't prompt yet)
            printf(
                "Guess a %d-letter word: ",
                WORD_LENGTH
            );
            doCursorAction(
                SAVE_POS,
                0
            );

            // Write quit instructions for the game
            printf(
                "%s\n\nPress escape to quit%s",
                colorStrings[GRAY],
                colorStrings[DEFAULT]
            );
            doCursorAction(
                RESTORE_POS,
                0
            );
            gameStarted = TRUE;
        }

        // Draw keyboard
        updateKeyboard(
            0,
            0,
            TRUE
        );

        // Get guess from user
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

        // Get number of letters guessed correctly
        numCorrectLetters =
            checkAgainstAnswer(
                buffer,
                answer,
                &numGuesses
            );

        // Check if an end condition has been met
        if (numCorrectLetters == WORD_LENGTH ||
            numGuesses == NUM_GUESSES)
        {
            // Do a final print of the keyboard
            updateKeyboard(
                0,
                0,
                TRUE
            );

            // Print end message and check if user wants
            //      to play again
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

    // Switch back to main buffer before quitting out
    doCursorAction(
        USE_MAIN_BUFFER,
        0
    );

    // Reset console to prior settings
    result = SetConsoleMode(
        hOut,
        originalConsoleMode
    );
    assert(result);
}

/// @brief Gets input from the user, only allowing 
///         valid characters
/// @param buffer 
///         - The buffer to place the guess in
/// @return Whether to end the game
///         TRUE to end, FALSE to continue
int getInput(char* buffer)
{
    int numValidChars = 0;

    // Loop until they enter a valid guess or press escape
    while (1)
    {
        char input = _getch();

        // If they press backspace, clear the current character
        if (input == '\b')
        {
            if (numValidChars > 0)
            {
                doCursorAction(
                    ERASE,
                    1
                );
                numValidChars--;
            }
        }

        // If they have guessed a valid character, print it
        //      and put it into buffer
        else if (input >= 'a' &&
                 input <= 'z' &&
                 numValidChars < WORD_LENGTH)
        {
            printf(
                "%c",
                input
            );
            buffer[numValidChars] = input;
            numValidChars++;
        }

        // If they have pressed escape, return TRUE immediately
        else if (input == ESCAPE_KEY)
        {
            buffer[WORD_LENGTH] = 0;
            return TRUE;
        }

        // If they have pressed enter and provided enough valid
        //      characters, check if guess is in valid dict
        else if (input == '\r' &&
                 numValidChars == WORD_LENGTH)
        {
            buffer[WORD_LENGTH] = 0;
            int inDict = isGuessInDictionary(buffer);

            // Guess is valid, return
            if (inDict)
            {
                return FALSE;
            }

            // Otherwise, erase current guess and reset
            //      number of valid characters
            doCursorAction(
                ERASE,
                WORD_LENGTH
            );
            numValidChars = 0;
        }
    }
}

/// @brief Performs an action in the console window
/// @param action 
///         - Which action to perform
/// @param numTimes 
///         - The number of times the action should be
///             performed, or 0 if not applicable
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
        case USE_ALT_BUFFER:
        {
            printf("\x1b[?1049h");
            break;
        }
        case USE_MAIN_BUFFER:
        {
            printf("\x1b[?1049l");
            break;
        }
    }
}

/// @brief Select a random answer from the answers array
/// @param numAnswers 
///         - The number of answers left in the array
/// @return A pointer to the chosen answer in the array
char* getRandomAnswer(int numAnswers)
{
    // Get random number using crypto library
    // Probably overkill, but I like it
    unsigned int randomNumber = 0;
    NTSTATUS status = BCryptGenRandom(
        NULL,
        (unsigned char*) &randomNumber,
        sizeof(unsigned int),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );

    // If there is a problem getting a random number,
    //      quit out immediately
    if (status != 0)
    {
        char errorMessage[50] = { 0 };
        sprintf(
            errorMessage,
            "BCryptGenRandom returned %d. Exiting.\n",
            status
        );
        OutputDebugStringA(
            errorMessage
        );
        return NULL;
    }

    // Select a random answer from within the array
    randomNumber %= numAnswers;
    char* answer = dictAnswers[randomNumber];
    char answerMessage[WORD_LENGTH + 3] = { 0 };

    // Print the answer to the debug stream
    sprintf(
        answerMessage,
        "\n%s\n",
        answer
    );
    OutputDebugStringA(answerMessage);
    return answer;
}

/// @brief Performs a binary search on the "valid" dictionary
/// @param guess 
///         - The guessed word
/// @param start 
///         - Where in the array to start the binary search
/// @param end 
///         - Where in the array to end the binary search
/// @return The index of a valid guess, -1 if guess not found
int binarySearch(const char* guess, int start, int end)
{
    // The word isn't in the array
    if (start > end)
    {
        return -1;
    }

    // Perform the binary search
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

/// @brief Prints end game message and checks if user wants
///         to play again
/// @param won 
///         - Whether the user won or lost the current round
/// @param answer 
///         - The answer in the current round
/// @param numAnswers 
///         - The number of answers in the answer array
///             This gets decremented in this function
/// @return TRUE to end the game, FALSE to continue
int endGame(int won, char* answer, int numAnswers)
{
    // Clear guess prompt
    doCursorAction(
        CLEAR_LINE,
        0
    );

    // Print appropriate round end message
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

    // If they have exhausted the entire answer 
    //      dictionary, quit out
    if (numAnswers == 1)
    {
        printf(
            "%s\n"
            "You have guessed all of the words "
            "in the game!\n"
            "Thanks for playing!\n%s",
            colorStrings[GREEN],
            colorStrings[DEFAULT]
        );
        return TRUE;
    }

    // Ask the user if they would like to play again
    printf("Press any key to play again...");
    char input = _getch();

    // If they press escape, indicate game should end
    if (input == ESCAPE_KEY)
    {
        return TRUE;
    }

    // If they want to continue, reset the keyboard
    for (char letter = 'a';
         letter <= 'z';
         letter++)
    {
        updateKeyboard(
            letter,
            DEFAULT,
            FALSE
        );
    }

    // Clear the screen
    doCursorAction(
        CLEAR_SCREEN,
        0
    );

    // Remove the used answer from the array of answers
    //  by moving the remaining answers "up" in the array
    int numCharsPerElement = WORD_LENGTH + 1;
    int answerIndex = (int)
        ((answer - dictAnswers[0]) /
         numCharsPerElement);
    int numAnswersLeft = numAnswers - answerIndex;
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
    return FALSE;
}

/// @brief Tracks state of the keyboard and 
///         optionally prints it
/// @param letter 
///         - The letter to update, optional
/// @param color 
///         - The color to assign to that letter, optional
/// @param print 
///         - TRUE to print the keyboard, FALSE to not
void updateKeyboard(char letter, int color, int print)
{
    // Track the colors assigned to each letter
    static Color colors[26] = { 0 };

    // Update letter color if a valid letter was indicated
    if (letter >= 'a' &&
        letter <= 'z')
    {
        Color currentColor = colors[letter - 'a'];
        int shouldUpdate = FALSE;

        // Colors should only update in specific cases
        switch (currentColor)
        {
            // Letters should stay green except in the case
            //      of a keyboard reset
            case GREEN:
            {
                shouldUpdate = (color == DEFAULT);
                break;
            }

            // Letters should not go from yellow to gray
            case YELLOW:
            {
                shouldUpdate = (color != GRAY);
                break;
            }

            // Otherwise, update the color
            default:
            {
                shouldUpdate = TRUE;
                break;
            }
        }

        // If update condition is met, update the color
        if (shouldUpdate)
        {
            colors[letter - 'a'] = color;
        }
    }

    // Print the keyboard if asked to do so
    if (print)
    {
        // Save old cursor position
        doCursorAction(
            SAVE_POS,
            0
        );

        // Move into new position to print keyboard
        doCursorAction(
            MOVE_UP,
            5
        );

        // Print the keyboard
        const char* order =
            "qwertyuiop\n asdfghjkl\n  zxcvbnm";

        for (int index = 0;
             index < 31;
             index++)
        {
            // Print whitespace characters immediatley
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

        // Set color back to default
        printf("%s", colorStrings[DEFAULT]);

        // Restore prior cursor position
        doCursorAction(
            RESTORE_POS,
            0
        );
    }
}

/// @brief Checks the users guess against the current answer
/// @param guess 
///         - The current guess from the user
/// @param answer 
///         - The answer for the current round
/// @param numGuesses 
///         - The number of guesses so far 
///             Incremented by this function
/// @return The number of exactly correct letters
int checkAgainstAnswer(
    char* guess,
    const char* answer,
    int* numGuesses)
{
    int numExactLetters = 0;

    // Track color for printing guess in the board
    Color colors[WORD_LENGTH] = { 0 };
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        colors[index] = GRAY;
    }

    // Check for exactly correct letters
    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        char letter = guess[index];
        if (letter == answer[index])
        {
            updateKeyboard(
                letter,
                GREEN,
                FALSE
            );
            colors[index] = GREEN;
            numExactLetters++;
        }
    }

    // Track letters already guessed (for duplicates)
    int letterAlreadyFound[WORD_LENGTH] = { 0 };

    // Check for letters in the answer, but not 
    //   in the right spot
    for (int guessIndex = 0;
         guessIndex < WORD_LENGTH;
         guessIndex++)
    {
        // Track if the letter is in the answer for
        //      updating the keyboard later in the loop
        int letterInAnswer = FALSE;

        char guessLetter = guess[guessIndex];

        // If they have already guessed this letter correctly,
        //      skip it
        if (colors[guessIndex] == GREEN)
        {
            letterInAnswer = TRUE;
            continue;
        }

        // Otherwise, iterate through the answer and check
        //      if the letter is in the answer
        for (int answerIndex = 0;
             answerIndex < WORD_LENGTH;
             answerIndex++)
        {
            // If they have already guessed this letter 
            //  correctly, or we have already found it, 
            //  skip it
            if (colors[answerIndex] == GREEN ||
                letterAlreadyFound[answerIndex])
            {
                continue;
            }

            // Otherwise, if we have found the letter
            //      in the word
            if (guessLetter == answer[answerIndex])
            {
                // Indicate that we have "used" this letter
                letterAlreadyFound[answerIndex] = TRUE;
                
                // Update the color on the board and keyboard
                colors[guessIndex] = YELLOW;
                updateKeyboard(
                    guessLetter,
                    YELLOW,
                    FALSE
                );

                // Do not turn the letter gray on the keyboard
                letterInAnswer = TRUE;
                break;
            }
        }

        // If the letter was not found in the answer, turn it 
        //      gray on the keyboard
        // The board color array is already gray by default
        if (NOT letterInAnswer)
        {
            updateKeyboard(
                guessLetter,
                GRAY,
                FALSE
            );
        }
    }

    // Increment number of guesses 
    *numGuesses += 1;

    // Save current cursor position
    doCursorAction(
        SAVE_POS,
        0
    );

    // Print board
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

    // Reset color to default
    printf(
        "%s",
        colorStrings[DEFAULT]
    );

    // Restore prior cursor position
    doCursorAction(
        RESTORE_POS,
        0
    );
    return numExactLetters;
}

/// @brief Checks if a guess is in the valid dictionary
/// @param guess 
///         - The user's guess
/// @return TRUE if guess is in valid dictionary, FALSE if not
int isGuessInDictionary(char* guess)
{
    // Search for guess in dictionary
    int inDict = binarySearch(
        guess,
        0,
        (NUM_VALID_WORDS - 1) // End is inclusive!
    );

    // Save current position
    doCursorAction(
        SAVE_POS,
        0
    );

    // Print "not in dictionary" message, or clear the
    //  message if guess is in dictionary
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

    // Restore prior cursor position
    doCursorAction(
        RESTORE_POS,
        0
    );

    return inDict;
}