/*
 * HOMEWORK:
 * - Try to find a worlde clone online that mimics wordle's
 *      duplicate behavior
 * - What is this hashmap thing chat won't shut up about ;)
 * - Cross-reference with other dictionaries to improve completeness
 * 
 * NOW:
 * 
 * NEXT STREAM: 
 * 
 * FUTURE:
 * Nicer UI
 */

#define WIN32_LEAN_AND_MEAN

#pragma comment( lib, "bcrypt" )

#pragma once
#include "Chardle.h"
#include <stdio.h>
#include <assert.h>
#include <bcrypt.h>

#define COLOR_NORMAL    0
#define COLOR_GREEN     32
#define COLOR_YELLOW    33

int validateInput(char* input);
int checkInputAgainstAnswer(char* input, const char* answer);
void printText(char text, int color);
void printTextArray(char* text, int color);
int isLetterInAnswer(char letter, char* tempAnswer, int* colors);
void clearScreen(void);
void printAlphabet(char letter, int color, int print);
int endGame(int won, const char* answer);
int binarySearch(const char* guess, int start, int end);

// RESEARCH: Is it bad that this is global?
char dictionary[NUM_WORDS][WORD_LENGTH + 1];

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

    // Get random word
    unsigned int randomNumber = 0;
    NTSTATUS status = BCryptGenRandom(
        NULL,
        (unsigned char*) &randomNumber,
        sizeof(unsigned int),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    assert(status == 0);
    randomNumber %= NUM_WORDS;
    const char* answer = dictionary[randomNumber];
    printf("%s\n", answer);

    char buffer[MAX_STRING_LENGTH];
    int numCorrectLetters = 0;
    int numGuesses = 0;
    while (1)
    {
        printAlphabet(
            0,
            0, 
            TRUE
        );
        printf(
            "Guess a %d-letter word, or press q to quit: ",
            WORD_LENGTH
        );
        fgets(
            buffer,
            MAX_STRING_LENGTH,
            stdin
        );
        buffer[MAX_STRING_LENGTH - 1] = '\0';

        // If the user has pressed 'q', quit immediately
        if (buffer[0] == 'q' && buffer[1] == '\n')
        {
            break;
        }

        int valid = validateInput(buffer);
        if (valid)
        {
            numCorrectLetters =
                checkInputAgainstAnswer(
                    buffer,
                    answer
                );
           numGuesses++;
        }

        // An end condition has been met
        if (numCorrectLetters == WORD_LENGTH || 
            numGuesses == NUM_GUESSES)
        {
            int won = (numCorrectLetters == WORD_LENGTH);
            if (endGame(
                won, 
                answer
            ))
            {
                break;
            }
            numCorrectLetters = 0;
            numGuesses = 0;
        }
    }

    result = SetConsoleMode(
        hOut, 
        originalConsoleMode
    );
    assert(result);
}

int binarySearch(const char* guess, int start, int end)
{
    // word isn't in the array
    if (start > end)
    {
        return -1;
    }
    int mid = end + (start - end) / 2;
    char* midWord = dictionary[mid];

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

int endGame(int won, const char* answer)
{
    // Print game end message
    if (won)
    {
        printTextArray(
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

    // Ask the user if they would like to play again
    char buffer[MAX_STRING_LENGTH] = { 0 };
    printf(
        "Press any key to play again, or q to quit: "
    );
    fgets(
        buffer,
        MAX_STRING_LENGTH,
        stdin
    );
    buffer[MAX_STRING_LENGTH - 1] = '\0';
    if (buffer[0] == 'q' && buffer[1] == '\n')
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
    clearScreen();
    return FALSE;
}

// If they pass 0 as the letter parameter, skip updates
void printAlphabet(char letter, int color, int print)
{
    static char alphabet[26] = { 0 };
    static int colors[26] = { COLOR_NORMAL };

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
        letter <= 'z' &&
        colors[letter - 'a'] != COLOR_GREEN)
    {
        colors[letter - 'a'] = color;
    }

    if (print)
    {
        for (int index = 0;
             index < 26;
             index++)
        {
            printText(
                alphabet[index], 
                colors[index]
            );
            printf(" ");
        }
        printf("\n");
    }
}

void clearScreen(void)
{
    wchar sequences[3][7] = {
        L"\x1b[2J",
        L"\x1b[3J",
        L"\x1b[0;0H"
    };

    for (int index = 0;
         index < 3;
         index++)
    {
        wchar* sequence = sequences[index];
        DWORD sequenceLength = (DWORD) wcslen(sequence);
        DWORD written = 0;
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
        BOOL result = WriteConsoleW(
            hStdOut,
            sequence,
            sequenceLength,
            &written,
            NULL
        );
        assert(result);
        assert(written == sequenceLength);
    }
}

void printTextArray(char* text, int color)
{
    printf(
        "\x1b[%dm%s", 
        color, 
        text
    );
    printf(
        "\x1b[%dm", 
        COLOR_NORMAL
    );
}

void printText(char text, int color)
{
    printf(
        "\x1b[%dm%c", 
        color, 
        text
    );
    printf(
        "\x1b[%dm", 
        COLOR_NORMAL
    );
}

int checkInputAgainstAnswer(char* input, const char* answer)
{
    int correctLetters = 0;
    int colors[WORD_LENGTH] = { COLOR_NORMAL };

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
            correctLetters++;
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

    for (int index = 0;
         index < WORD_LENGTH;
         index++)
    {
        printText(
            input[index], 
            colors[index]
        );
    }
    printf("\n");
    return correctLetters;
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

int validateInput(char* input)
{
    // Make sure input is 5 characters long
    int length;
    for (length = 0;
         input[length] != '\0';
         length++)
    { };
    if (length != 6)
    {
        return FALSE;
    }

    // Make sure input only contains letters
    for (int index = 0;
         index < (length - 1); // Avoid newline character
         index++)
    {
        char letter = input[index];

        // Convert from uppercase if necessary
        if (letter >= 'A' &&
            letter <= 'Z')
        {
            letter += LOWERCASE_OFFSET;
            input[index] = letter;
        }

        // Verify letter is lowercase
        if (letter < 'a' || letter > 'z')
        {
            return FALSE;
        }
    }

    // Make sure it is in our dictionary
    int isInDictionary = binarySearch(
        input, 
        0, 
        (NUM_WORDS - 1)
    );
    if (isInDictionary == -1)
    {
        printf("%s not in dictionary!\n", input);
        return FALSE;
    }
    return TRUE;
}